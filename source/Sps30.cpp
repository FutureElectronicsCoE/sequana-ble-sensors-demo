/*
 * Copyright (c) 2018 Future Electronics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mbed.h>
#include "Sps30.h"
#include "Callback.h"

static uint8_t sps30_command_start_bytes[] = {0x7E, 0x00, 0x00, 0x02, 0x01, 0x03, 0xF9, 0x7E}; //start
static uint8_t sps30_command_read_bytes[] = {0x7E, 0x00, 0x03, 0x00, 0xFC, 0x7E}; //read
static uint8_t sps30_command_clean_bytes[] = {0x7E, 0x00, 0x80, 0x05, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7B, 0x7E}; //auto-clean
static uint8_t sps30_command_reset_bytes[] = {0x7E, 0x00, 0xD3, 0x00, 0x2C, 0x7E}; //reset

#define PM_1_0_OFFSET       0
#define PM_2_5_OFFSET       4
#define PM_10_OFFSET        12
#define SPS30_DATA_SIZE     40

#ifdef MBED_DEBUG
static uint32_t rx_rq_stat = 0;
static uint32_t rx_done_stat = 0;
static uint32_t tx_done_stat = 0;
static uint32_t stuff_stat = 0;
static uint32_t total_bytes_stat = 0;
#endif // MBED_DEBUG


static void copy_uint32_be(uint32_t &d, uint8_t *src)
{
    float f;
    uint8_t *dest = (uint8_t *)&f;
    int i;

    for (i = 3; i >= 0; --i)
    {
        dest[i] = *src++;
    }

    d = f; // conversion float->uint
}


void Sps30Driver::shdlc_state_machine(uint8_t data)
{
#ifdef MBED_DEBUG
    ++total_bytes_stat;
#endif // MBED_DEBUG

    switch (_shdlc_state) {
        case ERROR:
        case READY:
        case SEARCH:
            if (data == SHDLC_FRAME_START_SYMBOL) {
                _shdlc_length = 0;
                _shdlc_state = GET;
            }
            break;

        case STUFF:
            if(data == 0x5E) {
                data = 0x7E;
            } else if(data == 0x5D) {
                data = 0x7D;
            } else if(data == 0x31) {
                data = 0x11;
            } else if(data == 0x33) {
                data = 0x13;
            } else {
                error_recovery();
                break;
            }
            if (_shdlc_length < SHDLC_MISO_FRAME_MAX_SIZE) {
                _frame.raw[_shdlc_length++] = data;
            } else {
                error_recovery();
                break;
            }
            _shdlc_state = GET;
            break;

        case GET:
            if (data == SHDLC_FRAME_STUFF_SYMBOL) {
#ifdef MBED_DEBUG
                ++stuff_stat;
#endif // MBED_DEBUG
                _shdlc_state = STUFF;
            } else if (data == SHDLC_FRAME_START_SYMBOL) {
                if (_shdlc_length == 0) {
                    // assume multiple start symbols may fill inter-frame space
                    // or may have get lost of sync - just ignore extra start symbol
                    break;
                }
                // End of Frame
                if (_shdlc_length < SHDLC_MISO_FRAME_MAX_SIZE) {
                    _frame.raw[_shdlc_length] = data;    // To support frame validation.
                    if (shdlc_frame_is_valid())
                    {
                        _shdlc_state = READY;
                    }
                } else {
                    error_recovery();
                }
            } else {
                if (_shdlc_length < SHDLC_MISO_FRAME_MAX_SIZE) {
                    _frame.raw[_shdlc_length++] = data;
                } else {
                    error_recovery();
                }
            }
            break;
    }
}

// Validate incoming frame.
bool Sps30Driver::shdlc_frame_is_valid()
{
    bool valid = false;

    // Minimum frame length must be considered.
    valid = (_shdlc_length >= (SHDLC_MISO_HDR_LENGTH + 1)); // header + checksum
    if (valid) {
        // Frame length must be consistent.
        valid = (_shdlc_length == (_frame.f.len + SHDLC_MISO_HDR_LENGTH + 1));
    }
    if (valid) {
        // Frame end must be clearly marked.
        valid = _frame.raw[_shdlc_length] == SHDLC_FRAME_START_SYMBOL;
    }
    if (valid) {
        // Checksum must be correct.
        valid = (_frame.raw[_shdlc_length - 1] == shdlc_frame_checksum());
        if (!valid) {
            printf("SPS30 frame checksum error!\n");
        }
    }
    return valid;
}


uint8_t Sps30Driver::shdlc_frame_checksum()
{
    uint32_t i, chk;

    chk = 0;
    for (i = 0; i < (_shdlc_length - 1); ++i) {
        chk += _frame.raw[i];
    }

    return (uint8_t)(~(chk & 0xFF));
}


void Sps30Driver::error_recovery()
{
    _shdlc_state = ERROR;
    _shdlc_length = 0;
    _status = STATUS_PROTOCOL_ERROR;
}


uint32_t Sps30Driver::num_bytes_needed()
{
    uint32_t bytes = SHDLC_MISO_FRAME_MIN_SIZE + 1; // need also first start symbol

    switch (_shdlc_state) {
        case ERROR:
        case READY:
        case SEARCH:
            // We are starting anew, already has right value;
            break;

        case STUFF:
        case GET:
            if (_shdlc_length < SHDLC_MISO_HDR_LENGTH) {
                // need at least to complete header + checksum + start symbol
                bytes = SHDLC_MISO_HDR_LENGTH + 2 - _shdlc_length;
            } else {
                bytes = _frame.f.len + SHDLC_MISO_HDR_LENGTH + 2 - _shdlc_length;
            }
            break;
    }
    return bytes;
}


void Sps30Driver::start_reading()
{
    if (!_read_in_progress) {
        _read_in_progress = true;
        _serial_size = num_bytes_needed();
        _serial.read(_serial_buf, _serial_size, callback(this, &Sps30Driver::rx_done));
#ifdef MBED_DEBUG
        ++rx_rq_stat;
#endif // MBED_DEBUG
    }
}


void Sps30Driver::reset_frame()
{
    _serial.abort_read();
    _read_in_progress = false;
    _shdlc_length = 0;
    _shdlc_state = SEARCH;
    _status = STATUS_NOT_READY;
}


void Sps30Driver::retrieve_data()
{
    MBED_ASSERT(_shdlc_state == READY);
    if (_frame.f.len == SPS30_DATA_SIZE) {
        copy_uint32_be(_value.pm_1_0, &_frame.f.data[PM_1_0_OFFSET]);
        copy_uint32_be(_value.pm_2_5, &_frame.f.data[PM_2_5_OFFSET]);
        copy_uint32_be(_value.pm_10, &_frame.f.data[PM_10_OFFSET]);
        _status = STATUS_OK;
   } else {
       // may got wrong (previous) frame, continue reading
        _shdlc_length = 0;
        _shdlc_state = SEARCH;
        start_reading();
    }
}

void Sps30Driver::rx_done(int event)
{
#ifdef MBED_DEBUG
    ++rx_done_stat;
#endif // MBED_DEBUG
    if (event == SERIAL_EVENT_RX_COMPLETE) {
        uint32_t index = 0;
        while (index < _serial_size) {
            shdlc_state_machine(_serial_buf[index++]);
        }
        _read_in_progress = false;
        if (_shdlc_state == READY) {
                retrieve_data();
        } else {
            // need more data
            start_reading();
        }
    } else {
        // error
        _shdlc_state = ERROR;
        _status = STATUS_RX_ERROR;
        _read_in_progress = false;
    }
}


void Sps30Driver::tx_done(int event)
{
#ifdef MBED_DEBUG
    ++tx_done_stat;
#endif // MBED_DEBUG
    if (event & SERIAL_EVENT_ERROR) {
        // error
        _shdlc_state = ERROR;
        _status = STATUS_TX_ERROR;
    }
}


Sps30Driver::Status Sps30Driver::request_new_frame()
{
#ifdef MBED_DEBUG
    stuff_stat = 0;
#endif // MBED_DEBUG
    reset_frame();
    start_reading();
    _serial.write(sps30_command_read_bytes, sizeof(sps30_command_read_bytes), callback(this, &Sps30Driver::tx_done));
    return STATUS_OK;
}


Sps30Driver::Status Sps30Driver::send_start()
{
    _serial.write(sps30_command_start_bytes, sizeof(sps30_command_start_bytes),NULL);
    return STATUS_OK;
}



Sps30Driver::Status Sps30Driver::read(Sps30Value& value)
{
#ifdef MBED_DEBUG
    printf("SPS30 read: shdlc=%d, stat=%d, rxq= %lu, rxd=%lu, txd=%lu, stuff=%lu, bcnt=%lu\n", _shdlc_state, _status, rx_rq_stat, rx_done_stat, tx_done_stat, stuff_stat, total_bytes_stat);
    printf("PM1.0 = %lu, PM2.5 = %lu, PM10 = %lu\n", _value.pm_1_0, _value.pm_2_5, _value.pm_10);
#endif // MBED_DEBUG
    if (_status == STATUS_OK) {
        value = _value;
    }
    return _status;
}


Sps30Driver::Sps30Driver(RawSerial &serial) :
    _serial(serial),
    _read_in_progress(false),
    _shdlc_state(ERROR),
    _shdlc_length(0),
    _status(STATUS_NOT_READY)
{
    _serial.write(sps30_command_start_bytes, sizeof(sps30_command_reset_bytes),NULL);
}


/** Callback function periodically updating sensor value.
 */
void Sps30Sensor::updater()
{
    Sps30Value val;

    if (_driver.read(val) == Sps30Driver::STATUS_OK) {
        update_value(val);
    }
    _driver.request_new_frame();
}


/** Initialize driver and setup periodic sensor updates.
 */
void Sps30Sensor::start(EventQueue& ev_queue)
{
    ev_queue.call_every(5000, callback(this, &Sps30Sensor::updater));
    _driver.send_start();
}



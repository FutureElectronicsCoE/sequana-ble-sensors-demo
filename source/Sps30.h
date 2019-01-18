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

#ifndef SPS30_H_
#define SPS30_H_

#include <stdint.h>
#include <mbed.h>
#include "Sensor.h"


/** Represents measurement result received from SPS30 particulate matter sensor.
 * When this matches format of the sensor characteristic then
 * no conversion is needed.
 */
struct Sps30Value {
    uint32_t    pm_1_0;
    uint32_t    pm_2_5;
    uint32_t    pm_10;
};


/** Driver for SPS30 sensor.
 */
class Sps30Driver {
public:
    /** Measurement result status.
     */
    enum Status {
        STATUS_OK = 0,
        STATUS_NOT_READY,
        STATUS_TX_ERROR,
        STATUS_RX_ERROR,
        STATUS_PROTOCOL_ERROR
    };

    /** Creates and initializes driver.
     *
     * @param serial RawSerial interface to use for communication.
     */
    Sps30Driver(RawSerial &serial);

    /** Request new measurement result.
     * Result of the measurement is stored internally.
     *
     * @returns operation status
     */
    Status request_new_frame();

    /** Read the last obtained measurement result.
     *
     * @param value measurement result
     * @returns operation status
     */
    Status read(Sps30Value& value);

    /** Send 'start measurement' command to the sensor..
     *
     * @returns operation status
     */
    Status send_start();

protected:
    /** SPS30 commands.
     */
    enum Command {
        START_MEASUREMENT   = 0x00,
        STOP_MEASUREMENT    = 0x01,
        READ_MEASURED_VALUE = 0x03,
        AUTO_CLEAN_INTERVAL = 0x80,
        DEVICE_INFORMATION  = 0xd0,
        RESET               = 0xd3
    };

    static const uint8_t SHDLC_FRAME_START_SYMBOL = 0x7E;
    static const uint8_t SHDLC_FRAME_STUFF_SYMBOL = 0x7D;

    static const size_t SHDLC_DATA_MAX_LEN          = 255;
    static const size_t SHDLC_MISO_HDR_LENGTH       = 4;
    static const size_t SHDLC_MISO_FRAME_MAX_SIZE   = SHDLC_MISO_HDR_LENGTH + SHDLC_DATA_MAX_LEN + 2;
    static const size_t SHDLC_MISO_FRAME_MIN_SIZE   = SHDLC_MISO_HDR_LENGTH + 2; // header + checksum + (ending)start symbol

    struct SHDLCFrame {
        uint8_t addr;
        uint8_t cmd;
        uint8_t state;
        uint8_t len;
        uint8_t data[SHDLC_DATA_MAX_LEN];
    };

    union SHDLCRawFrame {
        struct SHDLCFrame   f;
        uint8_t             raw[SHDLC_MISO_FRAME_MAX_SIZE];
    };

    enum SHDLCState {
        ERROR = 0,
        READY,
        SEARCH,
        STUFF,
        GET
    };

protected:
    void    shdlc_state_machine(uint8_t data);
    bool    shdlc_frame_is_valid();
    uint8_t shdlc_frame_checksum();
    uint32_t num_bytes_needed();
    void    error_recovery();
    void    retrieve_data();
    void    reset_frame();
    void    start_reading();
    void    rx_done(int event);
    void    tx_done(int event);

protected:
    Sps30Value      _value;
    RawSerial       &_serial;
    bool            _read_in_progress;
    SHDLCState      _shdlc_state;
    uint32_t        _shdlc_length;
    Status          _status;
    size_t          _serial_size;
    SHDLCRawFrame   _frame;
    uint8_t         _serial_buf[SHDLC_MISO_FRAME_MAX_SIZE + 1];

};

/** SPS30 particulate matter sensor interface.
 */
class Sps30Sensor : public Sensor<Sps30Value> {
public:
    /** Creates and initializes sensor interface.
     *
     * @param serial RawSerial interface to use for communication.
     */
    Sps30Sensor(RawSerial &serial) : _driver(serial) {}

    /** Schedule measurement process.
     */
    virtual void start(EventQueue& ev_queue);

protected:
    void updater();
    Sps30Driver _driver;
};


#endif // SPS30_H

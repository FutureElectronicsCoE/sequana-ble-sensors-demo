/*
 * Copyright (c) 2017-2019 Future Electronics
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
#include "Kx64.h"


Kx64Driver::Kx64Driver(SPI& bus, PinName cs) : _spi(bus), _chip_select(cs)
{
    _chip_select = 1;
};


// For each 8-bit register access must first send register address.
uint8_t Kx64Driver::spi_transaction(uint8_t address, uint8_t data) {
    _chip_select = 0;

    _tx_buffer[0] = address;
    _tx_buffer[1] = data;

    _spi.write(_tx_buffer, 2, _rx_buffer, 2);
    _chip_select = 1;
    // SSEL needs to be set inactive for at least 100ns.
    for (uint32_t j = 0; j < 16; ++j) {
        volatile uint32_t tmp = 0;
    }
    return _rx_buffer[1];
}

/** Read multiple bytes/registers from KX chip.
 * The tricky part here is that the first byte read corresponds to address write
 * and is always 0, so it should be ignored and the count should be 1 more than
 * the really required data.
 */
void Kx64Driver::spi_read_multiple(uint8_t reg_address, uint8_t *data, uint32_t count) {
    _chip_select = 0;

    _tx_buffer[0] = reg_address | READ_MASK;

    _spi.write(_tx_buffer, 1, (char*)data, count);
    _chip_select = 1;
    // SSEL needs to be set inactive for at least 100ns.
    for (uint32_t j = 0; j < 16; ++j) {
        volatile uint32_t tmp = 0;
    }
}


#define AccScale(x) (int16_t)(((int32_t)(x) * 2 * 16000) >> 16)
#define MagScale(x) (int16_t)((((int32_t)(x)) * 2 * 12000) >> 16)

Kx64Driver::Status Kx64Driver::read(Kx64Value& value)
{
    uint8_t buffer[14];
//    uint8_t status[3];
//    spi_read_multiple(BUF_STATUS_1, status, 3);

    spi_read_multiple(BUF_READ, &buffer[1], 13);

#if 0
    printf("kx64: raw=");
    for (uint32_t i = 2; i < 14; i+=2) {
        printf("%02x%02x ", buffer[i+1], buffer[i]);
    }
    printf("  status %02x%02x\n", status[2], status[1]);
#endif // 0

    value.acc_x = AccScale(*reinterpret_cast<int16_t*>(buffer+2));
    value.acc_y = AccScale(*reinterpret_cast<int16_t*>(buffer+4));
    value.acc_z = AccScale(*reinterpret_cast<int16_t*>(buffer+6));
    value.mag_x = MagScale(*reinterpret_cast<int16_t*>(buffer+8));
    value.mag_y = MagScale(*reinterpret_cast<int16_t*>(buffer+10));
    value.mag_z = MagScale(*reinterpret_cast<int16_t*>(buffer+12));

#if 0
    printf("Acc: %8d %8d %8d   Mag: %8d %8d %8d\n",
           value.acc_x, value.acc_y, value.acc_z,
           value.mag_x, value.mag_y, value.mag_z);
#endif // 0

    return STATUS_OK;
}

void Kx64Driver::clear_buffer()
{
    spi_transaction(BUF_CLEAR, 0x00);
}

void Kx64Driver::init_chip()
{
    // Initialize chip
    spi_transaction(CNTL2, 0x14);       // Acc range 8g, disable sensors, oversampling
    spi_transaction(CNTL1, 0x03);       // Mag range 1200uT
    spi_transaction(ODCNTL, 0x00);      // data rate 12.5 sps
    spi_transaction(BUF_CTRL1, 12);     // trig level
    spi_transaction(BUF_CTRL2, 0x00);   // buffer FIFO mode
    spi_transaction(BUF_CTRL3, 0x7E);   // enable all Acc and Mag data
    spi_transaction(CNTL2, 0x17);       // enable sensors
}


/** Callback function periodically updating sensor value.
 */
void Kx64Sensor::updater()
{
    if (_driver.read(_value) == Kx64Driver::STATUS_OK) {
        update_notify();
    };
    _driver.clear_buffer();
}

/** Initialize driver and setup periodic sensor updates.
 */
void Kx64Sensor::start(EventQueue& ev_queue)
{
    _driver.init_chip();
    _driver.clear_buffer();
    ev_queue.call_every(500, callback(this, &Kx64Sensor::updater));
}

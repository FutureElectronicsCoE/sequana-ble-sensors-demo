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

#ifndef KX64_H_
#define KX64_H_

#include <stdint.h>
#include <Sensor.h>


/** Represents measurement result received from KX64/65 accelerometer and magnetometer sensor.
 * When this matches format of the sensor characteristic then
 * no conversion is needed.
 */
struct Kx64Value {
    int16_t     acc_x;
    int16_t     acc_y;
    int16_t     acc_z;
    int16_t     mag_x;
    int16_t     mag_y;
    int16_t     mag_z;
};


/** Driver for KX64/65 sensor.
 */
class Kx64Driver {
public:
    /** Last measurement result status.
     */
    enum Status {
        STATUS_OK = 0,
        STATUS_STALLED,
        STATUS_NOT_READY
    };

    /** KX64 chip register addresses.
     */
     enum Register {
        WHO_AM_I        = 0x00,
        ODCNTL          = 0x38,
        CNTL1           = 0x39,
        CNTL2           = 0x3A,
        BUF_CTRL1       = 0x77,
        BUF_CTRL2       = 0x78,
        BUF_CTRL3       = 0x79,
        BUF_CLEAR       = 0x7A,
        BUF_STATUS_1    = 0x7B,
        BUF_STATUS_2    = 0x7C,
        BUF_STATUS_3    = 0x7D,
        BUF_READ        = 0x7E,
        READ_MASK       = 0x80
     };

public:
    /** Create and initialize driver.
     *
     * @param bus SPI bus to use
     * @param cs CS/SS pin to use to select sensor on a bus
     */
    Kx64Driver(SPI& bus, PinName cs);

    /** Obtain reading of the sensor.
     *
     * @param[out] value measurement result
     * @returns operation status
     */
    Status read(Kx64Value& value);

    /** Initialize sensor chip after reset.
     */
    void init_chip();

    /** Clear readout buffer.
     *
     * Sensor read out is perform via the FIFO buffer included within the chip.
     * To make sure the readout is synchronized with measurement this buffer
     * needs to be cleared prior to starting the next measurement cycle.
     */
    void clear_buffer();

protected:
    uint8_t spi_transaction(uint8_t address, uint8_t data);
    void    spi_read_multiple(uint8_t address, uint8_t *data, uint32_t length);

    SPI&        _spi;
    DigitalOut  _chip_select;
    char        _tx_buffer[2];
    char        _rx_buffer[2];
};


/** KX64 accelerometer/magnetometer sensor interface.
 */
class Kx64Sensor : public Sensor<Kx64Value> {
public:
    /** Create and initialize sensor interface.
     *
     * @param spi SPI bus to use
     * @param cs CS/SS pin to use to select sensor on a bus
     */
    Kx64Sensor(SPI &spi, PinName cs) : _driver(spi, cs) {}

    /** Schedule measurement process.
     */
    virtual void start(EventQueue& ev_queue);

protected:
    void updater();

    Kx64Driver _driver;
};

#endif // KX64_H_

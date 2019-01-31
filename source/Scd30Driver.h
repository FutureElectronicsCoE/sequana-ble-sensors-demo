/*
 * Copyright (c) 2019 Future Electronics
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

#ifndef SCD30_DRIVER_H_
#define SCD30_DRIVER_H_

#include <stdint.h>
#include <Sensor.h>


/** Driver for Sensirion SCD30 CO2 sensor.
 */
class Scd30Driver {
public:
    enum Status {
        STATUS_OK = 0,
        STATUS_STALLED,
        STATUS_NOT_READY,
        STATUS_I2C_ERROR,
        STATUS_CRC_ERROR
    };

public:
    /** Create and initialize driver.
     *
     * @param bus I2C bus to use for communication
     * @param address I2C address to use
     */
    Scd30Driver(I2C& bus, uint8_t address);

    /** Read measured value from sensor.
     */
    Status read(uint32_t& co2);

    /** Initialize chip and start measurement/ conversion cycle.
     */
    void init_chip(void);

protected:
    enum Command {
        CMD_START_MEASUREMENT   = 0x0010,
        CMD_STOP_MEASUREMENT    = 0x0104,
        CMD_SET_INTERVAL        = 0x4600,
        CMD_GET_DATA_STATUS     = 0x0202,
        CMD_READ_MEASUREMENT    = 0x0300,
        CMD_SET_ASC             = 0x5306,
        CMD_SET_FRC             = 0x5204,
        CMD_SET_TEMP_OFFSET     = 0x5403,
        CMD_SET_ALTITUDE        = 0x5102,
        CMD_READ_FW_VER         = 0xD100,
        CMD_SOFT_RESET          = 0xD304
    };

protected:
    Status _write_command(Command command, uint16_t *args, uint8_t num_args);
    Status _read_command(Command command, uint16_t *data, uint8_t num_words);

protected:
    I2C&        _i2c;
    uint8_t     _address;
};


#endif // SCD30_DRIVER_H_

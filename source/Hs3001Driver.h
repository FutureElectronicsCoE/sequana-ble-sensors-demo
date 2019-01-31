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

#ifndef HS3001_DRIVER_H_
#define HS3001_DRIVER_H_

#include <stdint.h>
#include <Sensor.h>


/** Driver for HS3001 temperature/humidity sensor.
 */
class Hs3001Driver {
public:
    enum Status {
        STATUS_OK = 0,
        STATUS_STALLED,
        STATUS_NOT_READY
    };

public:
    /** Create and initialize driver.
     *
     * @param bus I2C bus to use for communication
     * @param address I2C address to use
     */
    Hs3001Driver(I2C& bus, uint8_t address) : _i2c(bus), _address(address){};

    /** Read measured values from sensor.
     */
    Status read(uint16_t& hudmity, int16_t& temperature);

    /** Start next measurement/ conversion cycle.
     */
    void start_conversion(void);

protected:
    I2C&        _i2c;
    uint8_t     _address;
};


#endif // HS3001_DRIVER_H_

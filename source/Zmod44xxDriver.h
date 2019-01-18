/*
 * Copyright (c) 2017-2018 Future Electronics
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

#ifndef ZMOD44XX_DRIVER_H_
#define ZMOD44XX_DRIVER_H_

#include <mbed.h>

class Zmod44xxDriver {
public:
    enum Status {
        STATUS_OK = 0,
        STATUS_STALLED,
        STATUS_NOT_READY
    };

public:
    Zmod44xxDriver(I2C& bus, uint8_t addr) : _i2c(bus), _addr(addr) {};

    Status read(uint32_t &tvoc, uint32_t &eco2, float &iaq);

    void init_chip(void);

protected:
    I2C&        _i2c;
    uint8_t     _addr;
};


#endif // ZMOD44XX_DRIVER_H_

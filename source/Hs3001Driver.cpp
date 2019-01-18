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

#include <mbed.h>
#include "Hs3001Driver.h"


Hs3001Driver::Status Hs3001Driver::read(uint16_t& humidity, int16_t& temperature)
{
    static uint8_t buffer[4];
    uint32_t val;

    _i2c.read(_address, reinterpret_cast<char*>(buffer), 1);

    if ((buffer[0] & 0xC0) == 0x40) {
        return STATUS_STALLED;
    }

    _i2c.read(_address, reinterpret_cast<char*>(buffer), 4);
	val = (((uint32_t)buffer[0] & 0x3f) << 8) | (uint32_t)buffer[1];
    humidity = (uint16_t)((val * 100 + 0x2000) / 0x3fff);

    val = (((uint32_t)buffer[2]) << 6) | (((uint32_t)buffer[3]) >> 2);
    temperature = (int16_t)(val * 16500 / 0x3fff) - 4000;

//    printf("hs3001: raw=%08lx, h=0x%04x, t=0x%04x\n", *reinterpret_cast<uint32_t*>(buffer), humidity, temperature);
    return STATUS_OK;
}

void Hs3001Driver::start_conversion(void)
{
    static char buffer = 0;
     _i2c.write(_address, &buffer, 1);
}

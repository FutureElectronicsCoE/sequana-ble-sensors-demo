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
#include "Zmod44xxDriver.h"

// This is just a temporary stub driver.

Zmod44xxDriver::Status Zmod44xxDriver::read(uint32_t &tvoc, uint32_t &eco2, float &iaq)
{
    tvoc = 1.0;
    eco2 = 200;
    iaq = 1.0;

    return STATUS_OK;
}

void Zmod44xxDriver::init_chip(void)
{
}


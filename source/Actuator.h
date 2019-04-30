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

#ifndef ACTUATOR_H_
#define ACTUATOR_H_

#include "mbed.h"
#include "Sensor.h"

/** Actuator interface linking actuator BLE characteristic and actuator
 * implementation (driver).
 * Actuator is an extension of sensor (see Sensor.h) that reacts to client
 * setting (writing) it's characteristic value.
 *
 * @param ValueT Type representing actuator value. Generally should match
 *               characteristic value data, but conversion also can be
 *               implemented.
 */
template <typename ValueT> class Actuator : public Sensor<ValueT>
{
public:
    /** Create and initialize actuator interface.
     */
    Actuator() {}

    /** Set new value to the actuator.
     *
     * @value - new value to be set
     * @returns 0 when success, (-1) when not
     */
    virtual int set_value(ValueT& value) = 0;

};


#endif // ACTUATOR_H

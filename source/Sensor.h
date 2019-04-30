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

#ifndef SENSOR_H_
#define SENSOR_H_

#include "mbed.h"

/** Sensor interface linking sensor BLE characteristic and sensor
 * implementation (driver).
 *
 * @param ValueT Type representing sensor value. Generally should match
 *               characteristic value data, but conversion can also be
 *               implemented.
 */
template <typename ValueT> class Sensor
{
public:
    /** Create and initialize sensor interface.
     */
    Sensor() {}

    /** Get current value of the sensor.
     *
     * Assumption is that the sensor object caches the last value
     * recently updated.
     *
     * @returns Current sensor value.
     */
    const ValueT& get_value()
    {
        return _value;
    }

    /** Register characteristic update method.
     *
     * This method will be called every time the sensor value changes
     * and it's supposed to update the BEL characteristic based
     * on the current sensor value.
     *
     * @param func Callback function called by sensor to update its characteristic value.
     */
    void register_updater(Callback<void()> func)
    {
        _on_update = func;
    }

    /** Start sensor updates.
     *
     * This method is supposed to create and start sensor updating
     * algorithm, depending on sensor requirements.
     *
     * @param ev_queue Event queue to schedule sensor events on.
     */
    virtual void start(EventQueue& ev_queue) = 0;

protected:
    void update_value(ValueT& value)
    {
        _value = value;
        if (_on_update) {
            _on_update();
        }
    }

protected:
    ValueT              _value;
    Callback<void()>    _on_update;
};


#endif // SENSOR_H

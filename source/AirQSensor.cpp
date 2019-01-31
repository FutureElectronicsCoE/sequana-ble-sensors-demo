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

#include <mbed.h>
#include "AirQSensor.h"

using namespace sequana;

/** Callback function periodically updating sensor value.
 */
void AirQSensor::updater()
{
    AirQValue val = get_value();
    bool update = false;
    uint8_t iaq;

    if (_zmod_driver.read(val.tvoc, val.eco2, iaq) == Zmod44xxDriver::STATUS_OK) {
        update = true;
    };

    if (_scd_driver.read(val.co2) == Scd30Driver::STATUS_OK) {
        update = true;
    };

    if (update) {
        update_value(val);
    }
}


/** Initialize driver and setup periodic sensor updates.
 */
void AirQSensor::start(EventQueue& ev_queue)
{
    _zmod_driver.init_chip();
    _scd_driver.init_chip();
    ev_queue.call_every(2400, callback(this, &AirQSensor::updater));
}

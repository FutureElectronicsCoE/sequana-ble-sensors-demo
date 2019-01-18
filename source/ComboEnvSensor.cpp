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
#include "ComboEnvSensor.h"
#include "As7261Driver.h"
#include "Hs3001Driver.h"


using namespace sequana;

/** Callback function periodically updating sensor value.
 */
void ComboEnvSensor::updater()
{
    ComboEnvValue val;
    bool update = true;
    uint32_t temp;

    val.noise = 0;  // not supported yet

    if (_as_driver.read(val.ambient_light, temp) != As7261Driver::STATUS_OK) {
        update = false;
    };
    val.color_temp = temp;

    if (_hs_driver.read(val.humidity, val.temperature) != Hs3001Driver::STATUS_OK) {
        update = false;
    };

    if (update) {
        update_value(val);
    }
    _hs_driver.start_conversion();
}


/** Initialize driver and setup periodic sensor updates.
 */
void ComboEnvSensor::start(EventQueue& ev_queue)
{
    _as_driver.init_chip();
    _hs_driver.start_conversion();
    ev_queue.call_every(1000, callback(this, &ComboEnvSensor::updater));
}

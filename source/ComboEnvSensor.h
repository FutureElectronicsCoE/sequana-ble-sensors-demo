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
#ifndef COMBO_ENV_SENSOR_H_
#define COMBO_ENV_SENSOR_H_

#include <mbed.h>
#include "Sensor.h"
#include "As7261Driver.h"
#include "Hs3001Driver.h"
#include "NoiseLevelDriver.h"

namespace sequana {

/** Represents measurement result received from multiple environmental sensors
 * and presented using a single Sequana characteristic.
 *
 * This definition does not match characteristic binary data definition
 * and needs converter to be implemented.
 */
struct ComboEnvValue {
    int16_t     temperature;    //<! temperature
    uint16_t    humidity;       //<! relative humidity
    uint32_t    ambient_light;  //<! ambient light level
    uint16_t    color_temp;     //<! light temperature
    uint16_t    noise;          //<! noise level
    uint8_t     color_red;      //<! light red component
    uint8_t     color_green;    //<! light green component
    uint8_t     color_blue;     //<! light blue component
};


/** Sequana combo environmental sensor interface.
*/
class ComboEnvSensor : public Sensor<ComboEnvValue> {
public:
    ComboEnvSensor(I2C &i2c, uint32_t as_addr, uint32_t hs_addr, PinName pdm_data, PinName pdm_clk) :
        _as_driver(i2c, as_addr),
        _hs_driver(i2c, hs_addr),
        _pdm_driver(pdm_data, pdm_clk)
    {}

    virtual void start(EventQueue& ev_queue);

protected:
    void updater();
    As7261Driver _as_driver;
    Hs3001Driver _hs_driver;
    NoiseLevelDriver _pdm_driver;
};


} //namespace

#endif // COMBO_ENV_SENSOR_H_

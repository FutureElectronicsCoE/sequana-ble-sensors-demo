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
#ifndef AIR_Q_SENSOR_H_
#define AIR_Q_SENSOR_H_

#include <mbed.h>
#include "Sensor.h"
#include "Zmod44xxDriver.h"
#include "Scd30Driver.h"


namespace sequana {

/** Represents measurement result received from air quality sensors
 * and presented using a single Sequana characteristic.
 *
 * This definition does not match characteristic binary data definition
 * and needs converter to be implemented.
 */
struct AirQValue {
    uint32_t    tvoc;       //<! total volatile organic compounds
    uint32_t    eco2;       //<! estimated CO2 level
    uint32_t    co2;        //<! measured CO2 level
};


/** Sequana air quality sensor interface.
 *
 * For now it's a stub only.
 */
class AirQSensor : public Sensor<AirQValue> {
public:
    AirQSensor(I2C &i2c, uint32_t zmod_addr, DigitalOut &zmod_reset, uint32_t scd_addr) :
        _zmod_driver(i2c, (uint8_t)zmod_addr, zmod_reset),
        _scd_driver(i2c, (uint8_t)scd_addr)
    {}

    virtual void start(EventQueue& ev_queue);

protected:
    void updater();
    Zmod44xxDriver  _zmod_driver;
    Scd30Driver     _scd_driver;
};


} //namespace

#endif // AIR_Q_SENSOR_H_

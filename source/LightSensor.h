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
#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

#include <mbed.h>
#include "Actuator.h"
#include "As7261Driver.h"


namespace sequana {

/** Represents sensor configuration interface.
 *
 */
struct LightSensorCalibrationValue {
public:
public:
    uint8_t     command;                //<! r/w calibration command
    uint8_t     backlight;              //<! r/w backlight on/off
    uint8_t     state;                  //<! r/o current state
};


/** Sequana light parameters sensor calibration interface.
 *
 * It allows BLE clients to request/perform sensor calibration
 * and control state of the backlight LEDs.
 */
class LightSensor : public Actuator<LightSensorCalibrationValue> {
public:
    LightSensor(I2C &i2c, uint32_t as_addr) :
        _as_driver(i2c, as_addr)
    {
    }

    virtual void    start(EventQueue& ev_queue);
    virtual int     set_value(LightSensorCalibrationValue& value);

    /** This is extended interface allowing Env Sensor to obtain
     *  light measurement readings.
     */
    int read_light(uint32_t &lux, uint32_t &cct, uint8_t &red, uint8_t &green, uint8_t &blue)
    {
        if (_as_driver.read(lux, cct, red, green, blue ) == As7261Driver::STATUS_OK) {
            _as_driver.start_measurement();
            return 0;
        } else {
            _as_driver.start_measurement();
            return (-1);
        }
    }

protected:
    void updater();

protected:
    As7261Driver _as_driver;

};


} //namespace

#endif // LIGHT_SENSOR_H_

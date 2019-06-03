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
#include "LightSensor.h"

using namespace sequana;


/* Default conversion matrix values */
static const double default_color_conv[3][3] = {
//
//     1.756297, 0.253231, -0.657701,
//    -9.070800, 12.06207, -1.610928,
//    -0.735217, 0.987702, 0.319168
//*/
//
//     2.3638081, -0.8676030, -0.4988161
//    -0.5005940,  1.3962369,  0.1047562,
//     0.0141712, -0.0306400,  1.2323842
//
//
//     2.3706743, -0.9000405, -0.4706338,
//    -0.5138850,  1.4253036,  0.0885814,
//     0.0052982, -0.0146949,  1.0093968
//
    3.240479, -1.537150, -0.498535,
    -0.969256,  1.875992,  0.041556,
    0.055648, -0.204043,  1.057311
};


/** Initialize driver and setup periodic sensor updates.
 */
void LightSensor::start(EventQueue& ev_queue)
{
    _as_driver.set_conversion_matrix(default_color_conv);
    _as_driver.init_chip();
    _as_driver.start_measurement();
}

int LightSensor::set_value(LightSensorCalibrationValue& value)
{
    As7261Driver::Status status;

    _value = value;
    //printf("RGB color received (R/G/B): %u, %u, %u\n", value.color.r, value.color.g, value.color.b);

    // Set backlight mode.
    status = _as_driver.led_on(value.backlight);
    if (status == As7261Driver::STATUS_OK) {
        _value.backlight = (value.backlight != 0);
    }

    _value.command = value.command;
    status = _as_driver.calibrate(value.command);
    _value.state = _as_driver.get_calibration_state();
    // Make sure status gets passed to client.
    update_notify();

    return (status == As7261Driver::STATUS_OK)? 0 : (-1);
}


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
#include "RGBLedActuator.h"

using namespace sequana;

const uint32_t RGBLedActuator::NUM_LEDS = 8;


/** Callback function periodically updating LEDs.
 *  We use a simple algorithm here, where we first turn on
 * the LEDs sequentially, starting from the first one.
 * Then we sequentially turn them all off.
 */
void RGBLedActuator::updater()
{
    BGR24_color_t   current_color; // by default black

    if (_current_led == NUM_LEDS) {
        // starting a new cycle
        _turning_on = !_turning_on;
        _current_led = 0;
    }

    if (_turning_on) {
        current_color = get_value().color;
    }

//    printf("RGB LED(%u): %u, %u, %u\n", _current_led, current_color.r, current_color.g, current_color.g);
    _leds.set_color(_current_led++, current_color);
    _leds.refresh();
}


/** Initialize driver and setup periodic sensor updates.
 */
void RGBLedActuator::start(EventQueue& ev_queue)
{
    ev_queue.call_every(400, callback(this, &RGBLedActuator::updater));
}

int RGBLedActuator::set_value(RGBLedValue& value)
{
    _value = value;
    printf("RGB color received (R/G/B): %u, %u, %u\n", value.color.r, value.color.g, value.color.b);

    // Now set the new color for all LEDs that are on.
    if (_turning_on) {
        // Set color of in-phase LEDs
        for (int i = 0; i < _current_led; ++i) {
            _leds.set_color(i, value.color);
        }
    } else {
        // Set color of out-of-phase LEDs
        for (int i = _current_led; i < NUM_LEDS; ++i) {
            _leds.set_color(i, value.color);
        }
    }
    _leds.refresh();

    return 0;
}

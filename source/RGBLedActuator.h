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
#ifndef RGB_LED_ACTUATOR_H_
#define RGB_LED_ACTUATOR_H_

#include <mbed.h>
#include "Actuator.h"
#include "BD2808.h"


namespace sequana {

/** Represents RGB LED actuator value and required conversions.
 *
 */
struct RGBLedValue {
    BGR24_color_t   color;  //<! current color

    RGBLedValue(const uint8_t *data) :
        color(data[5],  // blue/256
              data[3],  // green/256
              data[1])  // red/256
    {}

    RGBLedValue() {}
};


/** Sequana RGB LED actuator interface.
 *
 * It allows BLE clients to manipulate the color of RGB LEDs.
 */
class RGBLedActuator : public Actuator<RGBLedValue> {
protected:
    static const uint32_t NUM_LEDS;
public:
    RGBLedActuator() :
        _turning_on(false),
        _current_led(NUM_LEDS)
    {
        _leds.set_dma_usage(DMA_USAGE_ALWAYS);
    }

    virtual void    start(EventQueue& ev_queue);
    virtual int     set_value(RGBLedValue& value);

protected:
    void updater();
    BD2808  _leds;
    bool    _turning_on;
    uint8_t _current_led;
};


} //namespace

#endif // RGB_LED_ACTUATOR_H_

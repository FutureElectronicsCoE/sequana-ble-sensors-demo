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

#ifndef OCCUPANCY_SENSOR_H_
#define OCCUPANCY_SENSOR_H_

#include <stdint.h>
#include <mbed.h>
#include <Sensor.h>

/** Driver and sensor implementation for PIR-based occupancy sensor.
 */

// Inter-sample delay in milliseconds.
#define PIR_SAMPLING_DELAY_MS       4

// Decimation rate for sampling.
// Samples are in-flight decimated by a moving average filter.
// Real sampling occurs at 1 / PIR_SAMPLING_DELAY frequency (oversampling)
// so we can have only a simple anti-alias filter in hardware.
#define PIR_DECIMATION_RATE         9

// Effective sampling frequency calculated from the above is 27.78 Hz
// Notice, that the filter coefficients has been calculated assuming this frequency.


// Number of samples in a single buffer. This determined buffer processing rate
// and the reaction latency
#define PIR_BUFFER_SIZE             15
#define NUM_PIR_BUFFERS             4

// Moving average filter to set up reference level value.
#define PIR_AVERAGE_OVER_BUFFERS    40      // i.e. over 20 seconds

// Detection timer, in terms of buffer processing frequency,
// defines how long detection signal is kept triggered.
#define PIR_DETECTION_TIMEOUT       6       // 3 seconds.


class PirDriver {
    friend class Samper;

public:
    enum Status {
        STATUS_OK = 0,
        STATUS_NOT_READY
    };

public:
    /** Create and initialize driver.
     *
     * @param adc_pin ADC pin where the PIR sensor is connected.
     * @param led_pin LED to use to signal activation (NC for none).
     */
    PirDriver(PinName adc_pin, PinName led_pin);

    /** Read measured value from sensor.
     */
    Status read(uint8_t& occupancy);

    /** Initialize everything and start measurement cycle.
     */
    void start_measurement(void);

protected:
    struct pir_buffer_t
    {
        float buff[PIR_BUFFER_SIZE];
    };

    struct IIRHistory
    {
        float w1;
        float w2;

        IIRHistory() : w1(0.0), w2(0.0) {}
    };

    struct IIRCoefficients
    {
        float b0, b1, b2;
        float a1, a2;
    };

    class Sampler
    {
    public:
        Sampler(PirDriver& drv) :
            _drv(drv),
            _current_buffer(NULL),
            _avg_level(0),
            _avg_index(0)
        {
        };

        void start_sampling();

    protected:
        void _sampler_thread_func();

    protected:
        PirDriver       &_drv;
        Thread          _thread;
        pir_buffer_t    *_current_buffer;
        uint32_t        _current_index;
        uint32_t        _avg_level;
        uint32_t        _avg_index;
    };

    static const IIRCoefficients filter_phase1;
    static const IIRCoefficients filter_phase2;

protected:
    void    _start_reading();
    void    _filter_biquad(const IIRCoefficients &coef, const float *input, float *output, uint32_t count);
    void    _filter(pir_buffer_t &buffer);
    bool    _preprocess(pir_buffer_t &buffer);
    void    _detect_occupancy(const pir_buffer_t &buffer);
    void    _data_processing_thread_func(void);

protected:
    AnalogIn                                    _adc;
    DigitalOut                                  *_p_led;
    Thread                                      _thread;
    Sampler                                     _sampler;
    volatile uint8_t                            _occupancy;
    uint16_t                                    _detection_timer;
    MemoryPool<pir_buffer_t, NUM_PIR_BUFFERS>   _buffer_pool;
    Queue<pir_buffer_t, NUM_PIR_BUFFERS>        _buffer_queue;
    float                                       _avg_level;
    float                                       _peak_level;
    float                                       _avg_level_sum;
    bool                                        _avg_calculated;
    IIRHistory                                  _history;
    uint32_t                                    _avg_index;
    float                                       _avg_table[PIR_AVERAGE_OVER_BUFFERS];
};

/** Occupancy (PIR) sensor interface.
 */
class OccupancySensor : public Sensor<uint8_t> {
public:
    /** Creates and initializes sensor interface.
     *
     * @param adc_pin ADC pin where the PIR sensor is connected.
     * @param led_pin LED to use to signal activation (NC for none).
     */
    OccupancySensor(PinName adc_pin, PinName led_pin) : _driver(adc_pin, led_pin), _started(false) {}

    /** Schedule measurement process.
     */
    virtual void start(EventQueue& ev_queue);

protected:
    void updater();
    PirDriver _driver;
    bool _started;
};


#endif // OCCUPANCY_SENSOR_H_

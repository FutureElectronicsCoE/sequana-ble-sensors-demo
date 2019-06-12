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
#include "OccupancySensor.h"
#include "math.h"



uint32_t pir_driver_stats_overrun = 0;
uint32_t pir_driver_stats_completed = 0;
uint32_t pir_driver_stats_processed = 0;


//  IIR HP filter coefficients for a giver cut-out frequency.

const PirDriver::IIRCoefficients PirDriver::filter_phase1 = {
//    0.814254556886246, -1.628509113772492, 0.814254556886246, 1.7259333950369407, -0.7474473719077911 //0.7Hz
    0.8883399541464222, -1.7766799082928444, 0.8883399541464222, 1.838176371086801, -0.845743736722382 // 0.4Hz
//    0.9143922770043105, -1.828784554008621, 0.9143922770043105, 1.8766708464198378, -0.8810799613045457 //0.3Hz
};

const PirDriver::IIRCoefficients PirDriver::filter_phase2 = {
//    1.0, -2.0, 1.0, 1.863800492075235, -0.8870329996526946 //0.7Hz
    1.0, -2.0, 1.0, 1.9251561049402897, -0.9330815469059268 //0.4Hz
//    1.0, -2.0, 1.0, 1.9443958512096418, -0.9489640815445924 //0.3Hz
};

// Voltage thresholds and hysteresis.
// Assuming ADC has 12-bit resolution.
#define PIR_VOLTAGE_CONSTANT        (3.3 / 0xfff)
#define PIR_THRESHOLD_HYSTERESIS    (PIR_VOLTAGE_CONSTANT * 7)
#define PIR_DETECTION_THRESHOLD     (PIR_VOLTAGE_CONSTANT * 35)


void PirDriver::Sampler::start_sampling()
{
    _thread.start(callback(this, &PirDriver::Sampler::_sampler_thread_func));
}


// Sampling thread/function for ADC.
void PirDriver::Sampler::_sampler_thread_func(void)
{
    uint16_t level;
    uint64_t ms_count = rtos::Kernel::get_ms_count();
    _avg_index = 0;
    _avg_level = 0;
    _current_index = 0;
    _current_buffer = _drv._buffer_pool.alloc();
    MBED_ASSERT(_current_buffer);

    while (1) {
        ms_count += PIR_SAMPLING_DELAY_MS;
        ThisThread::sleep_until(ms_count);

        level = _drv._adc.read_u16();

        // In-flight decimation filter.
        _avg_level += level;
        if (++_avg_index >= PIR_DECIMATION_RATE) {
            _avg_index = 0;
            float val = PIR_VOLTAGE_CONSTANT / PIR_DECIMATION_RATE * _avg_level;
            _current_buffer->buff[_current_index] = val;
            _avg_level = 0;

            if (++_current_index >= PIR_BUFFER_SIZE) {
                _current_index = 0;

                if (!_drv._buffer_queue.full()) {
                    _drv._buffer_queue.put(_current_buffer);
                } else {
                    error("PirDriver: buffer queue full!");
                }
                pir_driver_stats_completed++;
                // Allocate new buffer.
                _current_buffer = _drv._buffer_pool.alloc();
                MBED_ASSERT(_current_buffer);
            }
        }
    }
}


PirDriver::PirDriver(PinName adc_pin, PinName led_pin) :
    _adc(adc_pin),
    _p_led(NULL),
    _sampler(*this),
    _occupancy(0),
    _detection_timer(0),
    _avg_level(0.0),
    _peak_level(0.0),
    _avg_level_sum(0.0),
    _avg_calculated(false),
    _avg_index(0)
{
    if (led_pin != NC) {
        _p_led = new DigitalOut(led_pin);
    }

    for (uint32_t i = 0; i < PIR_AVERAGE_OVER_BUFFERS; ++i) {
        _avg_table[i] = 0.0;
    }
}


PirDriver::Status PirDriver::read(uint8_t &occupancy)
{
    occupancy = _occupancy;
//    printf("pir detector: avg = %6.3f, peak = %6.3f, occupancy = %d\n", _avg_level, _peak_level, _occupancy);
#if 0
    printf("pir driver stats: cmpl %lu, proc %lu, ovrrn %lu\n",
           pir_driver_stats_completed,
           pir_driver_stats_processed,
           pir_driver_stats_overrun);
#endif
    return STATUS_OK;
}


// IIR filtering function
void PirDriver::_filter_biquad(const IIRCoefficients &coef, const float *input, float *output, uint32_t count)
{
    float w0;
    float w1 = _history.w1;
    float w2 = _history.w2;

    for (uint32_t i = 0; i < count; ++i) {
        w0 = input[i];

        // feedback
        w0 = w0 + w1*coef.a1 + w2*coef.a2;

        // forward
        output[i] = w0*coef.b0 + w1*coef.b1 + w2*coef.b2;

        w2 = w1;
        w1 = w0;
    }

    // save state
    _history.w1 = w1;
    _history.w2 = w2;
}


// Digital HP filter to provide required characteristic.
// Implements 4th-order high-pass IIR filter with cutoff at 0.4Hz
// Low-pass filtering is obtained in hardware (anti-aliasing at 26Hz)
// and by decimation averaging over 9 samples.
void PirDriver::_filter(pir_buffer_t &buffer)
{
    _filter_biquad(filter_phase1, buffer.buff, buffer.buff, PIR_BUFFER_SIZE);
    _filter_biquad(filter_phase2, buffer.buff, buffer.buff, PIR_BUFFER_SIZE);
}


// Preprocessing removes DC offset from the buffer.
// This function also calculates this DC offset by averaging value
// over the last PIR_AVERAGE_OVER_BUFFERS buffers.
bool PirDriver::_preprocess(pir_buffer_t &buffer)
{
    bool result = false;
    float sum = 0.0;
    float level = 0.0;

    if (_avg_calculated) {
        for (uint32_t i = 0; i < PIR_BUFFER_SIZE; ++i) {
            sum += buffer.buff[i];
            buffer.buff[i] -= _avg_level;
        }
        result = true;
    } else {
        for (uint32_t i = 0; i < PIR_BUFFER_SIZE; ++i) {
            sum += buffer.buff[i];
        }
    }
    level = sum / PIR_BUFFER_SIZE; // average

    // Update moving average filter.
    _avg_level_sum -= _avg_table[_avg_index];
    _avg_table[_avg_index] = level;
    _avg_level_sum += level;
    _avg_level = _avg_level_sum / PIR_AVERAGE_OVER_BUFFERS;
    if (++_avg_index >= PIR_AVERAGE_OVER_BUFFERS) {
        _avg_calculated = true;
        _avg_index = 0;
    }
    return result;
}


// To detect movement, we trigger when the absolute value of the current
// sample is greater than specified threshold. The buffer at the input
// has been preprocessed to remove DC offset.
// value and trigger when certain threshold is found.
void PirDriver::_detect_occupancy(const pir_buffer_t &buffer)
{
    // Process detection timer.
    if (_detection_timer > 0) {
        --_detection_timer;
        if (_detection_timer == 0) {
            _occupancy = 0;
            if (_p_led) {
                *_p_led = 0;
            }
        }
    }

    // Detect movement by comparing samples to average value.
    // At the same time calculate buffer average value (optimization).
    float threshold = PIR_DETECTION_THRESHOLD;
    if (_occupancy) {
        threshold -= PIR_THRESHOLD_HYSTERESIS;
    }
    for (uint32_t i = 0; i < PIR_BUFFER_SIZE; ++i) {
        float sample = buffer.buff[i];
        if (abs(sample) > threshold) {
            _peak_level = _avg_level + sample;
            _occupancy = 1;
            if (_p_led) {
                *_p_led = 1;
            }
            _detection_timer = PIR_DETECTION_TIMEOUT;
            break;
        }
    }
    // This section prints a 'scope like display on the serial terminal
    // from the preprocessed buffer values to allow adjustment/debugging
    // of the thresholds and filters.
#if 0
    for (uint32_t i = 0; i < PIR_BUFFER_SIZE; ++i) {
        int val = buffer.buff[i] * (1.0 / PIR_VOLTAGE_CONSTANT / 10);
        fputs("      ", stdout);
        for (int j = -16; j < 0; ++j) {
            int c = (val <= j)? '|' : ' ';
            putchar(c);
        }
        putchar('|');
        for (int j = 1; j <= 16; ++j) {
            int c = (val >= j)? '|' : ' ';
            putchar(c);
        }
        printf("  %6.3f    %c", buffer.buff[i], (i == (PIR_BUFFER_SIZE-1))? ' ' : '\n');
    }
    printf("avg = %6.3f, peak = %6.3f, occ = %d\n", _avg_level, _peak_level, _occupancy);
#endif
}


void PirDriver::start_measurement(void)
{
    _thread.start(callback(this, &PirDriver::_data_processing_thread_func));
    _sampler.start_sampling();
}


void PirDriver::_data_processing_thread_func()
{
    pir_buffer_t *buffer = NULL;

    while (true) {
        buffer = static_cast<pir_buffer_t *>(_buffer_queue.get().value.p);
        MBED_ASSERT(buffer);
        pir_driver_stats_processed++;
        if (_preprocess(*buffer)) {
            _filter(*buffer);
            _detect_occupancy(*buffer);
        }
        _buffer_pool.free(buffer);
    }
}


/** Callback function periodically updating sensor value.
 */
void OccupancySensor::updater()
{
    if (_started) {
        if (_driver.read(_value) == PirDriver::STATUS_OK) {
            update_notify();
        }
    } else {
        _driver.start_measurement();
        _started = true;
    }
}


/** Initialize driver and setup periodic sensor updates.
 */
void OccupancySensor::start(EventQueue& ev_queue)
{
    ev_queue.call_every(1000, callback(this, &OccupancySensor::updater));
}


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

#ifndef NOISE_LEVEL_DRIVER_H_
#define NOISE_LEVEL_DRIVER_H_

#include <stdint.h>
#include <Sensor.h>
#include <PDMAudio.h>

/** Driver for PDM Microphone (STM MP34DT05) audio noise sensor.
 */

// Number of audio samples in a single audio buffer.
// This determines how often data thread will be waked up, given constant
// data rate of 32000 sps, e.g. 512 -> every 16ms
#define AUDIO_BUFFER_SIZE   512
#define NUM_AUDIO_BUFFERS   4


class NoiseLevelDriver {
public:
    enum Status {
        STATUS_OK = 0,
        STATUS_NOT_READY
    };

public:
    /** Create and initialize driver.
     *
     * @param dat I/O pin to use as serial PDM data line
     * @param clk I/O pin to use as PDM data clock
     */
    NoiseLevelDriver(PinName dat, PinName clk);

    /** Read measured value from sensor.
     */
    Status read(uint16_t& noise);

    /** Initialize everything and start measurement cycle.
     */
    void start_measurement(void);

protected:
    typedef int32_t audio_buffer_t[AUDIO_BUFFER_SIZE];

protected:
    void        _start_reading();
    void        _rx_done(int event);
    void        _filter(audio_buffer_t &buffer);
    uint32_t    _peak_detector(audio_buffer_t &buffer);
    void        _data_processing_thread_func(void);

protected:
    PDMAudio                                        _pdm_audio;
    Thread                                          _thread;
    volatile uint32_t                               _audio_level;
    MemoryPool<audio_buffer_t, NUM_AUDIO_BUFFERS>   _buffer_pool;
    Queue<audio_buffer_t, NUM_AUDIO_BUFFERS>        _buffer_queue;
    audio_buffer_t                                  *_current_buffer;
};


#endif // NOISE_LEVEL_DRIVER_H_

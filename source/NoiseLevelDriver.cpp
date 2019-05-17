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
#include "NoiseLevelDriver.h"
#include "math.h"

// This is reference level to scale audio level into dB scale
#define REFERENCE_LEVEL     (4.19e6)

// FADING_CONSTANT implements digital RC constant of the peak detector.
// Keep it as proper fraction without parentheses to avoid invalid calculations
#define FADING_CONSTANT     123 / 128



NoiseLevelDriver::NoiseLevelDriver(PinName dat, PinName clk) :
    _pdm_audio(dat, clk),
    _audio_level(0),
    _current_buffer(NULL)
{

}

NoiseLevelDriver::Status NoiseLevelDriver::read(uint16_t &noise_level)
{
    // Weight off current level and convert into logarithmic scale.
    float level = _audio_level;

    noise_level = 20.0 * log(level / REFERENCE_LEVEL);
    return STATUS_OK;
}

void NoiseLevelDriver::start_measurement(void)
{
    _thread.start(callback(this, &NoiseLevelDriver::_data_processing_thread_func));
}


// Allocate new buffer and pass it to audio driver.
void NoiseLevelDriver::_start_reading()
{
    int events = PDM_AUDIO_EVENT_RX_COMPLETE | PDM_AUDIO_EVENT_OVERRUN | PDM_AUDIO_EVENT_DMA_ERROR;

    _current_buffer = _buffer_pool.alloc();
    MBED_ASSERT(_current_buffer);
    if (_current_buffer) {
        _pdm_audio.read(*_current_buffer, AUDIO_BUFFER_SIZE, callback(this, &NoiseLevelDriver::_rx_done), events);
    }
}

// Receive complete event handler
void NoiseLevelDriver::_rx_done(int event)
{
    if (event == PDM_AUDIO_EVENT_RX_COMPLETE) {
        MBED_ASSERT(_current_buffer);
        // Give the currently received buffer to the data processing thread.
        if (!_buffer_queue.full()) {
            _buffer_queue.put(_current_buffer);
        } else {
            error("NoiseLevelDriver: buffer queue full!");
        }
        // Continue with next buffer.
        _start_reading();
    } else {
        // error
        _shdlc_state = ERROR;
        _status = STATUS_RX_ERROR;
        _read_in_progress = false;
    }
}

// Digital filter to provide required audio characteristic.
// Right now it's empty function, we only filter off some low
// level frequency using PDM-PCM hardware unit built-in HPF.
void NoiseLevelDriver::_filter(audio_buffer_t &buffer)
{
    // Nothing to do right now.
}

// To obtain noise level we just peak-detect audio signal level
// and scale result properly.
uint32_t NoiseLevelDriver::_peak_detector(audio_buffer_t &buffer)
{
    int32_t sample;
    uint32_t level = 0;
    uint32_t i, temp;
    // First find the peak within current buffer.
    for (i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
        sample = buffer[i];
        sample = sample > 0 ? sample : -sample;
        level = static_cast<uint32_t>(sample) > level ? sample : level;
    }

    // Fade current value.
    temp = _audio_level * FADING_CONSTANT;
    // Return peak value.
    return (level > temp ? level : temp);
}


void NoiseLevelDriver::_data_processing_thread_func()
{
    audio_buffer_t *buffer = NULL;

    // Initialize stream reading with a first buffer.
    _start_reading();

    while (true) {
        buffer = static_cast<audio_buffer_t *>(_buffer_queue.get().value.p);
        MBED_ASSERT(buffer);
        _filter(*buffer);
        _audio_level = _peak_detector(*buffer);
        _buffer_pool.free(buffer);
    }

}

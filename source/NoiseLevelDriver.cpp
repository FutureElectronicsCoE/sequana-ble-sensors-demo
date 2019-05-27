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
// assuming mic acoustic overload level of 122.5 dB
#define REFERENCE_LEVEL     (6.29057)   // (2^23 / (10^(log(122.5/20))))
//#define REFERENCE_LEVEL     (524.288)

// FADING_CONSTANT implements digital RC constant of the peak detector.
// Keep it as proper fraction without parentheses to avoid invalid calculations
#define FADING_CONSTANT     125 / 128



uint32_t noise_level_driver_stats_overrun = 0;
uint32_t noise_level_driver_stats_dma_err = 0;
uint32_t noise_level_driver_stats_completed = 0;
uint32_t noise_level_driver_stats_processed = 0;


// FIR LP filter
#define TAP_NUM         9
// We exploit filter characteristic being symmetrical
// over digital time.
#define HALF_TAP_NUM    (TAP_NUM / 2 + 1)

static const double filter_taps_input[HALF_TAP_NUM] = {
    0.08508778500402367,
    -0.00599282842058143,
    -0.1270449138428389,
    0.28479707451843517,
    0.6445031274185362
};

static int32_t filter_taps[HALF_TAP_NUM];
static int64_t fir_window[TAP_NUM][HALF_TAP_NUM];
static uint32_t hist_index;


NoiseLevelDriver::NoiseLevelDriver(PinName dat, PinName clk) :
    _pdm_audio(dat, clk),
    _audio_level(0),
    _current_buffer(NULL),
    _avg_level(0),
    _avg_index(0)
{
    hist_index = 0;
    for (uint32_t j = 0; j < HALF_TAP_NUM; ++j) {
        for (uint32_t i = 0; i < TAP_NUM; ++i) {
            fir_window[i][j] = 0;
        }
        filter_taps[j] = (int32_t)(filter_taps_input[j] * 2147483647.0 + 0.5);
    }
    for (uint32_t i = 0; i < AVERAGE_OVER_BUFFERS; ++i) {
        _avg_table[i] = 0;
    }
}

#include "cy_pdm_pcm.h"
#include "cy_dma.h"

NoiseLevelDriver::Status NoiseLevelDriver::read(uint16_t &noise_level)
{
    // Weight off current level and convert into logarithmic scale.
    float level = _audio_level;
    noise_level = 20.0 * log(level / REFERENCE_LEVEL) / log(10.0) + 0.5;

    printf("noise level: raw audio = %f, level = %u\n", (double)level, noise_level);
    printf("noise level: stats: cmpl %lu, proc %lu, ovrrn %lu, err %lu\n",
           noise_level_driver_stats_completed,
           noise_level_driver_stats_processed,
           noise_level_driver_stats_overrun,
           noise_level_driver_stats_dma_err);

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
        _pdm_audio.read(_current_buffer->buff, AUDIO_BUFFER_SIZE, callback(this, &NoiseLevelDriver::_rx_done), events);
    }
}

// Receive complete event handler
void NoiseLevelDriver::_rx_done(int event)
{
    if (event & PDM_AUDIO_EVENT_RX_COMPLETE) {
        MBED_ASSERT(_current_buffer);
        // Give the currently received buffer to the data processing thread.
        if (!_buffer_queue.full()) {
            _buffer_queue.put(_current_buffer);
        } else {
            error("NoiseLevelDriver: buffer queue full!");
        }
        noise_level_driver_stats_completed++;
    }

    // error
    if (event & PDM_AUDIO_EVENT_OVERRUN ){
        noise_level_driver_stats_overrun++;
    }
    if (event & PDM_AUDIO_EVENT_DMA_ERROR){
        noise_level_driver_stats_dma_err++;
        _pdm_audio.abort_read();
    }
    // Continue with next buffer.
    _start_reading();
}


static void filter_put(int32_t sample) {
    hist_index = ++hist_index >= TAP_NUM ? 0 : hist_index;

    for (uint32_t i = 0; i < HALF_TAP_NUM; ++i) {
        fir_window[hist_index][i] = (int64_t)sample * filter_taps[i];
    }
}


static int32_t filter_get() {
    int64_t acc = 0;
    uint32_t index = hist_index;

    for (uint32_t i = 0; i < HALF_TAP_NUM; ++i) {
        acc += fir_window[index][i];
        index = --index > TAP_NUM ? TAP_NUM - 1 : index;
    }
    for (int32_t i = HALF_TAP_NUM - 1; i >= 0; --i) {
        acc += fir_window[index][i];
        index = --index > TAP_NUM ? TAP_NUM - 1 : index;
    }
    return (int32_t)(acc / 2147483647LL);
}


// Digital filter to provide required audio characteristic.
// Implements 9-tap low-pass FIR filter flat to 9kHz
// and -18dB @ 12kHz.
// Right now it's empty function, we only filter off some low
// level frequency using PDM-PCM hardware unit built-in HPF.
void NoiseLevelDriver::_filter(audio_buffer_t &buffer)
{
    int32_t sample;
    for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
        sample = buffer.buff[i];
        filter_put(sample);
        buffer.buff[i] = filter_get();
    }
}

// To obtain noise level we just average audio signal level
// over the last N buffers and then peak-detect it over last M buffers
// (M depending on the fading constant value).
// The result is then scaled properly.
uint32_t NoiseLevelDriver::_peak_detector(audio_buffer_t &buffer)
{
    int32_t sample;
    uint64_t sum = 0;
    uint32_t level = 0;
    uint32_t i, temp;
    // First find average level over current buffer.
    for (i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
        sample = buffer.buff[i];
        sample = sample > 0 ? sample : -sample;
        sum += sample;
    }

    level = (uint32_t)(sum / AUDIO_BUFFER_SIZE); // average
    _avg_level -= _avg_table[_avg_index];
    _avg_table[_avg_index] = level;
    _avg_level += level;
    _avg_index = ++_avg_index >= AVERAGE_OVER_BUFFERS ? 0 : _avg_index;
    level = _avg_level / AVERAGE_OVER_BUFFERS;
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
        noise_level_driver_stats_processed++;
        _filter(*buffer);
        _audio_level = _peak_detector(*buffer);
        _buffer_pool.free(buffer);
    }
}

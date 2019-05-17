/*
 * Low level Mbed OS driver for PSoC 6 PDM-PCM converter.
 *
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
#ifndef __PDM_AUDIO_API_H__
#define __PDM_AUDIO_API_H__

#include <stdint.h>
#include <cmsis.h>
#include "objects.h"
#include "hal/buffer.h"
#include "hal/dma_api.h"

typedef enum {
    PDM_STATE_IDLE = 0,
    PDM_STATE_STOPPED,
    PDM_STATE_STREAMING,
    PDM_STATE_RECEIVING
} pdm_stream_state_t;


#define PDM_AUDIO_EVENT_RX_COMPLETE     (1 << 0)
#define PDM_AUDIO_EVENT_STREAM_STOPPED  (1 << 4)
#define PDM_AUDIO_EVENT_OVERRUN         (1 << 8)
#define PDM_AUDIO_EVENT_DMA_ERROR       (1 << 9)


#ifdef __cplusplus
extern "C" {
#endif

struct pdm_audio_s {
    PDM_Type                            *base;
    PinName                             pin_data_in;
    PinName                             pin_clk;
    uint32_t                            actual_speed;
    uint8_t                             dma_unit;
    uint8_t                             dma_channel;
    IRQn_Type                           irqn;
    uint16_t                            events;
    volatile pdm_stream_state_t         state;
    volatile uint32_t                   num_scheduled;
    uint32_t                            current_scheduled;
    uint32_t                            current_completed;
#if DEVICE_SLEEP && DEVICE_LPTICKER
    cy_stc_syspm_callback_params_t      pm_callback_params;
    cy_stc_syspm_callback_t             pm_callback_handler;
#endif
};


/** Asynch PDM AUDIO HAL structure
 */
typedef struct {
    struct pdm_audio_s  pdm;        /**< Target specific PDM AUDIO structure */
    struct buffer_s     rx_buff;    /**< Rx buffer */
} pdm_audio_t;


void pdm_audio_init(pdm_audio_t* obj, PinName clk, PinName data_in);

void pdm_audio_free(pdm_audio_t *obj);

/** Begin asynchronous receive of audio data (enable interrupt for data collecting)
 *
 * @param obj        The pdm audio object
 * @param rx         The receive buffer
 * @param count      The number of 32-bit words to receive
 * @param handler    The pdm audio interrupt handler
 * @param event      The logical OR of events to be registered
 * @param hint       A suggestion for how to use DMA with this transfer
 */
void pdm_audio_rx_async(pdm_audio_t *obj, int32_t *rx_buffer, uint32_t count, uint32_t handler, uint32_t event, DMAUsage hint);


/** The asynchronous IRQ handler
 *
 *  @param obj The pdm audio object which holds the transfer information
 *  @return Event flags if a transfer termination condition was met, otherwise return 0.
 */
uint32_t pdm_audio_irq_handler_asynch(pdm_audio_t *obj);

/** Attempts to determine if the I2C peripheral is already in use
 *
 *  @param obj The PDM AUDIO object
 *  @return Non-zero if the I2C module is active or zero if it is not
 */
uint8_t pdm_audio_active(pdm_audio_t *obj);

/** Abort asynchronous transfer
 *
 *  This function does not perform any check - that should happen in upper layers.
 *  @param obj The PDM AUDIO object
 */
void pdm_audio_abort_asynch(pdm_audio_t *obj);


#ifdef __cplusplus
}
#endif


#endif //__PDM_AUDIO_API_H__

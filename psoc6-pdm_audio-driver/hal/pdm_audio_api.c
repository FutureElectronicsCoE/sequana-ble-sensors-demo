/*
 * Low level Mbed OS driver for PSoC 6 PDM-PCM controller circuit
 * on Future Electronics Sequana board.
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

#include "cmsis.h"
#include "mbed_assert.h"
#include "mbed_error.h"
#include "mbed_debug.h"
#include "PeripheralPins.h"
#include "pinmap.h"
#include "psoc6_utils.h"
#include "hal/pdm_audio_api.h"
#include "cy_pdm_pcm.h"
#include "cy_dma.h"
#include "cy_trigmux.h"
#include "cy_sysint.h"


// PDM-PCM controller is used to interface PDM microphones.
//
// In this HAL driver we support only asynchronous interface using DMA and interrupts
// for efficiency, available in two modes:
//    * internal buffering - HAL implements double buffering using internally allocated
//      buffers (streaming mode). Application has to complete buffer processing before
//      next buffer is completely filled in.
//    * external buffering - HAL does not implement buffering and application must
//      complete data processing before hardware internal buffer is full (255 data samples).
//
// Master clock (HF1) is 49142900 Hz


////////////////////////////////////
////   LIBRARY CONFIGURATION    ////
////////////////////////////////////

#if defined (TARGET_MCU_PSOC6_M0)
    #error  "PDM-AUDIO: only supported on PSoC6 M4 core"
#endif // defined(TARGET_MCU_PSOC6_M0)

#ifndef MBED_CONF_PDM_AUDIO_CLOCK_DIVIDER
    #define MBED_CONF_PDM_AUDIO_CLOCK_DIVIDER   24      // for 2048 kHz operation
#endif // MBED_CONF_PDM_AUDIO_CLOCK_DIVIDER

#ifndef MBED_CONF_PDM_AUDIO_OVERSAMPLE
    #define MBED_CONF_PDM_AUDIO_OVERSAMPLE      64      // Fs = 32 ksps @ 2048 kHz
#endif // MBED_CONF_PDM_AUDIO_OVERSAMPLE

#ifndef MBED_CONF_PDM_AUDIO_CHANNEL_MODE
    #define MBED_CONF_PDM_AUDIO_CHANNEL_MODE    RIGHT
#endif // MBED_CONF_PDM_AUDIO_CHANNEL_MODE

#ifndef MBED_CONF_PDM_AUDIO_DATA_BITS
    #define MBED_CONF_PDM_AUDIO_DATA_BITS   24
#endif // MBED_CONF_PDM_AUDIO_DATA_BITS

#ifndef MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER
    #define MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER    8
#endif // MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER



#if MBED_CONF_PDM_AUDIO_DATA_BITS == 16
    #define PDM_AUDIO_WORD_LENGTH   CY_PDM_PCM_WLEN_16_BIT
#elif MBED_CONF_PDM_AUDIO_DATA_BITS == 18
    #define PDM_AUDIO_WORD_LENGTH   CY_PDM_PCM_WLEN_18_BIT
#elif MBED_CONF_PDM_AUDIO_DATA_BITS == 20
    #define PDM_AUDIO_WORD_LENGTH   CY_PDM_PCM_WLEN_20_BIT
#elif MBED_CONF_PDM_AUDIO_DATA_BITS == 24
    #define PDM_AUDIO_WORD_LENGTH   CY_PDM_PCM_WLEN_24_BIT
#else
    #error "PDM-AUDIO: unsupported data bits value"
#endif // MBED_CONF_PDM_AUDIO_DATA_BITS == 24


#define _PDM_AUDIO_CHANNEL_MODE(x) CY_PDM_PCM_OUT_CHAN_ ## x
#define PDM_AUDIO_CHANNEL_MODE(x)  _PDM_AUDIO_CHANNEL_MODE(x)

//#if PDM_AUDIO_CHANNEL_MODE(MBED_CONF_PDM_AUDIO_CHANNEL_MODE) == 0
//    #error "PDM-AUDIO: invalid value of channel-mode"
//#endif

#if MBED_CONF_PDM_AUDIO_OVERSAMPLE < 32 || MBED_CONF_PDM_AUDIO_OVERSAMPLE > 254
    #error "PDM-AUDIO: invalid value of oversample"
#endif // MBED_CONF_PDM_AUDIO_OVERSAMPLE

#if MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER
    #define PDM_AUDIO_HPF_DISABLE   false
#else
    #define PDM_AUDIO_HPF_DISABLE   true
#endif // MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER


#define NUM_PDM_AUDIO_INTERNAL_BUFFERS      2
#define PDM_AUDIO_IRQ_PRIORITY              2

////////////////////////////////////
////    STATIC CONFIGURATION    ////
////////////////////////////////////


const PinMap PinMap_PDM_CLK[] = {
    {P10_4, PDM0_BASE, CY_PIN_OUT_FUNCTION( P10_4_AUDIOSS_PDM_CLK, 0)},
    {P12_4, PDM0_BASE, CY_PIN_OUT_FUNCTION( P12_4_AUDIOSS_PDM_CLK, 0)},
    {NC,    NC,    0}
};
const PinMap PinMap_PDM_DATA[] = {
    {P10_5, PDM0_BASE, CY_PIN_IN_FUNCTION( P10_5_AUDIOSS_PDM_DATA, 0)},
    {P12_5, PDM0_BASE, CY_PIN_IN_FUNCTION( P12_5_AUDIOSS_PDM_DATA, 0)},
    {NC,    NC,    0}
};


static const cy_stc_pdm_pcm_config_t default_pdm_pcm_config =
{
    .clkDiv             = CY_PDM_PCM_CLK_DIV_BYPASS,
    .mclkDiv            = CY_PDM_PCM_CLK_DIV_BYPASS,
    .ckoDiv             = 0U,
    .ckoDelay           = 0U,
    .sincDecRate        = MBED_CONF_PDM_AUDIO_OVERSAMPLE / 2,
    .chanSelect         = PDM_AUDIO_CHANNEL_MODE(MBED_CONF_PDM_AUDIO_CHANNEL_MODE),
    .chanSwapEnable     = false,
    .highPassFilterGain = MBED_CONF_PDM_AUDIO_HIGH_PASS_FILTER,
    .highPassDisable    = PDM_AUDIO_HPF_DISABLE,
    .softMuteCycles     = CY_PDM_PCM_SOFT_MUTE_CYCLES_96,
    .softMuteFineGain   = 1UL,
    .softMuteEnable     = false,
    .wordLen            = PDM_AUDIO_WORD_LENGTH,
    .signExtension      = true,
    .gainLeft           = CY_PDM_PCM_BYPASS,
    .gainRight          = CY_PDM_PCM_BYPASS,
    .rxFifoTriggerLevel = 4U,
    .dmaTriggerEnable   = true,
    .interruptMask      = 0UL
};


typedef enum {
    BUFFER_FREE = 0,
    BUFFER_SCHEDULED,
    BUFFER_ARMED
} pdm_buffer_state_t;


typedef struct {
    pdm_buffer_state_t  state;
    int32_t            *buffer;
    uint32_t            count;
    uint32_t            handler;
} pdm_bufctrl_t;


typedef struct pdm_audio_s pdm_audio_obj_t;
#define OBJ_P(in)     (&(in->pdm))



////////////////////////////////////
////     DMA CONFIGURATION      ////
////////////////////////////////////


// Due to the way PSoC DMA controller works (channel deactivation flag
// is part of the current descriptor and can't be modified on-the-fly,
// rather than valid/invalid flag in the next (not yet loaded) descriptor)
// we use 3 data buffers and 3 DMA descriptors.
// Initially D0_next points to D1 and D1_next points to D2, while D2_next is NULL.
// When D0 is completed, interrupt notifies user that the buffer is now ready,
// while DMA switches to D1.
// User has to process data and call pdm_audio_release_buffer() before
// D1 transfer completes to set D2_next to point to D0. Otherwise transfer
// will be stopped and rx overflow will be signaled.


// DMA declarations
#define PDM_AUDIO_DMA_CHANNEL           0
#define PDM_AUDIO_DMA_UNIT              1
#define PDM_AUDIO_DMA_TR_GROUP_CHANNEL  0


#define _DMA_HW_BASE(unit)                      DW ## unit
#define DMA_HW_BASE(unit)                       _DMA_HW_BASE(unit)

#define _DMA_TRIGGER_IN(unit, chn)              TRIG ## unit ##_OUT_CPUSS_DW ## unit ## _TR_IN ## chn
#define DMA_TRIGGER_IN(unit, chn)               _DMA_TRIGGER_IN(unit, chn)

#define _DMA_TR_GROUP_IN(unit, in_grp, grp_out) TRIG ## unit ##_IN_TR_GROUP ## in_grp ## _OUTPUT ## grp_out
#define DMA_TR_GROUP_IN(unit, in_grp, grp_out)  _DMA_TR_GROUP_IN(unit, in_grp, grp_out)

#define _TRIGGER_GROUP_OUT(grp, select)         TRIG ## grp ## _OUT_TR_GROUP0_INPUT27 + select
#define TRIGGER_GROUP_OUT(grp, select)          _TRIGGER_GROUP_OUT(grp, select)

#define __PDM_AUDIO_DMA_HW(unit)                DW ## unit
#define _PDM_AUDIO_DMA_HW(unit)                 __PDM_AUDIO_DMA_HW(unit)
#define PDM_AUDIO_DMA_HW                        _PDM_AUDIO_DMA_HW(PDM_AUDIO_DMA_UNIT)

#define PDM_AUDIO_DMA_INTR_MASK                 CY_DMA_INTR_MASK


const cy_stc_dma_descriptor_config_t pdm_dma_descr_config =
{
    .retrigger       = CY_DMA_RETRIG_4CYC,
    .interruptType   = CY_DMA_DESCR,
    .triggerOutType  = CY_DMA_DESCR,
    .channelState    = CY_DMA_CHANNEL_DISABLED,
    .triggerInType   = CY_DMA_1ELEMENT,
    .dataSize        = CY_DMA_WORD,
    .srcTransferSize = CY_DMA_TRANSFER_SIZE_DATA,
    .dstTransferSize = CY_DMA_TRANSFER_SIZE_DATA,
    .descriptorType  = CY_DMA_1D_TRANSFER,
    .srcAddress      = NULL,
    .dstAddress      = NULL,
    .srcXincrement   = 1,
    .dstXincrement   = 0,
    .xCount          = 1,
    .srcYincrement   = 1,
    .dstYincrement   = 0,
    .yCount          = 1,
    .nextDescriptor  = NULL
};

cy_stc_dma_descriptor_t dma_descriptors[NUM_PDM_AUDIO_INTERNAL_BUFFERS] =
{
    {
        .ctl = 0,
        .src = 0,
        .dst = 0,
        .xCtl = 0,
        .yCtl = 0,
        .nextPtr = 0
    }
};


static void pdm_configure_dma(pdm_audio_obj_t *obj)
{
    cy_stc_dma_channel_config_t channel_config;
    int i;

	/* Perform Trigger Mux configuration */
	Cy_TrigMux_Connect(TRIG13_IN_AUDIOSS_TR_PDM_RX_REQ,
                       TRIGGER_GROUP_OUT(13, PDM_AUDIO_DMA_TR_GROUP_CHANNEL),
                       CY_TR_MUX_TR_INV_DISABLE,
                       TRIGGER_TYPE_LEVEL);
	Cy_TrigMux_Connect(DMA_TR_GROUP_IN(PDM_AUDIO_DMA_UNIT, 13, PDM_AUDIO_DMA_TR_GROUP_CHANNEL),
                       DMA_TRIGGER_IN(PDM_AUDIO_DMA_UNIT, PDM_AUDIO_DMA_CHANNEL),
                       CY_TR_MUX_TR_INV_DISABLE,
                       TRIGGER_TYPE_LEVEL);

    for (i = 0; i < NUM_PDM_AUDIO_INTERNAL_BUFFERS; ++i) {
        Cy_DMA_Descriptor_Init(&dma_descriptors[i], &pdm_dma_descr_config);
    }

    channel_config.descriptor  = &dma_descriptors[0];
    channel_config.preemptable = false;
    channel_config.priority    = 2;
    channel_config.enable      = false;
    channel_config.bufferable  = false;

    Cy_DMA_Channel_Init(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL, &channel_config);

    Cy_DMA_Enable(PDM_AUDIO_DMA_HW);
}


static void pdm_trigger_dma(cy_stc_dma_descriptor_t *descriptor, void *ptr, uint32_t count)
{
    // Check and wait if DMA is currently busy.
    if (PDM_AUDIO_DMA_HW->CH_STRUCT[PDM_AUDIO_DMA_CHANNEL].CH_CTL & DW_CH_STRUCT_CH_CTL_ENABLED_Msk) {
        // Busy wait till completion.
        while (Cy_DMA_Channel_GetStatus(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL) == CY_DMA_INTR_CAUSE_NO_INTR) {
        }
    }

    Cy_DMA_Descriptor_SetSrcAddress(descriptor, (const void *)ptr);
    Cy_DMA_Descriptor_SetDstAddress(descriptor, (const void *)&(PDM0->RX_FIFO_RD));
    Cy_DMA_Descriptor_SetXloopDataCount(descriptor, count);
    Cy_DMA_Channel_SetDescriptor(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL, descriptor);
    Cy_DMA_Channel_Enable(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL);
}

static void pdm_abort_dma(void)
{
    Cy_DMA_Channel_Disable(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL);
}


////////////////////////////////////
////    CLOCK CONFIGURATION     ////
////////////////////////////////////

static bool find_divider_settings(uint32_t value, cy_stc_pdm_pcm_config_t *p_config)
{
    uint32_t clk, mclk, cko;
    uint32_t total;

    for (clk = 1; clk <= 4; ++clk) {
        for (mclk = 1; mclk <= 4; ++mclk) {
            for (cko = 2; cko <= 16; ++cko) {
                total = clk * mclk * cko;
                if (total == value) {
                    p_config->clkDiv = clk - 1;
                    p_config->mclkDiv = mclk - 1;
                    p_config->ckoDiv = cko;
                    return true;
                }
                if (total > value) {
                    break;
                }
            }
        }
    }
    return false;
}


////////////////////////////////////
////  PERIPHERAL CONFIGURATION  ////
////////////////////////////////////

pdm_audio_t     *pdm_audio_object = NULL;

static pdm_bufctrl_t   buffer_control[NUM_PDM_AUDIO_INTERNAL_BUFFERS];

void pdm_audio_interrupt_handler(void);


static IRQn_Type pdm_irq_allocate_channel(pdm_audio_obj_t *obj)
{
    if (obj->dma_unit == 0) {
        return (IRQn_Type)(cpuss_interrupts_dw0_0_IRQn + obj->dma_channel);
    } else {
        return (IRQn_Type)(cpuss_interrupts_dw1_0_IRQn + obj->dma_channel);
    }
}


static int pdm_setup_irq_handler(pdm_audio_obj_t *obj, uint32_t handler)
{
    cy_stc_sysint_t irq_config;

    // Configure NVIC
    irq_config.intrPriority = PDM_AUDIO_IRQ_PRIORITY;
    irq_config.intrSrc = obj->irqn;
    if (Cy_SysInt_Init(&irq_config, (cy_israddress)(handler)) != CY_SYSINT_SUCCESS) {
        return(-1);
    }

    return 0;
}

static void pdm_init_buffers(void)
{
    uint32_t i;

    for (i = 0; i < NUM_PDM_AUDIO_INTERNAL_BUFFERS; ++i)
    {
        buffer_control[i].state = BUFFER_FREE;
        buffer_control[i].count = 0;
        buffer_control[i].buffer = NULL;
        buffer_control[i].handler = 0;
    }
}

static void pdm_reset_state(pdm_audio_obj_t *obj)
{
    pdm_init_buffers();
    obj->state = PDM_STATE_IDLE;
    obj->current_scheduled = 0;
    obj->current_completed = 0;
    obj->num_scheduled = 0;
    obj->events = 0;
}


/*
 * Initializes i/o pins for PDM-PCM controller.
 */
static void pdm_init_pins(pdm_audio_obj_t *obj)
{
    int data_function = pinmap_function(obj->pin_data_in, PinMap_PDM_DATA);
    int clk_function = pinmap_function(obj->pin_clk, PinMap_PDM_CLK);
    pin_function(obj->pin_data_in, data_function);
    pin_function(obj->pin_clk, clk_function);
}


/*
 * Initializes and enables .
 */
static void pdm_init_peripheral(pdm_audio_obj_t *obj)
{
    cy_stc_pdm_pcm_config_t pdm_config = default_pdm_pcm_config;

    if (!find_divider_settings(MBED_CONF_PDM_AUDIO_CLOCK_DIVIDER, &pdm_config)) {
            error("PDM AUDIO invalid clock divider settings: %u", MBED_CONF_PDM_AUDIO_CLOCK_DIVIDER);
    }
    pdm_config.sincDecRate = MBED_CONF_PDM_AUDIO_OVERSAMPLE / 2;
    Cy_PDM_PCM_Init(obj->base, &pdm_config);
}


/*
 * Callback function to handle into and out of deep sleep state transitions.
 */
#if DEVICE_SLEEP && DEVICE_LPTICKER
static cy_en_syspm_status_t pdm_pm_callback(cy_stc_syspm_callback_params_t *callback_params)
{
    cy_stc_syspm_callback_params_t params = *callback_params;
    params.context = NULL;

    return Cy_PDM_PCM_DeepSleepCallback(&params);
}
#endif // DEVICE_SLEEP && DEVICE_LPTICKER


void pdm_audio_init(pdm_audio_t *obj_in, PinName data_in, PinName clk)
{
    pdm_audio_obj_t *obj = OBJ_P(obj_in);
    uint32_t pdm = pinmap_peripheral(data_in, PinMap_PDM_DATA);
    pdm = pinmap_merge(pdm, pinmap_peripheral(clk, PinMap_PDM_CLK));
    if (pdm != (uint32_t)NC) {
        MBED_ASSERT(pdm_audio_object == NULL);
        pdm_audio_object = obj_in;
        if (cy_reserve_io_pin(data_in) || cy_reserve_io_pin(clk)) {
            error("PDM AUDIO pin reservation conflict.");
        }
        obj->base = (PDM_Type*)pdm;
        obj->pin_data_in = data_in;
        obj->pin_clk = clk;
        obj->state = PDM_STATE_IDLE;
        pdm_reset_state(obj);
        pdm_init_pins(obj);
        pdm_init_peripheral(obj);
        pdm_configure_dma(obj);
        obj->irqn = pdm_irq_allocate_channel(obj);
#if DEVICE_SLEEP && DEVICE_LPTICKER
        obj->pm_callback_handler.callback = pdm_pm_callback;
        obj->pm_callback_handler.type = CY_SYSPM_DEEPSLEEP;
        obj->pm_callback_handler.skipMode = 0;
        obj->pm_callback_handler.callbackParams = &obj->pm_callback_params;
        obj->pm_callback_params.base = obj->base;
        obj->pm_callback_params.context = obj;
        if (!Cy_SysPm_RegisterCallback(&obj->pm_callback_handler)) {
            error("PDM AUDIO PM callback registration failed!");
        }
#endif // DEVICE_SLEEP && DEVICE_LPTICKER
    } else {
        error("PDM AUDIO pinout mismatch. Requested pins can't be used for PDM audio peripheral.");
    }
}

void pdm_audio_free(pdm_audio_t *obj_in)
{
    pdm_audio_obj_t *obj = OBJ_P(obj_in);

}


static void pdm_trigger_buffer(pdm_audio_obj_t *obj)
{
    MBED_ASSERT(obj->current_completed < NUM_PDM_AUDIO_INTERNAL_BUFFERS);
    MBED_ASSERT(obj->num_scheduled > 0);

    pdm_bufctrl_t *buffer = &buffer_control[obj->current_completed];
    obj->num_scheduled--;
    MBED_ASSERT(buffer->state == BUFFER_SCHEDULED);
    buffer->state = BUFFER_ARMED;
    MBED_ASSERT(buffer->buffer);
    obj->state = PDM_STATE_STREAMING;
    pdm_setup_irq_handler(obj, buffer->handler);
    pdm_trigger_dma(&dma_descriptors[obj->current_completed], buffer->buffer, buffer->count);

    // Enable interrupts;
    NVIC_EnableIRQ(obj->irqn);
}



void pdm_audio_rx_async(pdm_audio_t *obj_in, int32_t *rx_buffer, uint32_t count, uint32_t handler, uint32_t event, DMAUsage hint)
{
    pdm_audio_obj_t *obj = OBJ_P(obj_in);

    if (obj->num_scheduled < NUM_PDM_AUDIO_INTERNAL_BUFFERS) {
        uint32_t current = obj->current_scheduled;
        pdm_bufctrl_t *buffer = &buffer_control[current];

        obj->current_scheduled = (obj->current_scheduled + 1) % NUM_PDM_AUDIO_INTERNAL_BUFFERS;
        buffer->buffer = rx_buffer;
        buffer->count = count;
        buffer->handler = handler;
        obj->num_scheduled++;
        if (obj->state == PDM_STATE_IDLE) {
            // Clear PDM-PCM fifo and status bits.
            Cy_PDM_PCM_ClearFifo(obj->base);
            (void)Cy_PDM_PCM_GetInterruptStatus(obj->base);
        }
        if (obj->state == PDM_STATE_IDLE || obj->state == PDM_STATE_STOPPED) {
            pdm_trigger_buffer(obj);
        }
    }
}


uint32_t pdm_audio_irq_handler_asynch(pdm_audio_t *obj_in)
{
    MBED_ASSERT(obj_in == pdm_audio_object);
    pdm_audio_obj_t *obj = OBJ_P(obj_in);
    uint32_t event = 0;

    cy_en_dma_intr_cause_t status = Cy_DMA_Channel_GetStatus(PDM_AUDIO_DMA_HW, PDM_AUDIO_DMA_CHANNEL);

    if (status == CY_DMA_INTR_CAUSE_COMPLETION) {
        MBED_ASSERT(obj->current_completed < NUM_PDM_AUDIO_INTERNAL_BUFFERS);
        buffer_t *ubuffer = &obj_in->rx_buff;
        pdm_bufctrl_t *buffer = &buffer_control[obj->current_completed];
        MBED_ASSERT(buffer->state == BUFFER_ARMED);
        ubuffer->pos = obj->current_completed;
        ubuffer->buffer = buffer->buffer;
        ubuffer->length = buffer->count;
        ubuffer->width = sizeof(uint32_t);
        buffer->state = BUFFER_FREE;
        buffer->buffer = NULL;
        buffer->count = 0;
        // Switch to next buffer and trigger DMA.
        obj->current_completed = (obj->current_completed + 1) % NUM_PDM_AUDIO_INTERNAL_BUFFERS;

        if (obj->num_scheduled > 0) {
            pdm_trigger_buffer(obj);
        } else {
            // No more scheduled buffers;
            obj->state = PDM_STATE_STOPPED;
            event |= PDM_AUDIO_EVENT_STREAM_STOPPED;
        }
        // Notify user.
        event |= PDM_AUDIO_EVENT_RX_COMPLETE;
    } else {
        // error handling
        NVIC_DisableIRQ(obj->irqn);
        pdm_abort_dma();
        obj->state = PDM_STATE_STOPPED;
        event |= PDM_AUDIO_EVENT_DMA_ERROR;
    }

    if (Cy_PDM_PCM_GetInterruptStatus(obj->base) & CY_PDM_PCM_INTR_RX_OVERFLOW) {
        event |= PDM_AUDIO_EVENT_OVERRUN;
    }

    return event;
}


uint8_t pdm_audio_active(pdm_audio_t *obj_in)
{
    pdm_audio_obj_t *obj = OBJ_P(obj_in);
    return (obj->state != PDM_STATE_IDLE);
}


void pdm_audio_abort_asynch(pdm_audio_t *obj_in)
{
    pdm_audio_obj_t *obj = OBJ_P(obj_in);
    if (obj->state != PDM_STATE_IDLE) {
        NVIC_DisableIRQ(obj->irqn);
        pdm_abort_dma();
        pdm_reset_state(obj);
    }
}





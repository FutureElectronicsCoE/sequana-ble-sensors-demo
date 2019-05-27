/*
 * Mbed OS device driver for ROHM BD2808 24-channel LED Controller
 * on Future Electronics Sequana board (PSoC 6 platform).
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
#ifndef MBED_PDM_AUDIO_H
#define MBED_PDM_AUDIO_H

#include "platform/platform.h"

#include "platform/PlatformMutex.h"
#include "platform/SingletonPtr.h"
#include "platform/NonCopyable.h"
#include "platform/Callback.h"
#include "platform/CThunk.h"
#include "hal/dma_api.h"
#include "hal/pdm_audio_api.h"



namespace mbed {


class PDMAudio : private NonCopyable<PDMAudio> {

public:

    /** Create a PDM Audio driver.
     *
     *  @note PDM audio parameters are are fixed in the HAL implementation.
     *
     */
    PDMAudio(PinName data_in, PinName clk);

    virtual ~PDMAudio();

    /** Begin receiving of audio data.
     *
     *  Audio data is a stream that doesn't stop by itself, so once receive is started
     *  read operation must be continuously invoked, otherwise overrun event will occur.
     *  When read operation completes the next one must be started before hardware FIFO
     *  is completely filled in. It is possible to start new read operation before the
     *  previous one completes. This gives more time for data processing.
     *  The read operation ends with any of the enabled events and invokes registered
     *  callback function (which can be NULL to not receive callback at all).
     *  Events that are not enabled by event argument are simply ignored.
     *  Operation has to be ended explicitly by calling abort_read() when
     *  no events are enabled.
     *  This function locks the deep sleep until any event has occurred.
     *
     *  @param buffer     The buffer where received data will be stored
     *  @param length     The buffer length in 32-bit words (samples)
     *  @param callback   The event callback function
     *  @param event      The logical OR of RX events that should end operation
     *  @return Zero if new transaction was started, -1 if no space to schedule transaction
     */
    int read(int32_t *buffer, int length, const event_callback_t &callback, int event = PDM_AUDIO_EVENT_RX_COMPLETE);

    /** Abort the on-going read transfer
     *
     *  It is safe to call abort_read() when there is no on-going transaction.
     */
    void abort_read();

#if !defined(DOXYGEN_ONLY)

protected:
    /** PDMAudio interrupt handler.
     */
    void irq_handler_asynch(void);

    /* Internal PDM object identifying the resources */
    pdm_audio_t _pdm_dev;
    /* Current sate of the sleep manager */
    uint32_t _deep_sleep_locked;
    /* Current user of the SPI */
    static PDMAudio *_owner;
    /* Used by lock and unlock for thread safety */
    static SingletonPtr<PlatformMutex> _mutex;

    CThunk<PDMAudio> _thunk_irq;
    event_callback_t _rx_callback;
private:

#endif //!defined(DOXYGEN_ONLY)
};

} // namespace mbed

#endif //MBED_PDM_AUDIO_H

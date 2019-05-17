/*
 * Mbed OS device driver for ROHM PDMAudio 24-channel LED Controller
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

#include "PDMAudio.h"
#include "platform/mbed_critical.h"
#include "platform/mbed_wait_api.h"
#include "platform/mbed_power_mgmt.h"
#include "hal/pdm_audio_api.h"

namespace mbed {



PDMAudio *PDMAudio::_owner = NULL;

SingletonPtr<PlatformMutex> PDMAudio::_mutex;


PDMAudio::PDMAudio(PinName data_in, PinName clk) :
    _pdm_dev(),
    _deep_sleep_locked(false),
    _thunk_irq(this)
{
    lock();
    pdm_audio_init(&_pdm_dev, data_in, clk);
    _owner = this;
    unlock();
}

PDMAudio::~PDMAudio()
{
   if (_owner == this) {
        pdm_audio_free(&_pdm_dev);
        _owner = NULL;
    }
}

void PDMAudio::lock()
{
    _mutex->lock();
}

void PDMAudio::unlock()
{
    _mutex->unlock();
}


int PDMAudio::read(int32_t *buffer, int length, const event_callback_t &callback, int event)
{
    lock();
    _rx_callback = callback;
    _thunk_irq.callback(&PDMAudio::irq_handler_asynch);
    sleep_manager_lock_deep_sleep();
    _deep_sleep_locked++;
    pdm_audio_rx_async(&_pdm_dev, buffer, length, _thunk_irq.entry(), event, DMA_USAGE_ALWAYS);
    unlock();
    return 0;
}


void PDMAudio::abort_read()
{
    lock();
    pdm_audio_abort_asynch(&_pdm_dev);
    while (_deep_sleep_locked) {
        sleep_manager_unlock_deep_sleep();
        _deep_sleep_locked--;
    }
    unlock();
}


void PDMAudio::irq_handler_asynch(void)
{
    int event = pdm_audio_irq_handler_asynch(&_pdm_dev);
    if (_rx_callback && event) {
        _rx_callback.call(event);
    }

    if (_deep_sleep_locked) {
        _deep_sleep_locked--;
        sleep_manager_unlock_deep_sleep();
    }
}

} // namespace mbed

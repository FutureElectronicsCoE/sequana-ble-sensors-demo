/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
 * Copyright (c) 2017-2019 Future Electronics
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

#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "pretty_printer.h"

#include <mbed.h>
#include <vector>
#include "events/mbed_events.h"
#include "ble/BLE.h"
#include "ble/gap/Gap.h"
#include "SequanaPrimaryService.h"
#include "kx64.h"

using namespace sequana;

const static char     DEVICE_NAME[] = "Sequana";

static sequana::PrimaryService *sequana_service_ptr;

static EventQueue event_queue(/* event count */ 64 * EVENTS_EVENT_SIZE);

/* Buses and I/O */
SPI spi1(SPI_MOSI, SPI_MISO, SPI_CLK);

/* Sensor declarations  */
Kx64Sensor      kx64(spi1, P9_5);

class SequanaDemo : ble::Gap::EventHandler {
public:
    SequanaDemo(BLE &ble, events::EventQueue &event_queue, sequana::PrimaryService& primary_service) :
        _ble(ble),
        _event_queue(event_queue),
        _led1(LED1, 1),
        _connected(false),
        _primary_uuid(sequana::PrimaryService::UUID_SEQUANA_PRIMARY_SERVICE),
        _primary_service(primary_service),
        _adv_data_builder(_adv_buffer) { }

    void start() {
        _ble.gap().setEventHandler(this);

        _ble.init(this, &SequanaDemo::on_init_complete);

        _event_queue.call_every(500, this, &SequanaDemo::blink);
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
        if (params->error != BLE_ERROR_NONE) {
            printf("BLE initialization failed\n");
            return;
        }

        print_mac_address();

        start_advertising();
    }

    void start_advertising() {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setLocalServiceList(mbed::make_Span(&_primary_uuid, 1));
        _adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            printf("_ble.gap().setAdvertisingParameters() failed\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            printf("_ble.gap().setAdvertisingPayload() failed\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\n");
            return;
        }
    }

    void blink(void) {
        /* LED will be on when a client is connected or will blink while advertising */
        if (_connected) {
            _led1 = 1;
        } else {
            _led1 = !_led1;
        }
    }

private:
    /* Event handler */

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent&) {
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        _connected = false;
    }

    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) {
        if (event.getStatus() == BLE_ERROR_NONE) {
            _connected = true;
        }
    }

private:
    BLE                         &_ble;
    events::EventQueue          &_event_queue;
    DigitalOut                  _led1;
    bool                        _connected;
    UUID                        _primary_uuid;
    sequana::PrimaryService     &_primary_service;
    uint8_t                     _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};


/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    printf("Application processor started.\n\n");

    /* Initialize bus(es) (placeholder) */
    spi1.frequency(1000000);

    /* Setup BLE interface */
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    /* Setup primary service (placeholder). */
    sequana_service_ptr = new sequana::PrimaryService(ble, kx64);

    /* Start BLE / demo application */
    SequanaDemo demo(ble, event_queue, *sequana_service_ptr);
    demo.start();

    printf("BLE started.\n\n");

    /* Process all events */
    event_queue.dispatch_forever();

    return 0;
}


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
#include "sps30.h"
#include "ComboEnvSensor.h"
#include "AirQSensor.h"

#ifndef MCU_PSoC6_M0

using namespace sequana;

static const uint8_t AS7261_ADDR = 0x92;
static const uint8_t HS3001_ADDR = 0x88;
static const uint8_t ZMOD44XX_ADDR = 0x64;
static const uint8_t SCD30_ADDR = 0xc2;


I2C i2c1(I2C_SDA, I2C_SCL);
SPI spi1(SPI_MOSI, SPI_MISO, SPI_CLK);
RawSerial uart1(D1, D0);
DigitalOut zmod1_reset(D3);

#ifdef TARGET_FUTURE_SEQUANA
const static char     DEVICE_NAME[] = "Sequana";
#else
const static char     DEVICE_NAME[] = "SequanaEnvShield";
#endif  //TARGET_FUTURE_SEQUANA

static sequana::PrimaryService *sequanaServicePtr;

static EventQueue event_queue(/* event count */ 64 * EVENTS_EVENT_SIZE);


#ifdef TARGET_FUTURE_SEQUANA
Kx64Sensor      kx64(spi1, P9_5);
#endif //TARGET_FUTURE_SEQUANA

Sps30Sensor     sps30(uart1);
ComboEnvSensor  combo(i2c1, AS7261_ADDR, HS3001_ADDR);
AirQSensor      airq(i2c1, ZMOD44XX_ADDR, zmod1_reset, SCD30_ADDR);


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
            printf("Ble initialization failed.");
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
        //_adv_data_builder.setAppearance(ble::adv_data_appearance_t::GENERIC_HEART_RATE_SENSOR);
        _adv_data_builder.setLocalServiceList(mbed::make_Span(&_primary_uuid, 1));
        _adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            printf("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            printf("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }
    }

    void blink(void) {
        _led1 = !_led1;
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
    BLE &_ble;
    events::EventQueue &_event_queue;
    DigitalOut _led1;

    bool _connected;

    UUID _primary_uuid;

    uint8_t _hr_counter;
    sequana::PrimaryService& _primary_service;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};


/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    printf("Application processor started.\n\n");

    // Initialize buses.
    i2c1.frequency(100000);
    spi1.format(8, 0);
    spi1.frequency(1000000);

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    /* Setup primary service. */
    sequanaServicePtr = new sequana::PrimaryService(ble,
#ifdef TARGET_FUTURE_SEQUANA
                                                    kx64,
#endif //TARGET_FUTURE_SEQUANA
                                                    sps30,
                                                    combo,
                                                    airq);

    SequanaDemo demo(ble, event_queue, *sequanaServicePtr);
    demo.start();

    printf("BLE started.\n\n");

#ifdef TARGET_FUTURE_SEQUANA
    kx64.start(event_queue);
#endif //TARGET_FUTURE_SEQUANA
    sps30.start(event_queue);
    combo.start(event_queue);
    airq.start(event_queue);
    event_queue.dispatch_forever();

    return 0;
}


#endif // MCU_PSoC6_M0

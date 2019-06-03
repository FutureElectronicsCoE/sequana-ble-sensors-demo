/*
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

#include <mbed.h>
#include "ble/BLE.h"
#include "UUID.h"

#include "SequanaPrimaryService.h"
#include "app_version.h"
#include <string.h>

namespace sequana {

const UUID PrimaryService::UUID_SEQUANA_PRIMARY_SERVICE("F79B4EB2-1B6E-41F2-8D65-D346B4EF5685");

UUID UUID_TEMPERATURE_CHAR(GattCharacteristic::UUID_TEMPERATURE_CHAR);
UUID UUID_RGB_LED_CHAR("F79B4EB4-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_ACCELLEROMETER_CHAR("F79B4EB5-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_MAGNETOMETER_CHAR("F79B4EB7-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_AIR_QUALITY_CHAR("f79B4EBA-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_PARTICULATE_MATTER_CHAR("F79B4EBB-1B6B-41F2-8D65-D346B4EF5685");
UUID UUID_OTHER_ENV_CHAR("F79B4EBC-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_SEQUANA_INFO_CHAR("F79B4EB9-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_SEQUANA_LIGHT_SENSOR_CONTROL_CHAR("F79B4EBD-1B6E-41F2-8D65-D346B4EF5685");

int16_t tempValue = 0;

SingleCharParams accMagSensorCharacteristics[2] = {
    { &UUID_ACCELLEROMETER_CHAR, 0, 6 },
    { &UUID_MAGNETOMETER_CHAR, 6, 6 }
};

SingleCharParams comboEnvSensorCharacteristics[2] = {
    { &UUID_TEMPERATURE_CHAR, 0, 2 },
    { &UUID_OTHER_ENV_CHAR, 2, 12 }
};

static const char version_info[] =
#ifdef TARGET_FUTURE_SEQUANA
    "Sequana"
#else
    "Sequana Environmental Shield"
#endif // TARGET_FUTURE_SEQUANA
    " firmware version "
    APP_VERSION_STR
#if !(APP_VERSION_MAJOR || APP_VERSION_MINOR)
    " build at " __DATE__ "" __TIME__
#endif
;

static const char initial_info[SEQUANA_INFO_MAX_LEN] = "";


PrimaryService::PrimaryService(BLE &ble,
#ifdef TARGET_FUTURE_SEQUANA
                               Kx64Sensor &kx64,
#endif //TARGET_FUTURE_SEQUANA
                               Sps30Sensor &sps30,
                               LightSensor &lsc,
                               ComboEnvSensor &combo,
                               AirQSensor &airq
#ifdef TARGET_FUTURE_SEQUANA
                               ,
                               RGBLedActuator &rgb_led
#endif //TARGET_FUTURE_SEQUANA
                               ) :
        _ble(ble),
#ifdef TARGET_FUTURE_SEQUANA
        _accMagSensorMeasurement(ble,
                                accMagSensorCharacteristics,
                                kx64),
#endif //TARGET_FUTURE_SEQUANA
        _particulateMatterMeasurement(ble,
                                      UUID_PARTICULATE_MATTER_CHAR,
                                      sps30),
        _lightSensorCalibrator(ble,
                                UUID_SEQUANA_LIGHT_SENSOR_CONTROL_CHAR,
                                lsc),
        _comboEnvMeasurement(ble,
                             comboEnvSensorCharacteristics,
                             combo),
        _airQMeasurement(ble,
                         UUID_AIR_QUALITY_CHAR,
                         airq),
#ifdef TARGET_FUTURE_SEQUANA
        _ledState(ble,
                  UUID_RGB_LED_CHAR,
                  rgb_led),
#endif // TARGET_FUTURE_SEQUANA
        _info(UUID_SEQUANA_INFO_CHAR, (char*)initial_info)
{
        GattCharacteristic *sequanaChars[] = {
#ifdef TARGET_FUTURE_SEQUANA
             _accMagSensorMeasurement.get_characteristic(0),
             _accMagSensorMeasurement.get_characteristic(1),
#endif //TARGET_FUTURE_SEQUANA
             _particulateMatterMeasurement.get_characteristic(),
             _comboEnvMeasurement.get_characteristic(0),
             _comboEnvMeasurement.get_characteristic(1),
             _airQMeasurement.get_characteristic(),
#ifdef TARGET_FUTURE_SEQUANA
             _ledState.get_characteristic(),
#endif // TARGET_FUTURE_SEQUANA            ,
             &_info
        };

        GattService sequanaService(UUID_SEQUANA_PRIMARY_SERVICE, sequanaChars, sizeof(sequanaChars) / sizeof(GattCharacteristic *));

        _ble.gattServer().addService(sequanaService);

        set_information(version_info, strlen(version_info));
}

#ifdef TARGET_FUTURE_SEQUANA
void PrimaryService::on_data_written(const GattWriteCallbackParams *params)
{
    if ((params->handle == _ledState.get_characteristic()->getValueHandle()) && (params->len == 6)) {
        RGBLedValue value(params->data);
        _ledState.set_actuator(value);
    }
}
#endif // TARGET_FUTURE_SEQUANA

void PrimaryService::set_information(const char* info, size_t length)
{
    if (length <= SEQUANA_INFO_MAX_LEN) {
        _ble.gattServer().write(_info.getValueHandle(), (const uint8_t*)info, length);
    }
}

} //namespace


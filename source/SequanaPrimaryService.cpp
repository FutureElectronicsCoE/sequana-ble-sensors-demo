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


namespace sequana {

const UUID PrimaryService::UUID_SEQUANA_PRIMARY_SERVICE("F79B4EB2-1B6E-41F2-8D65-D346B4EF5685");

UUID UUID_TEMPERATURE_CHAR(GattCharacteristic::UUID_TEMPERATURE_CHAR);
UUID UUID_ACCELLEROMETER_CHAR("F79B4EB5-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_MAGNETOMETER_CHAR("F79B4EB7-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_AIR_QUALITY_CHAR("f79B4EBA-1B6E-41F2-8D65-D346B4EF5685");
UUID UUID_PARTICULATE_MATTER_CHAR("F79B4EBB-1B6B-41F2-8D65-D346B4EF5685");
UUID UUID_OTHER_ENV_CHAR("F79B4EBC-1B6E-41F2-8D65-D346B4EF5685");

int16_t tempValue = 0;

SingleCharParams accMagSensorCharacteristics[2] = {
    { &UUID_ACCELLEROMETER_CHAR, 0, 6 },
    { &UUID_MAGNETOMETER_CHAR, 6, 6 }
};


PrimaryService::PrimaryService(BLE &ble,
                               Kx64Sensor &kx64) :
        _ble(ble),
        _accMagSensorMeasurement(ble,
                                accMagSensorCharacteristics,
                                kx64)
{
        GattCharacteristic *sequanaChars[] = {
             _accMagSensorMeasurement.get_characteristic(0),
             _accMagSensorMeasurement.get_characteristic(1)
        };

        GattService sequanaService(UUID_SEQUANA_PRIMARY_SERVICE, sequanaChars, sizeof(sequanaChars) / sizeof(GattCharacteristic *));

        _ble.gattServer().addService(sequanaService);
}

} //namespace


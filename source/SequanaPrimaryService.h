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

#ifndef SEQUANAPRIMARYSERVICE_H__
#define SEQUANAPRIMARYSERVICE_H__

#include "ble/BLE.h"
#include "UUID.h"
#include "Sensor.h"
#include "SensorCharacteristic.h"
#include "Kx64.h"

namespace sequana {

/* Instantiation of binary buffers for characteristics values */
typedef CharBuffer<Kx64Value, 12>   Kx64CharBuffer;



/** Sequana BLE Primary Service. This service provides data obtained from Sequana and Sequana Environmental Shield sensors.
 */
class PrimaryService {
public:
    static const UUID UUID_SEQUANA_PRIMARY_SERVICE;

public:
    /** Add Sequana Primary Service to an existing BLE object, initializing it with all characteristics.
     * @param ble                   Reference to the BLE device.
     * @param accmag_sensor         Reference to KX64 sensor.
     * @param partmatter_sensor     Reference to PSP30 sensor.
     * @param combo_env_sensor      Reference to combined parameters sensor.
     */
    PrimaryService(BLE &ble,
                   Kx64Sensor &accmag_sensor );

protected:
    BLE &_ble;
    SensorMultiCharacteristic<2, Kx64CharBuffer, Kx64Value>         _accMagSensorMeasurement;
};

} //namespace

#endif // SEQUANAPRIMARYSERVICE_H__

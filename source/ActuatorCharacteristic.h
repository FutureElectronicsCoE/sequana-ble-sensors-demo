/*
 * Copyright (c) 2017-2018 Future Electronics
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

#ifndef ACTUATOR_CHARACTERISTIC_H_
#define ACTUATOR_CHARACTERISTIC_H_

#include <mbed.h>
#include "Actuator.h"
#include "SensorCharacteristic.h"

//#define ENABLE_CHAR_TEMPLATE_DEBUG  1

namespace sequana {

/** Wrapper, holding BLE characteristic with its value buffer
 *  and processing value updates.
 *  Version for actuators i.e. read-write characteristics.
 *
 * @param B Class representing characteristic's binary data buffer.
 * @param V Data type representing sensor value.
 */
template <class B, class V> class ActuatorCharacteristic : public BaseSensorCharacteristic<B, V>
{
public:
    /** Create BLE characteristic for a sensor.
     *
     * @param ble BLE interface
     * @param uuid characteristic UUID
     * @param sensor sensor interface
     */
    ActuatorCharacteristic(BLE& ble, const UUID& uuid, Actuator<V>& sensor) :
        BaseSensorCharacteristic<B, V>(ble, uuid, sensor),
        _characteristic(uuid,
                       reinterpret_cast<B*>(BaseSensorCharacteristic<B, V>::get_ptr()),
                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE |
                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {}

    /** Get sensor characteristic.
     *
     * @returns pointer to sensor's GATT characteristic
     */
    virtual GattCharacteristic* get_characteristic() { return &_characteristic; }

    int set_actuator(V& value)
    {
        return static_cast<Actuator<V> &>(BaseSensorCharacteristic<B, V>::_sensor).set_value(value);
    }

protected:
    ReadWriteGattCharacteristic<B>  _characteristic;
};

} // namespace

#endif // ACTUATOR_CHARACTERISTIC_H_

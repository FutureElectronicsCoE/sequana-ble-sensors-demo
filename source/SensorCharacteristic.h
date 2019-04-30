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

#ifndef SENSOR_CHARACTERISTIC_H_
#define SENSOR_CHARACTERISTIC_H_

#include <mbed.h>
#include "ble/BLE.h"
#include "Sensor.h"

//#define ENABLE_CHAR_TEMPLATE_DEBUG  1

namespace sequana {

#if ENABLE_CHAR_TEMPLATE_DEBUG
/** Get differentiating part of the characteristic long UUID.
 *
 * Helper function to help print useful part of the long UUID to identify characteristic.
 */
static inline uint32_t get_char_id(GattCharacteristic& characteristic)
{
    const UUID& uuid = characteristic.getValueAttribute().getUUID();
    return uuid.shortOrLong()? *(uint32_t *)(uuid.getBaseUUID()+12) : uuid.getShortUUID();
}
#endif // ENABLE_CHAR_TEMPLATE_DEBUG


/** Public interface to buffer holding characteristic binary value.
 *
 * Default implementation is valid only for constant data length characteristics,
 * but can be overloaded to support also dynamic data size.
 *
 * @param ValueT Type representing related sensor value.
 * @param SIZE Size in bytes of the characteristic data part.
 */
template <typename ValueT, size_t SIZE> class CharBuffer{
public:
    /** Default constructor.
     *
     * Clears binary data to zero.
     */
    CharBuffer()    {memset(_bytes, 0, SIZE);}

   /** Return pointer to binary data buffer.
     */
    uint8_t         *get_ptr() { return _bytes; }

    /** Return length of the binary data.
     */
    size_t          get_length() { return SIZE; }

    /** Default conversion/assignment operator, performs a simple copy.
     */
    CharBuffer<ValueT, SIZE>& operator= (const ValueT& val)
    {
        memcpy(_bytes, &val, SIZE);
        return *this;
    }

protected:
    uint8_t         _bytes[SIZE];   // Buffer holding binary representation of characteristic value.
};


/** Wrapper, holding BLE characteristic with its value buffer
 *  and processing value updates.
 *  This class is a base providing generic processing from which
 *  particular types are derived depending on the characteristic
 *  attribute properties (read-only, read-write etc.).
 *
 * @param B Class representing characteristic's binary data buffer.
 * @param V Data type representing sensor value.
 */
template <class B, class V> class BaseSensorCharacteristic
{
protected:
    /** Create BLE characteristic for a sensor.
     *
     * @param ble BLE interface
     * @param uuid characteristic UUID
     * @param sensor sensor interface
     */
    BaseSensorCharacteristic(BLE& ble, const UUID& uuid, Sensor<V>& sensor) :
        _ble(ble),
        _sensor(sensor)
    {
#if ENABLE_CHAR_TEMPLATE_DEBUG
            printf("Registered char 0x%08lx\n", get_char_id(_characteristic));
#endif // ENABLE_CHAR_TEMPLATE_DEBUG
            _sensor.register_updater(callback(this, &BaseSensorCharacteristic::update));
    }

public:
    /** Get sensor characteristic.
     *
     * @returns pointer to sensor's GATT characteristic
     */
    virtual GattCharacteristic* get_characteristic() = 0;

    /** Get characteristic data pointer.
     *
     * @returns pointer to buffer holding characteristic binary data value
     */
    uint8_t *get_ptr() { return _buffer.get_ptr(); }

    /** Get characteristic data length.
     *
     * @returns characteristic current binary data length
     */
    size_t get_length() { return _buffer.get_length(); }

protected:
    /** Process update of the sensor's value.
     */
    void update()
    {
        if (_ble.gap().getState().connected) {
            _buffer = _sensor.get_value();
#if ENABLE_CHAR_TEMPLATE_DEBUG
            printf("Updating char 0x%08lx with: ", get_char_id(_characteristic));
            uint8_t *ptr = _buffer.get_ptr();
            for (uint32_t i = 0; i < _buffer.get_length(); ++i) {
                printf("%02x", *ptr++);
            }
            printf("\n");
#endif // ENABLE_CHAR_TEMPLATE_DEBUG
            _ble.gattServer().write(get_characteristic()->getValueHandle(),
                                    _buffer.get_ptr(),
                                    _buffer.get_length());
        }
    }

protected:
    BLE&                            _ble;
    B                               _buffer;
    Sensor<V>&                      _sensor;
};


/** Wrapper, holding BLE characteristic with its value buffer
 *  and processing value updates.
 *  Version for basic sensors i.e. read-only characteristics.
 *
 * @param B Class representing characteristic's binary data buffer.
 * @param V Data type representing sensor value.
 */
template <class B, class V> class SensorCharacteristic : public BaseSensorCharacteristic<B, V>
{
public:
    /** Create BLE characteristic for a sensor.
     *
     * @param ble BLE interface
     * @param uuid characteristic UUID
     * @param sensor sensor interface
     */
    SensorCharacteristic(BLE& ble, const UUID& uuid, Sensor<V>& sensor) :
        BaseSensorCharacteristic<B, V>(ble, uuid, sensor),
        _characteristic(uuid,
                       reinterpret_cast<B*>(BaseSensorCharacteristic<B, V>::get_ptr()),
                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
                       GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {}

    /** Get sensor characteristic.
     *
     * @returns pointer to sensor's GATT characteristic
     */
    virtual GattCharacteristic* get_characteristic() { return &_characteristic; }

protected:
    ReadOnlyGattCharacteristic<B>   _characteristic;
};



/*****************************************************************************/
/*                       Multi-characteristic sensor                         */
/*****************************************************************************/


/** Object describing (parameterizing) single characteristic
 * used to define multi-characteristic sensors.
 */
struct SingleCharParams {
    UUID*     uuid;     // characteristic UUID
    uint16_t  offset;   // offset in the sensor value buffer where characteristic value starts
    uint16_t  length;   // length of the characteristic value
};

/** Wrapper, holding multiple BLE characteristics with its value buffers
 *  and processing value updates.
 *
 * Useful in a case where a single sensor should return multiple characteristic values.
 *
 * @param B Class representing characteristic's binary data buffer.
 * @param V Data type representing sensor value.
 */
template <size_t N, class B, class V> class SensorMultiCharacteristic
{
public:
    /** Create characteristic set for a sensor.
     *
     * @param ble BLE interface
     * @param params set of characteristics and their parameters
     * @param sensor senasor interface
     */
    SensorMultiCharacteristic(BLE& ble, SingleCharParams params[N], Sensor<V>& sensor) :
        _ble(ble),
        _sensor(sensor),
        _params(params)
    {
        for (size_t i = 0; i < N; ++i) {
            SingleCharParams &p = _params[i];
            _characteristic[i] = new GattCharacteristic(
                                        *p.uuid,
                                        _buffer.get_ptr() + p.offset,
                                        p.length,
                                        p.length,
                                        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
                                        NULL,
                                        0,
                                        false);
#if ENABLE_CHAR_TEMPLATE_DEBUG
            printf("Registered mchar 0x%08lx\n", get_char_id(*_characteristic[i]));
#endif // ENABLE_CHAR_TEMPLATE_DEBUG
        }
        _sensor.register_updater(callback(this, &SensorMultiCharacteristic::update));
    }

    /** Get pointer to GATT characteristic object for n'ths characteristic.
     *
     * @param idx index of the characteristic
     */
    GattCharacteristic* get_characteristic(size_t idx)
    {
        MBED_ASSERT(idx < N);
        return _characteristic[idx];
    }

    /** Get number of characteristics in this set.
     */
    size_t get_num()
    {
        return N;
    }

    ~SensorMultiCharacteristic()
    {
        for (size_t i = 0; i < N; ++i) {
            if (_characteristic[i]) {
                delete _characteristic[i];
            }
        }
    }

protected:
    void update()
    {
        if (_ble.gap().getState().connected) {
            _buffer = _sensor.get_value();
            for (size_t i = 0; i < N; ++i) {
                SingleCharParams& p = _params[i];
#if ENABLE_CHAR_TEMPLATE_DEBUG
                printf("Updating mchar 0x%08lx with: ", get_char_id(*_characteristic[i]));

                uint8_t* ptr = _buffer.get_ptr()+p.offset;
                for (uint32_t j = 0; j < p.length; ++j) {
                    printf("%02x", *ptr++);
                }
                printf("\n");
#endif // ENABLE_CHAR_TEMPLATE_DEBUG
                _ble.gattServer().write(_characteristic[i]->getValueHandle(),
                                       _buffer.get_ptr() + p.offset,
                                       p.length);
            }
        }
    }

protected:
    BLE&                            _ble;
    Sensor<V>&                      _sensor;
    GattCharacteristic*             _characteristic[N];
    B                               _buffer;
    SingleCharParams*               _params;
};

} // namespace


#endif // SENSOR_CHARACTERISTIC_H_

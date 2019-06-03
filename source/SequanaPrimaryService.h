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
#include "ActuatorCharacteristic.h"
#include "Kx64.h"
#include "Sps30.h"
#include "LightSensor.h"
#include "ComboEnvSensor.h"
#include "AirQSensor.h"
#include "RGBLedActuator.h"

namespace sequana {

/* Instantiation of binary buffers for characteristics values */
typedef CharBuffer<Kx64Value, 12>   Kx64CharBuffer;

typedef CharBuffer<Sps30Value, 12>  Sps30CharBuffer;

typedef CharBuffer<LightSensorCalibrationValue, 3> LightSensorCharBuffer;


#define SEQUANA_INFO_MAX_LEN        250


/** Converter to create BLE characteristic data from sensor data.
 */
class AirQCharBuffer : public CharBuffer<AirQValue, 10> {
public:
    AirQCharBuffer& operator= (const AirQValue &val)
    {
        memcpy(_bytes, &val.co2, 2);
        memcpy(_bytes+2, &val.eco2, 4);
        memcpy(_bytes+6, &val.tvoc, 4);
        return *this;
    }
};


/** Converter to create BLE characteristic data from sensor data.
 */
class ComboEnvCharBuffer : public CharBuffer<ComboEnvValue, 14> {
public:
    ComboEnvCharBuffer& operator= (const ComboEnvValue &val)
    {
        memcpy(_bytes, &val.temperature, 2);
        memcpy(_bytes+2, &val.noise, 2);
        memcpy(_bytes+4, &val.ambient_light, 3);
        memcpy(_bytes+7, &val.humidity, 2);
        memcpy(_bytes+9, &val.color_temp, 2);
        _bytes[11] = val.color_red;
        _bytes[12] = val.color_green;
        _bytes[13] = val.color_blue;
        return *this;
    }
};

#ifdef TARGET_FUTURE_SEQUANA
/** Converter to create BLE characteristic data from RGB Led data.
 */
class RGBLedCharBuffer : public CharBuffer<RGBLedValue, 6> {
public:
    RGBLedCharBuffer& operator= (const RGBLedValue &val)
    {
        _bytes[0] = 0;
        _bytes[1] = val.color.r;
        _bytes[2] = 0;
        _bytes[3] = val.color.g;
        _bytes[4] = 0;
        _bytes[5] = val.color.b;
        return *this;
    }
};
#endif //TARGET_FUTURE_SEQUANA



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
#ifdef TARGET_FUTURE_SEQUANA
                   Kx64Sensor &accmag_sensor,
#endif //TARGET_FUTURE_SEQUANA
                   Sps30Sensor &partmatter_sensor,
                   LightSensor &light_sensor,
                   ComboEnvSensor &combo_env_sensor,
                   AirQSensor &airq_sensor
#ifdef TARGET_FUTURE_SEQUANA
                    ,
                   RGBLedActuator &rgb_led_actuator
#endif //TARGET_FUTURE_SEQUANA
                   );

#ifdef TARGET_FUTURE_SEQUANA
    void on_data_written(const GattWriteCallbackParams *params);
#endif //TARGET_FUTURE_SEQUANA
    void set_information(const char* info, size_t length);

protected:
    BLE &_ble;
#ifdef TARGET_FUTURE_SEQUANA
    SensorMultiCharacteristic<2, Kx64CharBuffer, Kx64Value>         _accMagSensorMeasurement;
#endif //TARGET_FUTURE_SEQUANA
    SensorCharacteristic<Sps30CharBuffer, Sps30Value>               _particulateMatterMeasurement;
    ActuatorCharacteristic<LightSensorCharBuffer, LightSensorCalibrationValue> _lightSensorCalibrator;
    SensorMultiCharacteristic<2, ComboEnvCharBuffer, ComboEnvValue> _comboEnvMeasurement;
    SensorCharacteristic<AirQCharBuffer, AirQValue>                 _airQMeasurement;
#ifdef TARGET_FUTURE_SEQUANA
    ActuatorCharacteristic<RGBLedCharBuffer, RGBLedValue>           _ledState;
#endif //TARGET_FUTURE_SEQUANA
    ReadOnlyArrayGattCharacteristic<char, SEQUANA_INFO_MAX_LEN>     _info;
};

} //namespace

#endif // SEQUANAPRIMARYSERVICE_H__

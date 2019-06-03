/*
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

#ifndef AS7261_DRIVER_H_
#define AS7261_DRIVER_H_

#include <stdint.h>
#include "TypedEnum.h"
#include <Sensor.h>


/** Driver for AS7261 light sensor.
 */
class As7261Driver {
public:
    enum Status {
        STATUS_OK = 0,
        STATUS_NOT_READY,
        STATUS_ERROR
    };

    class CalibrationCommand : public TypedEnum<uint8_t> {
    public:
        enum enum_t {
            NOP        = 0,
            CAL_RED    = 1,
            CAL_GREEN  = 2,
            CAL_BLUE   = 3,
            SET        = 4,
            RESET      = 5
        };

        CalibrationCommand(enum_t init_val) : TypedEnum<uint8_t>(init_val) {}

        static bool is_value_valid(uint8_t v) { return v <= RESET; }
    };

    class CalibrationState : public TypedEnum<uint8_t> {
    public:
        enum enum_t {
            NONE        = 0x00,
            CAL_RED     = 0x01,
            CAL_GREEN   = 0x02,
            CAL_BLUE    = 0x04,
            CAL_RGB     = 0x07,
            COMPLETED   = 0x08,
            ERROR       = 0x10
        };

        CalibrationState(enum_t init_val) : TypedEnum<uint8_t>(init_val) {}

        CalibrationState &operator|=(const enum_t v) { _val |= v; return *this; }

        bool colors_ready() const { return (_val & CAL_RGB) == CAL_RGB; }
    };

public:
    /** Create and initialize driver.
     *
     * @param bus I2C bus to use for communication
     * @param address I2C address to use
     */
    As7261Driver(I2C& bus, uint8_t address) :
        _i2c(bus),
        _address(address),
        _last_i(0),
        _last_t(0),
        _conversion_matrix(0.0),
        _sensor_data(0.0),
        _cal_state(CalibrationState::NONE)
        {};

    /** Read measured values from sensor.
     */
    Status read(uint32_t &lux, uint32_t &cct, uint8_t &red, uint8_t &green, uint8_t &blue );

    /** Set lighting led status.
     */
    Status led_on(bool state);

    /** Initialize chip.
     */
    void init_chip();

    /** Start measurement cycle.
     */
    void start_measurement();

    /** Perform calibration action.
     */
    Status calibrate(uint8_t cmd);

    /** Get current status of calibration procedure.
     */
    CalibrationState get_calibration_state() { return _cal_state; }

    /** Set color conversion matrix.
     */
    void set_conversion_matrix(const double m[3][3]);

    /** et current color conversion matrix.
     */
    void get_conversion_matrix(double m[3][3]) const;

protected:
    enum PhyReg {
        STATUS_REG  = 0x00,
        WRITE_REG,
        READ_REG
    };

    enum VirtualReg {
        SETUP_CONTROL   = 0x04,
        INT_TIME        = 0x05,
        LED_CONTROL     = 0x07,
        COLOR_X         = 0x14,
        COLOR_Y         = 0x18,
        COLOR_Z         = 0x1C,
        CAL_LUX         = 0x3C,
        CAL_CCT         = 0x3E,
        WRITE_OP        = 0x80
    };

    static const uint32_t   LUXCCT_REG_SIZE = 2;
    static const uint32_t   COLOR_REG_SIZE = 4;

    class StatusReg : public TypedEnum<uint8_t> {
    public:
        enum enum_t {
            RX_PENDING = 0x01,
            TX_PENDING = 0x02
        };

        StatusReg(enum_t init_val) : TypedEnum<uint8_t>(init_val) {}
    };

    class ControlReg : public TypedEnum<uint8_t> {
    public:
        enum enum_t {
            RESET      = 0x80,
            INT_EN     = 0x40,
            GAINx1     = 0x00,
            GAINx4     = 0x10,
            GAINx16    = 0x20,
            GAINx64    = 0x30,
            MODE_0     = 0x00,
            MODE_1     = 0x04,
            MODE_2     = 0x08,
            MODE_3     = 0x0C,
            DATA_RDY   = 0x02
        };

        ControlReg(enum_t init_val) : TypedEnum<uint8_t>(init_val) {}
    };

    class LedControlReg : public TypedEnum<uint8_t> {
    public:
        enum enum_t {
            LED_DRV_12mA   = 0x08,
            LED_DRV_25mA   = 0x18,
            LED_DRV_50mA   = 0x28,
            LED_DRV_100mA  = 0x38,
            LED_INT_1mA    = 0x01,
            LED_INT_2mA    = 0x03,
            LED_INT_4mA    = 0x05,
            LED_INT_8mA    = 0x07,
            LED_OFF        = 0x00
        };

        LedControlReg(enum_t init_val) : TypedEnum<uint8_t>(init_val) {}
    };


    class ColorMatrix {
    protected:
        void set_matrix(const double m[3][3])
        {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    _a[i][j] = m[i][j];
                }
            }
        }

    public:
        ColorMatrix(double v)
        {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    _a[i][j] = (i == j)? v : 0.0;
                }
            }
        }

        ColorMatrix(const double a[3][3])
        {
            set_matrix(a);
        }

        double operator()(int i, int j) const { return _a[i][j]; }

        void set(int i, int j, double v) { _a[i][j] = v; }

        void set_row(int i, double a0, double a1, double a2)
        {
            _a[i][0] = a0; _a[i][1] = a1; _a[i][2] = a2;
        }

        ColorMatrix &operator*=(const ColorMatrix &b)
        {
            double c[3][3];
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    double temp = 0.0;
                    for (int k = 0; k < 3; ++k) {
                        temp += _a[i][k] = b(k, j);
                    }
                    c[i][j] = temp;
                }
            }
            set_matrix(c);
            return *this;
        }

        friend ColorMatrix operator*(ColorMatrix lhs, const ColorMatrix &rhs)
        {
            lhs *= rhs;
            return lhs;
        }

        ColorMatrix &transpose()
        {
            double t;
            t = _a[0][1]; _a[0][1] = _a[1][0]; _a[1][0] = t;
            t = _a[0][2]; _a[0][2] = _a[2][0]; _a[2][0] = t;
            t = _a[1][2]; _a[1][2] = _a[2][1]; _a[2][1] = t;
            return *this;
        }

        double det()
        {
            double t;
            t =  _a[0][0] * _a[0][2] * _a[2][2];
            t += _a[0][1] * _a[1][2] * _a[2][0];
            t += _a[0][2] * _a[1][0] * _a[2][1];
            t -= _a[0][2] * _a[1][1] * _a[2][0];
            t -= _a[0][1] * _a[1][0] * _a[2][2];
            t -= _a[0][0] * _a[1][2] * _a[2][1];
            return t;
        }

        ColorMatrix &invert()
        {
            double d = det();
            if (d != 0.0) {
                double ct[3][3];
                double d = 1.0 / det();

                ct[0][0] =  det2(_a[1][1], _a[1][2], _a[2][1], _a[2][2]) * d;
                ct[1][0] = -det2(_a[1][0], _a[1][2], _a[2][0], _a[2][2]) * d;
                ct[2][0] =  det2(_a[1][0], _a[1][1], _a[2][0], _a[2][1]) * d;
                ct[0][1] = -det2(_a[0][1], _a[0][2], _a[2][1], _a[2][2]) * d;
                ct[1][1] =  det2(_a[0][0], _a[0][2], _a[2][0], _a[2][2]) * d;
                ct[2][1] = -det2(_a[0][0], _a[0][1], _a[2][0], _a[2][1]) * d;
                ct[0][2] =  det2(_a[0][1], _a[0][2], _a[1][1], _a[1][2]) * d;
                ct[1][2] = -det2(_a[0][0], _a[0][2], _a[1][0], _a[1][2]) * d;
                ct[2][2] =  det2(_a[0][0], _a[0][1], _a[1][0], _a[1][1]) * d;
                set_matrix(ct);
            }

            return *this;
        }

    protected:
        double det2(double a, double b, double c, double d)
        {
            return a*d - b*c;
        }

    protected:
        double _a[3][3];
    };

    class ColorVector {
    public:
        ColorVector(double v0, double v1, double v2) {
            _v[0] = v0;
            _v[1] = v1;
            _v[2] = v2;
        }

        double r() { return _v[0]; }
        double x() { return _v[0]; }
        double g() { return _v[1]; }
        double y() { return _v[1]; }
        double b() { return _v[2]; }
        double z() { return _v[2]; }

        double operator[](int index) { return _v[index]; }

        ColorVector &operator*=(const ColorMatrix &m) {
            double temp[3];

            temp[0] = _v[0] * m(0,0) + _v[1] * m(1,0) + _v[2] * m(2,0);
            temp[1] = _v[0] * m(0,1) + _v[1] * m(1,1) + _v[2] * m(2,1);
            temp[2] = _v[0] * m(0,2) + _v[1] * m(1,2) + _v[2] * m(2,2);
            for (int i = 0; i < 3; ++i) {
                _v[i] = temp[i];
            }
            return *this;
        }

        friend ColorVector operator*(ColorVector lhs, const ColorMatrix &rhs)
        {
            lhs *= rhs;
            return lhs;
        }

        ColorVector &normalize() {
            double t = 0.0;
            for (int i = 0; i < 3; ++i) {
                if (_v[i] < 0.0) {
                    _v[i] = 0.0;
                }
                if (_v[i] > t) {
                    t = _v[i];
                }
            }

            if (t > 0.0) {
                for (int i = 0; i < 3; ++i) {
                    _v[i] /= t;
                }
            }
            return *this;
        }

    protected:
        double _v[3];
    };

protected:
    int read_phy_reg(PhyReg reg, uint8_t &value);
    int write_phy_reg(PhyReg reg, uint8_t value);
    int check_buffer_status(uint8_t mask, uint8_t required);
    Status read_register(VirtualReg reg, uint8_t &value);
    Status write_register(VirtualReg reg, uint8_t value);
    Status read_value(VirtualReg reg, uint32_t len, uint32_t &value);
    Status set_calibrate(int color);

protected:
    I2C&                _i2c;
    uint8_t             _address;
    uint16_t            _last_i;
    uint16_t            _last_t;
    ColorMatrix         _conversion_matrix;
    ColorMatrix         _sensor_data;
    CalibrationState    _cal_state;

    static const ColorMatrix _target_data;
};


#endif // AS7261_DRIVER_H_

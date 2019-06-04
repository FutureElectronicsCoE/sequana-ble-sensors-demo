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

#include <mbed.h>
#include "As7261Driver.h"


#define NUM_RETRIES     10


static const double target_def[3][3] = {{135,42,23}, {34,121,45}, {3,101,171}};

const As7261Driver::ColorMatrix As7261Driver::_target_data(target_def);



int As7261Driver::read_phy_reg(PhyReg reg, uint8_t &value)
{
    char reg_addr = reg;

    int status = _i2c.write(_address, &reg_addr, 1);
    if (status == 0) {
        return _i2c.read(_address, (char*)&value, 1);
    }
    return status;
}

int As7261Driver::write_phy_reg(PhyReg reg, uint8_t value)
{
    char reg_addr[2] = {reg, value};

    int status = _i2c.write(_address, reg_addr, 2);
    return status;
}

int As7261Driver::check_buffer_status(uint8_t mask, uint8_t required)
{
    int status = 0;
    int retry_cnt;
    uint8_t reg_val;

    for (retry_cnt = 0; retry_cnt < NUM_RETRIES; ++retry_cnt) {
        status = read_phy_reg(STATUS_REG, reg_val);
        if (status == 0) {
            if ((reg_val & mask) == required) {
                break;
            } else {
                status = 3;
            }
        }
    }
    return status;
}

As7261Driver::Status As7261Driver::read_register(VirtualReg reg, uint8_t &value)
{
    int status;
    uint8_t temp;

    // preventive read of the buffer
    status = check_buffer_status(StatusReg::RX_PENDING, 0);
    if (status) {
        status = read_phy_reg(READ_REG, temp);
    }

    // check that register address can be written
    status = check_buffer_status(StatusReg::TX_PENDING, 0);
    // setup register to read
    if (status == 0) {
        status = write_phy_reg(WRITE_REG, reg);
    }
    // check that data is ready
    if (status == 0) {
        status = check_buffer_status(StatusReg::RX_PENDING, StatusReg::RX_PENDING);
    }
    // read the data
    if (status == 0) {
        status = read_phy_reg(READ_REG, value);
    }

    return status ? STATUS_NOT_READY : STATUS_OK;
}


As7261Driver::Status As7261Driver::write_register(VirtualReg reg, uint8_t value)
{
    int status;

    // check that register address can be written
    status = check_buffer_status(StatusReg::TX_PENDING, 0);
    // setup register to write
    if (status == 0) {
        status = write_phy_reg(WRITE_REG, reg | WRITE_OP);
    }
    // check that buffer is ready
    if (status == 0) {
        status = check_buffer_status(StatusReg::TX_PENDING, 0);
    }
    // write the data
    if (status == 0) {
        status = write_phy_reg(WRITE_REG, value);
    }

    return status ? STATUS_NOT_READY : STATUS_OK;
}


As7261Driver::Status As7261Driver::set_calibrate(int color)
{
    uint32_t    val;
    double      x, y, z;
    Status      status;

    // read color coefficient X
    if ((status = read_value(COLOR_X, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    x = *reinterpret_cast<float*>(&val);
    printf("as7261: color X=0x%lx (%f)\n", val, x);
    x = x < 0.0 ? 0.0 : x;

    // read color coefficient Y
    if ((status = read_value(COLOR_Y, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    y = *reinterpret_cast<float*>(&val);
    printf("as7261: color Y=0x%lx (%f)\n", val, y);
    y = y < 0.0 ? 0.0 : y;

    // read color coefficient Z
    if ((status = read_value(COLOR_Z, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    z = *reinterpret_cast<float*>(&val);
    printf("as7261: color Z=0x%lx (%f)\n", val, z);
    z = z < 0.0 ? 0.0 : z;

    _sensor_data.set_row(color, x, y, z);
}


As7261Driver::Status As7261Driver::calibrate(uint8_t cmd)
{
    Status status = STATUS_ERROR;

    if (CalibrationCommand::is_value_valid(cmd)) {
        CalibrationCommand command(static_cast<CalibrationCommand::enum_t>(cmd));

        switch (command) {
            case CalibrationCommand::NOP:
                status = STATUS_OK;
                break;

            case CalibrationCommand::CAL_RED:
            case CalibrationCommand::CAL_GREEN:
            case CalibrationCommand::CAL_BLUE:
                status = set_calibrate(command - CalibrationCommand::CAL_RED);
                if (status == STATUS_OK) {
                    switch (cmd) {
                        case CalibrationCommand::CAL_RED:
                            _cal_state |= CalibrationState::CAL_RED;
                            break;
                        case CalibrationCommand::CAL_GREEN:
                            _cal_state |= CalibrationState::CAL_GREEN;
                            break;
                        case CalibrationCommand::CAL_BLUE:
                            _cal_state |= CalibrationState::CAL_BLUE;
                            break;
                    }
                }
                break;

            case CalibrationCommand::SET:
                if (_cal_state.colors_ready()) {
                    ColorMatrix calib_data(_target_data);

                    calib_data.transpose();

                    if (_sensor_data.det() != 0.0) {
                        _conversion_matrix = _sensor_data.invert() * calib_data;
                        _cal_state |= CalibrationState::COMPLETED;
                        status = STATUS_OK;
                    }
                }
                break;;

            case CalibrationCommand::RESET:
                _cal_state = CalibrationState::NONE;
                status = STATUS_OK;
                break;
        }
    }

    if (status != STATUS_OK) {
        _cal_state |= CalibrationState::ERROR;
    }
    return status;
}


void As7261Driver::set_conversion_matrix(const double m[3][3]) {
    for (int i = 0; i < 3; ++i) {
        _conversion_matrix.set_row(i, m[i][0], m[i][1], m[i][2]);
    }
    // We keep matrix transposed internally, as our implementation
    // of color transform has multiplication arguments switched
    // ( xyz * M instead of M * xyzT ).
    _conversion_matrix.transpose();
    _cal_state |= CalibrationState::COMPLETED;
}


void As7261Driver::get_conversion_matrix(double m[3][3]) const {
    // Transpose matrix while copying.
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m[i][j] = _conversion_matrix(j, i);
        }
    }
}


void As7261Driver::init_chip(void)
{
    write_register(SETUP_CONTROL, ControlReg::RESET);
    for(uint32_t i = 0; i < 100000; i++);

    write_register(INT_TIME, 100);
    for(uint32_t i = 0; i < 100000; i++);

    write_register(SETUP_CONTROL, ControlReg::MODE_3| ControlReg::GAINx16);
    for(uint32_t i = 0; i < 10000; i++);

    led_on(true);
}


As7261Driver::Status As7261Driver::read_value(VirtualReg reg, uint32_t len, uint32_t & value)
{
    Status      status;
    uint8_t     byte;
    uint32_t    val;
    uint32_t    i;

    val = 0;
    for (i = 0; i < len; ++i) {
        status = read_register((VirtualReg)(reg + i), byte);
        if (status != STATUS_OK) {
            return status;
        }
        val = (val << 8) | byte;
    }
    value = val;

    return STATUS_OK;
}

As7261Driver::Status As7261Driver::read(uint32_t &lux, uint32_t &cct, uint8_t &red, uint8_t &green, uint8_t& blue)
{
    Status      status;
    uint8_t     byte;
    uint32_t    val;
    double      x, y, z;

    // check data is ready
    status = read_register(SETUP_CONTROL, byte);
    if (status != STATUS_OK) {
        return status;
    }
    if ((byte & ControlReg::DATA_RDY) == 0) {
        return STATUS_NOT_READY;
    }

    // read color coefficient X
    if ((status = read_value(COLOR_X, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    x = *reinterpret_cast<float*>(&val);
    printf("as7261: color X=0x%lx (%f)\n", val, x);
    x = x < 0.0 ? 0.0 : x;

    // read color coefficient Y
    if ((status = read_value(COLOR_Y, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    y = *reinterpret_cast<float*>(&val);
    printf("as7261: color Y=0x%lx (%f)\n", val, y);
    y = y < 0.0 ? 0.0 : y;

    // read color coefficient Z
    if ((status = read_value(COLOR_Z, COLOR_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    z = *reinterpret_cast<float*>(&val);
    printf("as7261: color Z=0x%lx (%f)\n", val, z);
    z = z < 0.0 ? 0.0 : z;

    // read LUX
    if ((status = read_value(CAL_LUX, LUXCCT_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    lux = val;

    // read CCT
    if ((status = read_value(CAL_CCT, LUXCCT_REG_SIZE, val)) != STATUS_OK) {
        return status;
    }
    cct = val;

    // Convert color from CIEXYZ into RGB color space
    ColorVector xyz(x, y, z);
    ColorVector rgb = xyz * _conversion_matrix;

    double r =  rgb.r();
    double g =  rgb.g();
    double b =  rgb.b();

    printf("as7261: RGB [%8f %8f %8f]\n", rgb.r(), rgb.g(), rgb.b());
    // Normalize to maximum of 1.0
    rgb.normalize();

    red = rgb.r() * 255 + 0.5;
    green = rgb.g() * 255 + 0.5;
    blue = rgb.b() * 255 + 0.5;

    double sum = x + y + z;
    printf("as7261: xyY color: x=%3.2f y=%3.2f Y=%f\n", x/sum, y/sum, y);
    printf("as7261: lux=0x%04lx, cct=0x%04lx, r=%u, g=%u, b=%u\n", lux, cct, red, green, blue);
    return STATUS_OK;
}


As7261Driver::Status As7261Driver::led_on(bool status)
{
    return write_register(LED_CONTROL, status ? LedControlReg::LED_DRV_12mA : LedControlReg::LED_OFF);
}

void As7261Driver::start_measurement(void)
{
    write_register(SETUP_CONTROL, ControlReg::MODE_3| ControlReg::GAINx16);
}



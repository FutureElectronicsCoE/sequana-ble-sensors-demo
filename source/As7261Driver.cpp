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


void As7261Driver::init_chip(void)
{
    write_register(SETUP_CONTROL, ControlReg::RESET);
    for(uint32_t i = 0; i < 100000; i++);

    write_register(SETUP_CONTROL, ControlReg::MODE_1| ControlReg::GAINx16);
    for(uint32_t i = 0; i < 10000; i++);
}


As7261Driver::Status As7261Driver::read(uint32_t &lux, uint32_t &cct)
{
    Status      status;
    uint8_t     byte;
    uint32_t    val;
    unsigned    i;

    // check data is ready
    status = read_register(SETUP_CONTROL, byte);
    if (status != STATUS_OK) {
        return status;
    }
    if ((byte & ControlReg::DATA_RDY) == 0) {
        return STATUS_NOT_READY;
    }

    // read LUX
    val = 0;
    for (i = 0; i < LUXCCT_REG_SIZE; ++i) {
        status = read_register((VirtualReg)(CAL_LUX + i), byte);
        if (status != STATUS_OK) {
            return status;
        }
        val = (val << 8) | byte;
    }
    lux = val;

    // read CCT
    val = 0;
    for (i = 0; i < LUXCCT_REG_SIZE; ++i) {
        status = read_register((VirtualReg)(CAL_CCT + i), byte);
        if (status != STATUS_OK) {
            return status;
        }
        val = (val << 8) | byte;
    }
    cct = val;

//    printf("as7261: lux=0x%04lx, cct=0x%04lx\n", lux, cct);
    return STATUS_OK;
}


As7261Driver::Status As7261Driver::led_on(bool status)
{
    return write_register(LED_CONTROL, status ? LedControlReg::LED_DRV_12mA : LedControlReg::LED_OFF);
}



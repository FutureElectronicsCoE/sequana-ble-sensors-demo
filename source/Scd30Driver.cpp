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
#include "Scd30Driver.h"


#define MAX_DATA_WORDS      6


/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Thu Jan 31 11:59:42 2019
 * by pycrc v0.9.1, https://pycrc.org
 * using the configuration:
 *  - Width         = 8
 *  - Poly          = 0x31
 *  - XorIn         = 0xff
 *  - ReflectIn     = False
 *  - XorOut        = 0x00
 *  - ReflectOut    = False
 *  - Algorithm     = table-driven
 */
#include "Scd30_crc.h"     /* include the header file generated with pycrc */
#include <stdlib.h>
#include <stdint.h>


/**
 * Static table used for the table_driven implementation.
 */
static const crc_t crc_table[16] = {
    0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e
};


static crc_t crc_update(crc_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc >> 4) ^ (*d >> 4);
        crc = crc_table[tbl_idx & 0x0f] ^ (crc << 4);
        tbl_idx = (crc >> 4) ^ (*d >> 0);
        crc = crc_table[tbl_idx & 0x0f] ^ (crc << 4);
        d++;
    }
    return crc & 0xff;
}


static inline uint8_t GET_LSB(uint16_t value)
{
    return (uint8_t)(value & 0xff);
}

static inline uint8_t GET_MSB(uint16_t value)
{
    return (uint8_t)(value >> 8);
}


Scd30Driver::Status Scd30Driver::_write_command(Command command, uint16_t *args, uint8_t num_args)
{
    MBED_ASSERT(num_args < MAX_DATA_WORDS);

    uint32_t len = 0;

    char buffer[2 + 3 * MAX_DATA_WORDS];
    buffer[len++] = GET_MSB(command);
    buffer[len++] = GET_LSB(command);

    for (uint32_t i = 0; i < num_args; ++i) {
        crc_t crc = crc_init();
        buffer[len++] = GET_MSB(args[i]);
        buffer[len++] = GET_LSB(args[i]);
        crc = crc_update( crc, &buffer[len - 2], 2);
        buffer[len++] = crc_finalize(crc);
    }

    return _i2c.write(_address, buffer, len)? STATUS_I2C_ERROR : STATUS_OK;
}


Scd30Driver::Status Scd30Driver::_read_command(Command command, uint16_t *data, uint8_t num_words)
{
    MBED_ASSERT(num_words < MAX_DATA_WORDS);
    char cmd_buffer[2];
    char buffer[3 * MAX_DATA_WORDS] = {0xba, 0xad, 0xca, 0xfe, 0xde,0xbe, 0xce, 0xef};
    Status status = STATUS_OK;

    cmd_buffer[0] = GET_MSB(command);
    cmd_buffer[1] = GET_LSB(command);
    int i2c_status = _i2c.write(_address, cmd_buffer, 2);

    if (i2c_status == 0) {
        i2c_status = _i2c.read(_address, buffer, 3 * num_words);
    }

    /* verify data CRC and copy over the data */
    if (i2c_status == 0) {
//        printf("sdc30_read (0x%04x): rcv data: ", command);
        for (uint32_t i = 0; i < (3 * num_words); i++) {
            printf(" %02x", buffer[i]);
        }
        printf("\n");
        for (uint32_t i = 0; i < num_words; ++i) {
            uint32_t base = i * 3;
            crc_t crc = crc_init();
            crc = crc_update( crc, &buffer[base], 2);
            crc = crc_finalize(crc);
            if (crc != buffer[base + 2]) {
//                printf("scd30: crc_error: data = %02x %02x, cal = %02x, rcv = %02x\n", buffer[base], buffer[base+1], crc, buffer[base + 2]);
                status = STATUS_CRC_ERROR;
                break;
            }
            *data++ = (uint16_t)((buffer[base] << 8) | (uint8_t)buffer[base + 1]);
        }
    } else {
        status = STATUS_I2C_ERROR;
    }

    return status;
}


Scd30Driver::Scd30Driver(I2C& bus, uint8_t address) :
    _i2c(bus), _address(address)
{
};


Scd30Driver::Status Scd30Driver::read(uint32_t &co2)
{
    Status status;
    uint16_t data[2];
    uint32_t val;

    status = _read_command(CMD_GET_DATA_STATUS, data, 1);

    if (status == STATUS_OK) {
        if (data[0] != 0) {
            status = _read_command(CMD_READ_MEASUREMENT, data, 2);
        } else {
            status = STATUS_NOT_READY;
        }
    }

    if (status == STATUS_OK) {
        val = (uint32_t)data[0] << 16 | data[1];
        // convert from float
        co2 = *(float *)&val;
    }

//    printf("scd30: raw=%08lx, co2=%u, status = %d\n", val, co2, status);
    return status;
}


void Scd30Driver::init_chip(void)
{
    Status status;
    uint16_t air_preasure = 0;

    status = _write_command(CMD_SOFT_RESET, &air_preasure, 1);

    wait_ms(50);

    status = _write_command(CMD_START_MEASUREMENT, NULL, 0);

//    printf("sdc30_init: status = %d\n", status);
}


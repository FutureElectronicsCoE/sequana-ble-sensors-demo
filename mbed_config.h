/*
 * mbed SDK
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Automatically generated configuration file.
// DO NOT EDIT, content will be overwritten.

#ifndef __MBED_CONFIG_DATA__
#define __MBED_CONFIG_DATA__

// Configuration parameters
#define ATT_NUM_SIMUL_NTF                                    1           // set by library:cordio
#define ATT_NUM_SIMUL_WRITE_CMD                              1           // set by library:cordio
#define BLE_FEATURE_EXTENDED_ADVERTISING                     1           // set by library:ble
#define BLE_FEATURE_GATT_CLIENT                              1           // set by library:ble
#define BLE_FEATURE_GATT_SERVER                              1           // set by library:ble
#define BLE_FEATURE_PERIODIC_ADVERTISING                     1           // set by library:ble
#define BLE_FEATURE_PHY_MANAGEMENT                           1           // set by library:ble
#define BLE_FEATURE_PRIVACY                                  1           // set by library:ble
#define BLE_FEATURE_SECURE_CONNECTIONS                       1           // set by library:ble
#define BLE_FEATURE_SECURITY                                 1           // set by library:ble
#define BLE_FEATURE_SIGNING                                  1           // set by library:ble
#define BLE_FEATURE_WHITELIST                                1           // set by library:ble
#define BLE_ROLE_BROADCASTER                                 1           // set by library:ble
#define BLE_ROLE_CENTRAL                                     1           // set by library:ble
#define BLE_ROLE_OBSERVER                                    1           // set by library:ble
#define BLE_ROLE_PERIPHERAL                                  1           // set by library:ble
#define CY_CLK_HFCLK0_FREQ_HZ                                100000000UL // set by target:FUTURE_SEQUANA
#define CY_CLK_PERICLK_FREQ_HZ                               50000000UL  // set by target:FUTURE_SEQUANA
#define CY_CLK_SLOWCLK_FREQ_HZ                               50000000UL  // set by target:FUTURE_SEQUANA
#define DM_CONN_MAX                                          3           // set by library:cordio
#define DM_NUM_ADV_SETS                                      3           // set by library:cordio
#define DM_NUM_PHYS                                          3           // set by library:cordio
#define DM_SYNC_MAX                                          1           // set by library:cordio
#define L2C_COC_CHAN_MAX                                     8           // set by library:cordio
#define L2C_COC_REG_MAX                                      4           // set by library:cordio
#define MBED_CONF_CORDIO_DESIRED_ATT_MTU                     23          // set by library:cordio
#define MBED_CONF_CORDIO_MAX_PREPARED_WRITES                 4           // set by library:cordio
#define MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE             256         // set by library:drivers
#define MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE             256         // set by library:drivers
#define MBED_CONF_EVENTS_PRESENT                             1           // set by library:events
#define MBED_CONF_EVENTS_SHARED_DISPATCH_FROM_APPLICATION    0           // set by library:events
#define MBED_CONF_EVENTS_SHARED_EVENTSIZE                    768         // set by library:events
#define MBED_CONF_EVENTS_SHARED_HIGHPRIO_EVENTSIZE           256         // set by library:events
#define MBED_CONF_EVENTS_SHARED_HIGHPRIO_STACKSIZE           1024        // set by library:events
#define MBED_CONF_EVENTS_SHARED_STACKSIZE                    2048        // set by library:events
#define MBED_CONF_EVENTS_USE_LOWPOWER_TIMER_TICKER           0           // set by library:events
#define MBED_CONF_PLATFORM_CRASH_CAPTURE_ENABLED             0           // set by library:platform
#define MBED_CONF_PLATFORM_CTHUNK_COUNT_MAX                  8           // set by library:platform
#define MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE          115200      // set by application[*]
#define MBED_CONF_PLATFORM_ERROR_ALL_THREADS_INFO            0           // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_FILENAME_CAPTURE_ENABLED    0           // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_HIST_ENABLED                0           // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_HIST_SIZE                   4           // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_REBOOT_MAX                  1           // set by library:platform
#define MBED_CONF_PLATFORM_FATAL_ERROR_AUTO_REBOOT_ENABLED   0           // set by library:platform
#define MBED_CONF_PLATFORM_FORCE_NON_COPYABLE_ERROR          0           // set by library:platform
#define MBED_CONF_PLATFORM_MAX_ERROR_FILENAME_LEN            16          // set by library:platform
#define MBED_CONF_PLATFORM_POLL_USE_LOWPOWER_TIMER           0           // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_BAUD_RATE                   115200      // set by application[*]
#define MBED_CONF_PLATFORM_STDIO_BUFFERED_SERIAL             0           // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_CONVERT_NEWLINES            0           // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_CONVERT_TTY_NEWLINES        0           // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_FLUSH_AT_EXIT               1           // set by library:platform
#define MBED_CONF_PLATFORM_USE_MPU                           1           // set by library:platform
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE                512         // set by library:rtos
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE_DEBUG_EXTRA    0           // set by library:rtos
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE_TICKLESS_EXTRA 256         // set by library:rtos
#define MBED_CONF_RTOS_MAIN_THREAD_STACK_SIZE                4096        // set by library:rtos
#define MBED_CONF_RTOS_PRESENT                               1           // set by library:rtos
#define MBED_CONF_RTOS_THREAD_STACK_SIZE                     4096        // set by library:rtos
#define MBED_CONF_RTOS_TIMER_THREAD_STACK_SIZE               768         // set by library:rtos
#define MBED_CONF_TARGET_BOOT_STACK_SIZE                     0x400       // set by library:rtos[*]
#define MBED_CONF_TARGET_DEEP_SLEEP_LATENCY                  0           // set by target:Target
#define MBED_CONF_TARGET_MPU_ROM_END                         0x0fffffff  // set by target:Target
#define MBED_CONF_TARGET_TICKLESS_FROM_US_TICKER             0           // set by target:Target
#define SEC_CCM_CFG                                          1           // set by library:cordio
#define SMP_DB_MAX_DEVICES                                   3           // set by library:cordio
// Macros
#define WSF_MS_PER_TICK                                      1           // defined by library:cordio
#define _RTE_                                                            // defined by library:rtos

#endif

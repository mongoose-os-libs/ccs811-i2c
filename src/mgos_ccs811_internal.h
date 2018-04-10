/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "mgos.h"
#include "mgos_i2c.h"
#include "mgos_ccs811.h"
#include <math.h>

#define MGOS_SHT31_DEFAULT_I2CADDR               (0x5A)

// Registers
#define MGOS_CCS811_REG_STATUS                   (0x00)
#define MGOS_CCS811_REG_MEAS_MODE                (0x01)
#define MGOS_CCS811_REG_ALG_RESULT_DATA          (0x02)
#define MGOS_CCS811_REG_RAW_DATA                 (0x03)
#define MGOS_CCS811_REG_ENV_DATA                 (0x05)
#define MGOS_CCS811_REG_NTC                      (0x06)
#define MGOS_CCS811_REG_THRESHOLDS               (0x10)
#define MGOS_CCS811_REG_BASELINE                 (0x11)
#define MGOS_CCS811_REG_HW_ID                    (0x20)
#define MGOS_CCS811_REG_HW_VERSION               (0x21)
#define MGOS_CCS811_REG_FW_BOOT_VERSION          (0x23)
#define MGOS_CCS811_REG_FW_APP_VERSION           (0x24)
#define MGOS_CCS811_REG_ERROR_ID                 (0xE0)
#define MGOS_CCS811_REG_SW_RESET                 (0xFF)

// Bootloader registers
#define MGOS_CCS811_BOOTLOADER_REG_APP_ERASE     (0xF1)
#define MGOS_CCS811_BOOTLOADER_REG_APP_DATA      (0xF2)
#define MGOS_CCS811_BOOTLOADER_REG_APP_VERIFY    (0xF3)
#define MGOS_CCS811_BOOTLOADER_REG_APP_START     (0xF4)

// Status register bits
#define MGOS_CCS811_STATUS_ERR                   (0x01)
#define MGOS_CCS811_STATUS_DATA_READY            (0x08)
#define MGOS_CCS811_STATUS_APP_VALID             (0x10)
#define MGOS_CCS811_STATUS_FW_MODE               (0x80)

// Other defines
#define MGOS_CCS811_HW_ID_CODE                   (0x81)
#define MGOS_CCS811_REF_RESISTOR                 (100000)

#ifdef __cplusplus
extern "C" {
  #endif

struct mgos_ccs811 {
  struct mgos_i2c *        i2c;
  uint8_t                  i2caddr;
  struct mgos_ccs811_stats stats;

  float                    temperature_offset;
  uint16_t                 tvoc;
  uint16_t                 eco2;
};

  #ifdef __cplusplus
}
#endif

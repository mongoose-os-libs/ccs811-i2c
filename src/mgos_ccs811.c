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

#include "mgos.h"
#include "mgos_ccs811_internal.h"
#include "mgos_i2c.h"

// Datasheet:
// https://ams.com/eng/content/download/951091/2269479/file/CCS811_DS000459_4-00.pdf

bool i2c_read_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         size_t n, uint8_t *buf) {
  const bool wres = mgos_i2c_write(conn, addr, &reg, 1, false /* stop */);
#if CS_PLATFORM == CS_P_ESP8266
  // NOTE Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  mgos_usleep(50);
#endif // CS_PLATFORM == CS_P_ESP8266
  const bool rres = mgos_i2c_read(conn, addr, buf, n, true /* stop */);

  return wres && rres;
}

int i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg) {
  uint8_t value;
  if (!i2c_read_reg_n(conn, addr, reg, 1, &value)) {
    return -1;
  }
  return value;
}

// Private functions follow
static bool mgos_ccs811_getStatus(struct mgos_ccs811 *sensor, uint8_t *status) {
  int ret;

  if (!sensor) {
    return false;
  }
  ret = i2c_read_reg_b(sensor->i2c, sensor->i2caddr, MGOS_CCS811_REG_STATUS);
  if (ret < 0) {
    return false;
  }
  *status = (uint8_t)ret;
  return true;
}

static bool mgos_ccs811_getMeasMode(struct mgos_ccs811 *sensor, uint8_t *meas_mode) {
  int ret;

  if (!sensor) {
    return false;
  }
  ret = i2c_read_reg_b(sensor->i2c, sensor->i2caddr, MGOS_CCS811_REG_MEAS_MODE);
  if (ret < 0) {
    return false;
  }
  *meas_mode = (uint8_t)ret;
  return true;
}

// Return true if sensor status register has data-ready set (false upon read failure)
static bool mgos_ccs811_dataready(struct mgos_ccs811 *sensor) {
  uint8_t status;

  if (!mgos_ccs811_getStatus(sensor, &status)) {
    return false;
  }
  return status & MGOS_CCS811_STATUS_DATA_READY;
}

static bool mgos_ccs811_reset(struct mgos_ccs811 *sensor) {
  uint8_t data[5] = { MGOS_CCS811_REG_SW_RESET, 0x11, 0xE5, 0x72, 0x8A };

  if (!sensor) {
    return false;
  }
  return mgos_i2c_write(sensor->i2c, sensor->i2caddr, data, 5, true);
}

// Private functions end

// Public functions follow
struct mgos_ccs811 *mgos_ccs811_create(struct mgos_i2c *i2c, uint8_t i2caddr) {
  struct mgos_ccs811 *sensor;
  int success = 0;
  int ret;

  if (!i2c) {
    return NULL;
  }

  ret = i2c_read_reg_b(i2c, i2caddr, MGOS_CCS811_REG_HW_ID);
  if (ret != MGOS_CCS811_HW_ID_CODE) {
    LOG(LL_ERROR, ("Failed to detect CCS811 at I2C 0x%02x", i2caddr));
    return NULL;
  }

  sensor = calloc(1, sizeof(struct mgos_ccs811));
  if (!sensor) {
    return NULL;
  }

  memset(sensor, 0, sizeof(struct mgos_ccs811));
  sensor->i2caddr = i2caddr;
  sensor->i2c     = i2c;
  sensor->eco2    = 400;

  // Boot the application on CCS811.
  if (!mgos_ccs811_reset(sensor)) {
    LOG(LL_ERROR, ("CCS811 failed to reset device"));
    goto exit;
  }
  mgos_usleep(12000);

  uint8_t cmd = MGOS_CCS811_BOOTLOADER_REG_APP_START;
  mgos_i2c_write(sensor->i2c, sensor->i2caddr, &cmd, 1, true);
  mgos_usleep(72000);

  // Read status (expecting FW_MODE to be set and ERR to be clear)
  uint8_t status = MGOS_CCS811_STATUS_ERR;
  if (!mgos_ccs811_getStatus(sensor, &status)) {
    LOG(LL_ERROR, ("CCS811 failed to get status"));
    goto exit;
  }
  if (!(status & MGOS_CCS811_STATUS_FW_MODE) || (status & MGOS_CCS811_STATUS_ERR)) {
    LOG(LL_ERROR, ("CCS811 invalid firmware mode, and/or status error (0x%02x)", status));
    goto exit;
  }

  // Set Drive Mode (1s samples)
  uint8_t drive_mode = CCS811_DRIVE_MODE_IDLE;
  mgos_usleep(5000);
  if(!mgos_ccs811_setDriveMode(sensor, CCS811_DRIVE_MODE_1SEC)) {
    LOG(LL_ERROR, ("CCS811 mgos_ccs811_setDriveMode() failed"));
    goto exit;
  }
  mgos_usleep(72000);

  if(!mgos_ccs811_getDriveMode(sensor, &drive_mode)) {
    LOG(LL_ERROR, ("CCS811 mgos_ccs811_getDriveMode() failed"));
    goto exit;
  }
  if (drive_mode != CCS811_DRIVE_MODE_1SEC) {
    LOG(LL_ERROR, ("CCS811 failed to set drive mode"));
    goto exit;
  }

  success = 1;
  LOG(LL_INFO, ("CCS811 created at I2C 0x%02x", i2caddr));
exit:
  if (!success) {
    free(sensor);
    sensor = NULL;
  }
  return sensor;
}

bool mgos_ccs811_getDriveMode(struct mgos_ccs811 *sensor, uint8_t *mode) {
  uint8_t meas_mode;

  if (!mgos_ccs811_getMeasMode(sensor, &meas_mode)) {
    return false;
  }
  // bits -- 6:4 DRIVE_MOCE 3: Interrupt enable 2: Int on Threshhold
  meas_mode >>= 4;
  meas_mode  &= 0x07;
  *mode       = meas_mode;

  return true;
}

bool mgos_ccs811_setDriveMode(struct mgos_ccs811 *sensor, enum mgos_ccs811_drive_mode_t mode) {
  uint8_t meas_mode;

  // bits -- 6:4 DRIVE_MOCE 3: Interrupt enable 2: Int on Threshhold
  meas_mode = (mode << 4);
  return mgos_i2c_write_reg_b(sensor->i2c, sensor->i2caddr, MGOS_CCS811_REG_MEAS_MODE, meas_mode);
}

void mgos_ccs811_destroy(struct mgos_ccs811 **sensor) {
  if (!*sensor) {
    return;
  }
  free(*sensor);
  *sensor = NULL;
  return;
}

bool mgos_ccs811_read(struct mgos_ccs811 *sensor) {
  double start = mg_time();

  if (!sensor || !sensor->i2c) {
    return false;
  }

  sensor->stats.read++;

  if (start - sensor->stats.last_read_time < MGOS_CCS811_READ_DELAY) {
    sensor->stats.read_success_cached++;
    return true;
  }
  // Read out sensor data here
  //
  if (!mgos_ccs811_dataready(sensor)) {
    sensor->stats.read_success_cached++;
    return true;
  }

  uint8_t data[8];
  uint8_t cmd = MGOS_CCS811_REG_ALG_RESULT_DATA;

  data[4] = MGOS_CCS811_STATUS_ERR;
  mgos_i2c_write(sensor->i2c, sensor->i2caddr, &cmd, 1, false);
  mgos_i2c_read(sensor->i2c, sensor->i2caddr, data, 8, true);

  // bytes 0-1:eco2 2-3:tvoc 4:status 5:error 6-7: raw_data
  if (data[4] & MGOS_CCS811_STATUS_ERR) {
    LOG(LL_ERROR, ("Read error 0x%02x", data[5]));
    return false;
  }
  sensor->eco2 = ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
  sensor->tvoc = ((uint16_t)data[2] << 8) | ((uint16_t)data[3]);
  LOG(LL_DEBUG, ("eCO2=%u TVOC=%u", sensor->eco2, sensor->tvoc));

  sensor->stats.read_success++;
  sensor->stats.read_success_usecs += 1000000 * (mg_time() - start);
  sensor->stats.last_read_time      = start;
  return true;
}

float mgos_ccs811_get_eco2(struct mgos_ccs811 *sensor) {
  if (!mgos_ccs811_read(sensor)) {
    return NAN;
  }
  return (float)sensor->eco2;
}

float mgos_ccs811_get_tvoc(struct mgos_ccs811 *sensor) {
  if (!mgos_ccs811_read(sensor)) {
    return NAN;
  }
  return (float)sensor->tvoc;
}

bool mgos_ccs811_getStats(struct mgos_ccs811 *sensor, struct mgos_ccs811_stats *stats) {
  if (!sensor || !stats) {
    return false;
  }

  memcpy((void *)stats, (const void *)&sensor->stats, sizeof(struct mgos_ccs811_stats));
  return true;
}

bool mgos_ccs811_i2c_init(void) {
  return true;
}

// Public functions end

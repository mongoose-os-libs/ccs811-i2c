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

#define MGOS_CCS811_READ_DELAY    (2)

#ifdef __cplusplus
extern "C" {
  #endif

struct mgos_ccs811;
struct mgos_ccs811_stats {
  double   last_read_time;       // value of mg_time() upon last call to _read()
  uint32_t read;                 // calls to _read()
  uint32_t read_success;         // successful _read()
  uint32_t read_success_cached;  // calls to _read() which were cached
  // Note: read_errors := read - read_success - read_success_cached
  double   read_success_usecs;   // time spent in successful uncached _read()
};

// Drive modes
enum mgos_ccs811_drive_mode_t {
  CCS811_DRIVE_MODE_IDLE  = 0x00,
  CCS811_DRIVE_MODE_1SEC  = 0x01,
  CCS811_DRIVE_MODE_10SEC = 0x02,
  CCS811_DRIVE_MODE_60SEC = 0x03,
  CCS811_DRIVE_MODE_250MS = 0x04,
};

/*
 * Initialize a CCS811 on the I2C bus `i2c` at address specified in `i2caddr`
 * parameter (default CCS811 is on address 0x5A). The sensor will be polled for
 * validity, upon success a new `struct mgos_ccs811` is allocated and
 * returned. If the device could not be found, NULL is returned.
 */
struct mgos_ccs811 *mgos_ccs811_create(struct mgos_i2c *i2c, uint8_t i2caddr);

/*
 * Destroy the data structure associated with a CCS811 device. The reference
 * to the pointer of the `struct mgos_ccs811` has to be provided, and upon
 * successful destruction, its associated memory will be freed and the pointer
 * set to NULL.
 */
void mgos_ccs811_destroy(struct mgos_ccs811 **sensor);

/*
 * The sensor will be polled for its temperature and humidity data. If the poll
 * has occured in the last `MGOS_CCS811_READ_DELAY` seconds, the cached data is
 * used (so as not to repeatedly poll the bus upon subsequent calls).
 */
bool mgos_ccs811_read(struct mgos_ccs811 *sensor);

/*
 * Set the drive mode of the CCS811 sensor based on the `mode` argument
 * Returns true on success, false otherwise.
 */
bool mgos_ccs811_setDriveMode(struct mgos_ccs811 *sensor, enum mgos_ccs811_drive_mode_t mode);

/*
 * Retrieve the current drive mode (which will be one of `enum mgos_ccs811_drive_mode_t`
 * values into the byte pointed to by `mode`.
 * Returns true on success, false otherwise.
 */
bool mgos_ccs811_getDriveMode(struct mgos_ccs811 *sensor, uint8_t *mode);

/*
 * The sensor will be polled for its effective CO2 data. If the poll
 * has occured in the last `MGOS_CCS811_READ_DELAY` seconds, the cached data is
 * used (so as not to repeatedly poll the bus upon subsequent calls).
 *
 * Returns a value in eCO2 parts per million on success, NAN otherwise.
 */
float mgos_ccs811_get_eco2(struct mgos_ccs811 *sensor);

/*
 * The sensor will be polled for its Volatile Organic Compounds (TVOC) data.
 * If the poll has occured in the last `MGOS_CCS811_READ_DELAY` seconds, the
 * cached data is used (so as not to repeatedly poll the bus upon subsequent
 * calls).
 *
 * Returns a value in TVOC parts per billion on success, NAN otherwise.
 */
float mgos_ccs811_get_tvoc(struct mgos_ccs811 *sensor);

/*
 * Returns the running statistics on the sensor interaction, the user provides
 * a pointer to a `struct mgos_ccs811_stats` object, which is filled in by this
 * call.
 *
 * Upon success, true is returned. Otherwise, false is returned, in which case
 * the contents of `stats` is undetermined.
 */
bool mgos_ccs811_getStats(struct mgos_ccs811 *sensor, struct mgos_ccs811_stats *stats);

/*
 * Initialization function for MGOS -- currently a noop.
 */
bool mgos_ccs811_i2c_init(void);

  #ifdef __cplusplus
}
#endif

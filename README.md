# CCS811 I2C Driver

A Mongoose library for AMS CCS811 integrated circuit.

## Sensor details

The CCS811 is an ultra-low power digital gas sensor solution which integrates
a metal oxide (MOX) gas sensor to detect a wide range of Volatile Organic
Compounds (VOCs) for indoor air quality monitoring with a microcontroller unit
(MCU), which includes an Analog-to-Digital converter (ADC), and an I2C
interface.

CCS811 is based on ams unique micro-hotplate technology which enables a highly
reliable solution for gas sensors, very fast cycle times and a significant
reduction in average power consumption.

The integrated MCU manages the sensor driver modes and measurements. The I2C
digital interface significantly simplifies the hardware and software design,
enabling a faster time to market.

CCS811 supports intelligent algorithms to process raw sensor measurements to
output a TVOC value or equivalent CO2 (eCO2) levels, where the main cause of
VOCs is from humans.

CCS811 supports multiple measurement modes that have been optimized for
low-power consumption during an active sensor measurement and idle mode
extending battery life in portable applications. 

See [datasheet](https://ams.com/eng/content/download/951091/2269479/file/CCS811_DS000459_4-00.pdf)
for implementation details.

A great place to buy a ready made and tested unit is at [Adafruit](https://learn.adafruit.com/adafruit-ccs811-air-quality-sensor/overview).

## Example application

An example program using a timer to read data from the sensor every 5 seconds:

```
#include "mgos.h"
#include "mgos_i2c.h"
#include "mgos_ccs811.h"

static struct mgos_ccs811 *s_ccs811;

static void timer_cb(void *user_data) {
  float eco2, tvoc;

  eco2=mgos_ccs811_get_eco2(s_ccs811);
  tvoc=mgos_ccs811_get_tvoc(s_ccs811);

  LOG(LL_INFO, ("ccs811 eCO2=%.0fppm TVOC=%.0fppb", eco2, tvoc));

  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mgos_i2c *i2c;

  i2c=mgos_i2c_get_global();
  if (!i2c) {
    LOG(LL_ERROR, ("I2C bus missing, set i2c.enable=true in mos.yml"));
  } else {
    s_ccs811=mgos_ccs811_create(i2c, 0x5a); // Default I2C address
    if (s_ccs811) {
      mgos_set_timer(5000, true, timer_cb, NULL);
    } else {
      LOG(LL_ERROR, ("Could not initialize sensor"));
    }
  }
  return MGOS_APP_INIT_SUCCESS;
}
```

# Disclaimer

This project is not an official Google project. It is not supported by Google
and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.

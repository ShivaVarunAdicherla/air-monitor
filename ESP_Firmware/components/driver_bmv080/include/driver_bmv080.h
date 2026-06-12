#include "bmv080_defs.h"
#include <driver/i2c_types.h>
#include <stdint.h>
typedef struct bmv080 bmv080;
struct bmv080 {
  bmv080_handle_t bmv_handle;
  i2c_master_dev_handle_t i2c_handle;
};

/*
 * Provides device handle/struct after initing the driver.
 * Make sure that the i2c_addr is what's configured on the sensor.
 * Returns a status code from the sdk
 */
int16_t bmv080_init(uint8_t i2c_addr, i2c_master_bus_handle_t *bus_handle,
                    bmv080 *device);

/*
 * Starts Measurement, Reads a single packet of data/converted results, Stops
 * Measurement
 */
void bmv080_get_readings(bmv080 *device, float *pm10_mass_conc,
                         float *pm1_mass_conc, float *pm2_5_mass_conc);

/*
 * Closes the device on the driver, and removes the device from I2C Master Bus
 */
void bmv080_close_device(bmv080 *device);

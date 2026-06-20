#pragma once
#include "driver/i2c_types.h"
#include <esp_err.h>
typedef struct sgp41 sgp41;
struct sgp41 {
  i2c_master_dev_handle_t dev_handle;
};

esp_err_t sgp41_init(i2c_master_bus_handle_t bus_handle, uint32_t speed,
                     sgp41 *device);

/*
 * Returns Errors encountered in I2C Communication
 * Must be called atleast once before using this sensor for Measurements
 */
esp_err_t sgp41_execute_conditioning(sgp41 *device, uint16_t default_rh_percent,
                                     uint16_t default_temp_c_deg);

/*
 * Returns Errors encountered in I2C Communication
 * This should be called after running sgp41_execute_conditioning.
 * Else we'll receive a NACK from the sensor leading to I2C error
 * and this function will fill 0xFFFF on the measurements
 */
esp_err_t sgp41_read_raw_measurements(sgp41 *device, uint16_t rh_percent,
                                      uint16_t temp_c_deg, uint16_t *raw_NOX,
                                      uint16_t *raw_VOC);

/*
 * Returns 0 if sensor falied self test, 1 otherwise
 * All errors when communicating with the sensor are ignored in this command
 */
uint8_t sgp41_execute_self_test(sgp41 *device);

/*
 * Returns Errors encountered in I2C Communication
 */
esp_err_t sgp41_turn_heater_off(sgp41 *device);

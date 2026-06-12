#include "include/driver_bmv080.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_log_level.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/i2c_types.h"
#include "include/bmv080.h"
#include "include/bmv080_defs.h"
#include "portmacro.h"
#include <driver/i2c_master.h>
#include <driver/i2c_types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

SemaphoreHandle_t mutex_print = NULL;

struct bmv080 {
  bmv080_handle_t bmv_handle;
  i2c_master_dev_handle_t i2c_handle;
};

int8_t delay_callback(uint32_t time_ms) {
  vTaskDelay(pdMS_TO_TICKS(time_ms));
  return ESP_OK;
}

void enable_external_interrupt(bool enable) {
  // TODO: Interrupts NOT IMPLEMENTED YET
  return;
}

uint32_t get_tick_ms() { return xTaskGetTickCount() * portTICK_PERIOD_MS; }

// Example Thread Safe print from SDK
int thread_safe_printf(const char *const _format, ...) {
  int retVal = 0;
  const int wait_for_mutex = 100 / portTICK_PERIOD_MS;
  va_list args;
  va_start(args, _format);

  if (mutex_print == NULL) {
    return -1;
  }

  if (xSemaphoreTake(mutex_print, wait_for_mutex) == pdTRUE) {
    retVal = vprintf(_format, args);
    xSemaphoreGive(mutex_print);
  } else {
    retVal = -1;
  }

  va_end(args);
  return retVal;
}

// The sercom_handle_t is supposed to be a pointer to the
// i2c_master_dev_handle_t
int8_t i2c_read_16bit(bmv080_sercom_handle_t handle, uint16_t header,
                      uint16_t *payload, uint16_t payload_length) {
  i2c_master_dev_handle_t dev_handle = *(i2c_master_dev_handle_t *)handle;

  // Apparently the MSB of header is a R/W bit
  uint16_t adjusted_header = (header << 1);

  // Convert the header into Big Endian for transmission
  uint8_t header_buf[2] = {(adjusted_header >> 8), adjusted_header & 0x00FF};
  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, header_buf, 2, 25));

  ESP_ERROR_CHECK(
      i2c_master_receive(dev_handle, (uint8_t *)payload, payload_length, 100));
  for (uint16_t index = 0; index < payload_length; index++) {
    payload[index] = (payload[index] >> 8) |
                     (payload[index] << 8); // Shuffle back to little endian
  }

  esp_log_buffer_hex_internal("BMV080 Driver RECEIVED", payload,
                              2 * payload_length, ESP_LOG_INFO);
  return 0;
}

int8_t i2c_write_16bit(bmv080_sercom_handle_t handle, uint16_t header,
                       const uint16_t *payload, uint16_t payload_length) {
  i2c_master_dev_handle_t dev_handle = *(i2c_master_dev_handle_t *)handle;
  uint16_t adjusted_header = header << 1;
  uint16_t payload_rw[payload_length + 1];

  // Convert the header, payload into Big Endian
  payload_rw[0] = (adjusted_header >> 8) | adjusted_header << 8;
  for (uint16_t i = 0; i < payload_length; i++) {
    payload_rw[i + 1] = (payload[i] >> 8) | (payload[i] << 8);
  }

  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, (uint8_t *)payload_rw,
                                      2 * (payload_length + 1), 1000));
  return 0;
}

int16_t bmv080_init(uint8_t i2c_addr, i2c_master_bus_handle_t *bus_handle,
                    bmv080 *device) {
  i2c_device_config_t dev_config = {
      .device_address = i2c_addr,
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .flags.disable_ack_check = false,
      .scl_speed_hz = 100000,
  };

  i2c_master_bus_add_device(*bus_handle, &dev_config, &device->i2c_handle);

  uint16_t status_code =
      bmv080_open(&device->bmv_handle, &device->i2c_handle, i2c_read_16bit,
                  i2c_write_16bit, delay_callback);

  return status_code;
}

void bmv080_read_data(bmv080_output_t output, void *ret_val) {
  bmv080_output_t *data = ret_val;
  memcpy(data, &output, sizeof(bmv080_output_t));

  ESP_LOGI("BMV080 Driver", "Received Data: %d, %d, %f, %f",
           output.is_obstructed, output.is_outside_measurement_range,
           output.runtime_in_sec, output.pm10_mass_concentration);
}

void bmv080_get_readings(bmv080 *device, float *pm10_mass_conc,
                         float *pm1_mass_conc, float *pm2_5_mass_conc) {
  bmv080_start_continuous_measurement(device->bmv_handle);
  vTaskDelay(pdMS_TO_TICKS(1500));
  bmv080_output_t output;
  bmv080_serve_interrupt(device->bmv_handle, bmv080_read_data, &output);
  *pm10_mass_conc = output.pm10_mass_concentration;
  *pm1_mass_conc = output.pm1_mass_concentration;
  *pm2_5_mass_conc = output.pm2_5_mass_concentration;
  vTaskDelay(pdMS_TO_TICKS(50));
  bmv080_stop_measurement(device->bmv_handle);
}

void bmv080_close_device(bmv080 *device) {
  bmv080_close(device->bmv_handle);
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(device->i2c_handle));
  ESP_LOGI("BMV080 Driver", "Closed communication with the Sensor");
  device->bmv_handle = NULL;
  device->i2c_handle = NULL;
}

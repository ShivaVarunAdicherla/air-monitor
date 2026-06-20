#include "driver_sgp41.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_log_level.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/i2c_types.h"
#include <esp_compiler.h>
#include <stdint.h>

// Logging toggle
#define LOG
#ifdef LOG
#define OLOG(x) ESP_LOGI("SGP41 Driver", x)
#define OLOGF(x, y) ESP_LOGI("SGP41 Driver", x, y)
#define OLOGF2(x, y, z) ESP_LOGI("SGP41 Driver", x, y, z)
#else
#define OLOG(x) x
#define OLOGF(x, y) x
#define OLOGF2(x, y, z) x
#endif

esp_err_t sgp41_init(i2c_master_bus_handle_t bus_handle, uint32_t speed,
                     sgp41 *device) {
  i2c_device_config_t config = {
      .device_address = 0x59,
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .scl_speed_hz = speed,
      .scl_wait_us = 20000,
      .flags.disable_ack_check = true,
  };

  OLOG("SGP41 Sensor Init");
  return i2c_master_bus_add_device(bus_handle, &config, &device->dev_handle);
}

uint8_t sgp41_crc(const uint8_t *data, uint8_t count) {
  // We init CRC checks with 0xFF (Pg: 13 of SGP41 Datasheet)
  uint8_t crc = 0xFF;
  const uint8_t CRC_POLY = 0x31;

  for (uint8_t current_byte = 0; current_byte < count; current_byte++) {
    crc ^= data[current_byte];
    for (uint8_t bit = 8; bit > 0; bit--) {
      if (crc & 0x80) // Check MSB
        crc = (crc << 1) ^ CRC_POLY;
      else
        crc = (crc << 1);
    }
  }
  return crc;
}
esp_err_t sgp41_execute_conditioning(sgp41 *device, uint16_t default_rh_percent,
                                     uint16_t default_temp_c_deg) {
  uint8_t buf[8] = {0x26, 0x12};
  unsigned int rh = (unsigned int)default_rh_percent * 65535U;
  unsigned int temp = (default_temp_c_deg + 45) * 65535U;
  rh /= 100;
  temp /= 175;
  default_rh_percent = rh;
  default_temp_c_deg = temp;

  buf[2] = default_rh_percent >> 8;
  buf[3] = default_rh_percent;
  buf[4] = sgp41_crc(buf + 2, 2);
  buf[5] = default_temp_c_deg >> 8;
  buf[6] = default_temp_c_deg;
  buf[7] = sgp41_crc(buf + 5, 2);

  uint8_t rec_buf[3];
  // esp_log_buffer_hex_internal("SGP41 Execute Conditioning", buf, 8,
  // ESP_LOG_INFO);
  esp_err_t error = i2c_master_transmit(device->dev_handle, buf, 8, -1);
  if (unlikely(error != ESP_OK)) {
    OLOG("Execute Conditioning Failed at I2C Transmit");
    return error;
  }

  vTaskDelay(pdMS_TO_TICKS(50));

  error = i2c_master_receive(device->dev_handle, rec_buf, 3, -1);
  if (unlikely(error != ESP_OK)) {
    OLOG("Execute Conditioning Failed at I2C Receive");
    return error;
  }

  // esp_log_buffer_hex_internal("SGP41 Conditioning RAW", rec_buf, 3,
  //                             ESP_LOG_INFO);
  OLOGF("Execute Conditioning: VOC- %hu", *(uint16_t *)rec_buf);
  return error;
}

esp_err_t sgp41_read_raw_measurements(sgp41 *device, uint16_t rh_percent,
                                      uint16_t temp_c_deg, uint16_t *raw_NOX,
                                      uint16_t *raw_VOC) {
  uint32_t rh = rh_percent * 65535;
  uint32_t temp = (temp_c_deg + 45) * 65535;
  rh /= 100;
  temp /= 175;
  rh_percent = rh;
  temp_c_deg = temp;

  uint8_t buf[8] = {
      0x26,       0x19, rh_percent >> 8, rh_percent, 0x00, temp_c_deg >> 8,
      temp_c_deg, 0x00,
  };
  buf[4] = sgp41_crc(buf + 2, 2);
  buf[7] = sgp41_crc(buf + 5, 2);

  esp_err_t error = i2c_master_transmit(device->dev_handle, buf, 8, -1);
  if (unlikely(error != ESP_OK)) {
    OLOG("Execute Conditioning Failed at I2C Transmit");
    *raw_VOC = 0xFFFF;
    *raw_NOX = 0xFFFF;
    return error;
  }

  // esp_log_buffer_hex_internal("SGP41 READ RAW", buf, 8, ESP_LOG_INFO);

  vTaskDelay(pdMS_TO_TICKS(60));

  uint8_t rec_buf[6];
  error = i2c_master_receive(device->dev_handle, rec_buf, 6, -1);
  if (unlikely(error != ESP_OK)) {
    OLOG("Execute Conditioning Failed at I2C Receive");
    *raw_VOC = 0xFFFF;
    *raw_NOX = 0xFFFF;
    return error;
  }

  // esp_log_buffer_hex_internal("SGP41 RAW DATA", rec_buf, 6, ESP_LOG_INFO);

  // TODO: Add CRC checking
  *raw_VOC = ((uint16_t)rec_buf[0] << 8) | rec_buf[1];
  *raw_NOX = ((uint16_t)rec_buf[3] << 8) | rec_buf[4];

  OLOGF2("Read RAW Measurements: VOC- %hu NOX- %hu", *raw_VOC, *raw_NOX);
  return error;
}

// Rarely used func imo, so ESP_ERROR_CHECK is used while returning a uint8_t
// flag
uint8_t sgp41_execute_self_test(sgp41 *device) {
  ESP_ERROR_CHECK(
      i2c_master_transmit(device->dev_handle, (uint8_t *)"\x28\x0E", 2, -1));

  vTaskDelay(pdMS_TO_TICKS(321));

  uint8_t rec_buf[3];
  ESP_ERROR_CHECK(i2c_master_receive(device->dev_handle, rec_buf, 3, -1));
  esp_log_buffer_hex_internal("SGP41 Self Test", rec_buf, 3, ESP_LOG_INFO);

  if (rec_buf[1] & 0x03) {
    OLOGF("Self Test Failed: Received %x", rec_buf[1] & 0x03);
    return 0; // Return 0 if tests failed
  }
  OLOGF("Self Test Passed: Received %x", rec_buf[1] & 0x03);
  return 1;
}

esp_err_t sgp41_turn_heater_off(sgp41 *device) {

  esp_err_t error =
      i2c_master_transmit(device->dev_handle, (uint8_t *)"\x36\x15", 2, -1);
  if (unlikely(error != ESP_OK)) {
    OLOG("Execute Conditioning Failed at I2C Transmit");
    return error;
  }
  OLOG("Turned Off Heater");
  return error;
}

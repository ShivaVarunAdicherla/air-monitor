#include "./scd40.h"
#include "driver/i2c_types.h"
#include "esp_log.h"
#include "hal/i2c_types.h"
#include <driver/i2c_master.h>
#include <stdint.h>

// Logging toggle
#define LOG
#ifdef LOG
#define OLOG(x) ESP_LOGI("SCD40 Driver", x)
#else
#define OLOG(x) x
#endif

// Command List, obtained from pg.no 8 of SCD4X Datasheet. (Big endian)
#define SCD40_START_PERIODIC_MEASUREMENT (uint16_t *)"\x21\xb1"
#define SCD40_START_LOW_POWER_PERIODIC_MEASUREMENT (uint16_t *)"\x21\xac"
#define SCD40_STOP_PERIODIC_MEASUREMENT (uint16_t *)"\x3f\x86"
#define SCD40_GET_DATA_READY_STATUS (uint16_t *)"\xe4\xb8"
#define SCD40_READ_MEASUREMENT (uint16_t *)"\xec\x05"

struct scd40 {
  i2c_master_dev_handle_t dev_handle;
};

void scd40_init(i2c_master_bus_handle_t bus_handle, uint32_t speed,
                scd40 *device) {
  i2c_device_config_t config = {.device_address = 0x62,
                                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                .scl_speed_hz = speed,
                                .flags.disable_ack_check = false};

  ESP_ERROR_CHECK(
      i2c_master_bus_add_device(bus_handle, &config, &device->dev_handle));
}

uint8_t scd40_crc(const uint8_t *data, uint8_t count) {
  // We init CRC checks with 0xFF (Pg: 23 of SCD4X Datasheet)
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

void scd40_send_command(const uint16_t *command, scd40 *device) {
  ESP_ERROR_CHECK(
      i2c_master_transmit(device->dev_handle, (uint8_t *)command, 2, 20));
}

void scd40_read(const uint16_t *command, scd40 *device, uint8_t *buffer,
                uint8_t length) {
  ESP_ERROR_CHECK(i2c_master_transmit_receive(
      device->dev_handle, (uint8_t *)command, 2, buffer, length, 20));
}

void scd40_start_periodic_measurement(scd40 *device) {
  scd40_send_command(SCD40_START_PERIODIC_MEASUREMENT, device);
  OLOG("Starting Periodic Measurement");
}

void scd40_start_low_power_periodic_measurement(scd40 *device) {
  scd40_send_command(SCD40_START_LOW_POWER_PERIODIC_MEASUREMENT, device);
  OLOG("Starting Low Power Periodic Measurement");
}

void scd40_stop_periodic_measurement(scd40 *device) {
  scd40_send_command(SCD40_STOP_PERIODIC_MEASUREMENT, device);
  OLOG("Stopping Periodic Measurement");
}

void scd40_read_measurement(scd40 *device, int32_t *temp_c_deg,
                            int32_t *humidity_rh_percent, uint16_t *co2_ppm) {

  // TODO: Add Define Options to enable checking CRC. (We aren't checking now)

  uint8_t buf[9];
  scd40_read(SCD40_READ_MEASUREMENT, device, buf, 9);
  // Bit hack methods - SCD4X Datasheet pg.no 9
  *temp_c_deg =
      ((21875 * (int32_t)(((uint16_t)buf[3] << 8) | buf[4])) >> 13) - 45000;
  *humidity_rh_percent =
      ((12500 * (int32_t)(((uint16_t)buf[6] << 8) | buf[7])) >> 13);
  *co2_ppm = ((uint16_t)buf[0] << 8) | buf[1];
  OLOG("Attempted to read measurements");
}

uint8_t scd40_get_data_ready_status(scd40 *device) {
  uint8_t buf[3];
  scd40_read(SCD40_GET_DATA_READY_STATUS, device, buf, 3);
  OLOG("Polled for data ready flag");
  return (((uint16_t)buf[0] << 8) | buf[1]) &
         0x7FFF; // SCD4X Datasheet pg.no 16
}

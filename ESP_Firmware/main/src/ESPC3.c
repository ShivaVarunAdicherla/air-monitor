#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "portmacro.h"
#include <esp_log.h>
#include <esp_random.h>
#include <nvs_flash.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "BTHOME_ADV";
static const char *Device_Name = "ESP ADV";
uint8_t ble_addr_type;

void start_adv(void *pvParams) {
  int errCode;
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields adv_fields;
  memset(&adv_fields, 0, sizeof(adv_fields));
  adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  ble_uuid16_t bthomeUUID = BLE_UUID16_INIT(0xFCD2);
  adv_fields.uuids16 = &bthomeUUID;
  adv_fields.uuids16_is_complete = 1;
  adv_fields.num_uuids16 = 1;

  int16_t data = 4500;
  int8_t advbuff[] = {
      // BTHOME UUID FLAG //
      0xD2,
      0xFC,
      // BTHOME ADV_INTERVAL, ADV_ENC, VERSION //
      0x40,
      // BTHOME DATA TAG //
      0x02, // Temperature deg C, scale 0.01
      // Temperature Data //
      data & 0x0F,
      data & 0xF0,
  };

  adv_fields.svc_data_uuid16 = (const uint8_t *)advbuff;
  adv_fields.svc_data_uuid16_len = sizeof(advbuff);
  adv_fields.name = (const uint8_t *)Device_Name;
  adv_fields.name_is_complete = 1;
  adv_fields.name_len = strlen(Device_Name);

  errCode = ble_gap_adv_set_fields(&adv_fields);
  if (errCode != ESP_OK) {
    ESP_LOGE(
        TAG,
        "FAILED: To set the advertisement Fields. Exiting TASK with rc = %d",
        errCode);
    vTaskDelete(NULL);
  }

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  adv_params.itvl_max = 750;
  adv_params.itvl_min = 450;

  while (1) {
    errCode = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER,
                                &adv_params, NULL, NULL);
    if (errCode != ESP_OK) {
      ESP_LOGE(
          TAG,
          "FAILED : Starting the BLE GAP advertisement Server with rc = %d",
          errCode);
      vTaskDelete(NULL);
    }

    vTaskDelay(7500 / portTICK_PERIOD_MS);

    ble_gap_adv_stop();
    data = esp_random();
    printf("Started BroadCasting data = %hd", data);
    advbuff[4] = data & 0x0F;
    advbuff[5] = data & 0xF0;

    ble_gap_adv_set_fields(&adv_fields);
  }
}

void onsync(void) {
  ble_hs_id_infer_auto(0, &ble_addr_type);
  start_adv(NULL);
}

void host_task(void *param) { nimble_port_run(); }

void app_main(void) {
  nvs_flash_init();
  nimble_port_init();
  ble_hs_cfg.sync_cb = onsync;
  nimble_port_freertos_init(host_task);
}

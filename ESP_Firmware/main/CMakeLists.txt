file(GLOB SRC_FILES "./src/*")
idf_component_register(SRCS ${SRC_FILES}
  INCLUDE_DIRS "." REQUIRES esp_driver_gpio bt nvs_flash)

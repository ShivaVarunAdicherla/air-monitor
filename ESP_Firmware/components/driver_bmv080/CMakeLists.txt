idf_component_register(INCLUDE_DIRS "include" SRCS "driver_bmv080.c"
                      REQUIRES esp_driver_i2c esp_driver_gpio)

file(GLOB LIB_PATH "*.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE "${LIB_PATH}")

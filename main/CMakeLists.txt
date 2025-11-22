file(GLOB SRC_FILES
    "main.c"
    "src/*.c"
    "src/ble_nimble/*.c"
)


idf_component_register(SRCS ${SRC_FILES}
                       PRIV_REQUIRES bt wpa_supplicant esp_wifi nvs_flash esp_driver_gptimer esp_driver_gpio
                       INCLUDE_DIRS "./include"
                     )

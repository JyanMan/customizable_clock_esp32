file(GLOB SRC_FILES
    "main.c"
    "src/*.c"
)


idf_component_register(SRCS ${SRC_FILES}
                       INCLUDE_DIRS "./include"
                       PRIV_REQUIRES wpa_supplicant esp_wifi nvs_flash esp_driver_gptimer
                     )

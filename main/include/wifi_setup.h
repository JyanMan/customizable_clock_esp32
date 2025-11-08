#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "esp_err.h"

esp_err_t wifi_full_init();
esp_err_t wifi_start_scan();
esp_err_t wifi_stop_scan();

#endif

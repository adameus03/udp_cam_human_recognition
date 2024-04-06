#include "esp_err.h"
#include "esp_wifi.h"

void network_wifi_start(void);

void network_wifi_stop(void);

esp_err_t network_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);

esp_err_t network_wifi_sta_do_disconnect(void);

void network_wifi_shutdown(void);

//esp_err_t wifi_connect(void);
esp_err_t wifi_connect(char* network_ssid, char* network_psk);
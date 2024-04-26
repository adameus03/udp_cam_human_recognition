#include "misc.h"
#include "esp_mac.h"

void misc_get_netif_mac_wifi(uint8_t* puMac_out) {
    esp_read_mac(puMac_out, ESP_MAC_WIFI_STA);
}
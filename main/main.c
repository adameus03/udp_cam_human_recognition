#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "rtc_wdt.h"
#include "wifi_connect.h"
#include "camau_controller.h"

static const char *TAG = "main";

void app_main(void)
{
    // Disable the watchdog timer
    rtc_wdt_protect_off();
    rtc_wdt_disable();
    //rtc_wd

    ESP_LOGI(TAG, "Starting main");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //vTaskPrioritySet(NULL, 5);//set the priority of the main task to 5 ? 
    ESP_ERROR_CHECK(wifi_connect());
    camau_controller_init();
    //CAMAU is mainly executed on core 1
    camau_controller_run();
}
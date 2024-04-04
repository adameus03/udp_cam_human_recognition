#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h" // ENABLE THIS
#include "esp_event.h"
#include "rtc_wdt.h"
#include "wifi_connect.h" // ENABLE THIS
#include "camau_controller.h" // ENABLE THIS

#include "registration.h"

static const char *TAG = "main";

void app_main(void)
{
    // Disable the watchdog timer
    rtc_wdt_protect_off();
    rtc_wdt_disable();

    ESP_LOGI(TAG, "Starting main");
    //ESP_ERROR_CHECK(nvs_flash_init()); // or maybe do like this: https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/nimble/blehr/main/main.c#L276
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); //?
        err = nvs_flash_init();
    }
        ESP_ERROR_CHECK(err);

    //ESP_ERROR_CHECK(esp_netif_init()); //??? Ble conflict ??? ///!!!!! DISABLE THIS
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //vTaskPrioritySet(NULL, 5);//set the priority of the main task to 5 ? 
    
    ////////////////test_start();
    err = registration_main();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "registration_main exited with success.");
    } else {
        ESP_LOGE(TAG, "registration_main exited with fail.");
    }
    return;

    //////ESP_ERROR_CHECK(wifi_connect());
    //////camau_controller_init();
    //////camau_controller_run(); //CAMAU is mainly executed on core 1
    //////return;
}
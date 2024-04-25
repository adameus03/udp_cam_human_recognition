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

registration_network_state_t app_registration_network_connectivity_check_handler(registration_data_t* pRegistrationData) {
    // Check network connection
    //ffsd//[TODO NOW] Start TCP serv task if successfull???
    return wifi_connect(pRegistrationData->pCharacteristics->wifi_ssid, pRegistrationData->pCharacteristics->wifi_psk) == ESP_OK;
    //return NETWORK_STATE_WIFI_DISCONNECTED;
}

uint32_t app_registration_server_communication_callback(registration_data_t* pRegistrationData) {
    // [TODO] trigger a communication to get cam_id and ckey from server
    return 0;
}

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

    ESP_ERROR_CHECK(esp_netif_init()); //??? Ble conflict ??? ///!!!!! DISABLE THIS
    ESP_LOGI(TAG, "TCP/IP stack initialized."); // CALL wifi_connect() to test network connection <<<<<< [TODO NOW] + check if ble works while netif is initialized 

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //vTaskPrioritySet(NULL, 5);//set the priority of the main task to 5 ? 
    
    registration_data_t registrationData = {};
    /*err = ESP_OK;[debug]*/err = registration_main(&registrationData, app_registration_network_connectivity_check_handler, app_registration_server_communication_callback);
    //ESP_ERROR_CHECK(wifi_connect("ama", "2a0m0o3n")); //[debug]
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "registration_main returned error code [%d]", err);
    } else {
        ESP_LOGI(TAG, "Successfully returned from registration_main");
        camau_controller_init();
        camau_controller_run(); //CAMAU is mainly executed on core 1
    }

    return;
    ////test_start();
    /*err = registration_main();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "registration_main exited with success.");
    } else {
        ESP_LOGE(TAG, "registration_main exited with fail.");
    }
    return;*/

    //////ESP_ERROR_CHECK(wifi_connect());
    //////ESP_LOGI(TAG, "After wifi_connect");//
    //////camau_controller_init();
    //////camau_controller_run(); //CAMAU is mainly executed on core 1
    //////return;
}
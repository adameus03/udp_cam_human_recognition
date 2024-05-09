#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h" // ENABLE THIS
#include "esp_event.h"
#include "rtc_wdt.h"
#include "wifi_connect.h" // ENABLE THIS
#include "camau_controller.h" // ENABLE THIS

#include "registration.h"
#include "server_communications.h"

static const char *TAG = "main";

TaskHandle_t tcp_connection_manage_task_handle = NULL;
TaskHandle_t tcp_app_incoming_request_handler_task_handle = NULL;

registration_network_state_t app_registration_network_connectivity_check_handler(registration_data_t* pRegistrationData, SemaphoreHandle_t semphSync) {
    // Check network connection
    //ffsd//[TODO NOW] Start TCP serv task if successfull???
    esp_err_t wifi_conn_err = wifi_connect(pRegistrationData->pCharacteristics->wifi_ssid, pRegistrationData->pCharacteristics->wifi_psk);
    
    if (wifi_conn_err == ESP_OK) {
        ESP_LOGI(TAG, "Creating task 'tcp_conection_manager'");
        if (pdPASS != xTaskCreate( tcp_connection_manage_task, "tcp_conection_manager", 2304, NULL, 1, &tcp_connection_manage_task_handle)) {
            ESP_LOGE(TAG, "Failed to create the tcp_conection_manager task.");
            exit(EXIT_FAILURE); // [TODO] Does this make esp32 reset or not?
        }
        ESP_LOGI(TAG, "Creating task 'tcp_app_incoming_request_handler'");
        if (pdPASS != xTaskCreate( tcp_app_incoming_request_handler_task, "tcp_app_incoming_request_handler", 2304, NULL, 1, &tcp_app_incoming_request_handler_task_handle)) {
            ESP_LOGE(TAG, "Failed to create the tcp_app_incoming_request_handler task.");
            exit(EXIT_FAILURE);
        }
    }

    ESP_LOGI(TAG, "%s", wifi_conn_err == ESP_OK ? "Netconn check pass" : "Netconn check fail");
    
    return wifi_conn_err == ESP_OK;
    //return NETWORK_STATE_WIFI_DISCONNECTED;
}

typedef struct {
    char* pcUid_in; char** ppcCid_out; char** ppcCkey_in; SemaphoreHandle_t semphSync;
} __registrationServerCommunicationCallback_params_t;

void __registrationServerCommmunicationCallback_task(void* pvParameters) {
    __registrationServerCommunicationCallback_params_t* pParams = pvParameters;
    tcp_app_handle_registration(pParams->pcUid_in, pParams->ppcCid_out, pParams->ppcCkey_in, pParams->semphSync);
    vTaskDelete(NULL);
}

TaskHandle_t registrationServerCommmunication_callback_task_handle = NULL;

char* ___pCamId = NULL; char* ___pCkey = NULL;
uint32_t app_registration_server_communication_callback(registration_data_t* pRegistrationData, SemaphoreHandle_t semphSync) {
    // [TODO] trigger a communication to get cam_id and ckey from server
    // Call tcp_app_handle_registration
    ___pCamId = pRegistrationData->cam_id;
    ___pCkey = pRegistrationData->ckey;
    __registrationServerCommunicationCallback_params_t params = {
        .pcUid_in = pRegistrationData->pCharacteristics->user_id,
        .ppcCid_out = &___pCamId,
        .ppcCkey_in = &___pCkey,
        .semphSync = semphSync
    };

    ESP_LOGI(TAG, "Creating task registrationServerCommmunication_callback");
    if (pdPASS != xTaskCreate( __registrationServerCommmunicationCallback_task, "registrationServerCommmunication_callback", /*2560*/3072, &params, 1, &registrationServerCommmunication_callback_task_handle )) {
        ESP_LOGE(TAG, "Failed to create the registrationServerCommmunication_callback task.");
        exit(EXIT_FAILURE);
    }

    return 0;
}

uint32_t app_registration_welcome_back_callback(registration_data_t* pRegistrationData, SemaphoreHandle_t semphSync) {
    network_wifi_set_had_ever_connected();
    app_tcp_set_mode(APP_TCP_COMM_MODE_REGULAR);
    app_comm_set_credentials(pRegistrationData->cam_id, pRegistrationData->ckey);
    return app_registration_network_connectivity_check_handler(pRegistrationData, NULL);
}

void app_main(void)
{
    // Disable the watchdog timer
    rtc_wdt_protect_off();
    rtc_wdt_disable();

    ESP_LOGI(TAG, "Starting main");
    // Log config
    //esp_log_level_set("server_communications", ESP_LOG_NONE);
    //esp_log_level_set("CAMAU_CONTROLLER", ESP_LOG_NONE);
    //esp_log_level_set("analyser", ESP_LOG_NONE);

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
    app_tcp_set_mode(APP_TCP_COMM_MODE_REGISTRATION);
    /*err = ESP_OK;[debug]*/err = registration_main(&registrationData, 
                                                    (registrationCallback_Function)app_registration_network_connectivity_check_handler, 
                                                    app_registration_server_communication_callback,
                                                    (registrationCallback_Function)app_registration_welcome_back_callback);
    //ESP_ERROR_CHECK(wifi_connect("ama", "2a0m0o3n")); //[debug]
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "registration_main returned error code [%d]", err);
    } else {
        ESP_LOGI(TAG, "Successfully returned from registration_main");
        

        // [TODO] Start general tcp req/resp handler/s using registration data?
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
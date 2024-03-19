////#include "esp_nimble_hci.h" #include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

//#include "nimble"
#include "esp_log.h"
#include "register.h"

static const char* TAG = "register";

/* Stub */
static const struct ble_gatt_svc_def gatt_server_services[] = {
    {
        
    }
};




void hostTaskFn_stub(void* arg) {
    //This is a stub
    ESP_LOGI(TAG, "I'm in the host task stub function");
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Host task stub function heartbeat log");
    }
}

/*void test_config_ble_nimble() {
    
}*/

void on_ble_hs_sync_stub() {
    ESP_LOGI(TAG, "In ble_hs sync stub");
}

void on_ble_hs_reset_stub(int reason) {
    ESP_LOGI(TAG, "In ble_hs reset stub");
}

/**
 * Stub function for testing NimBLE port API
*/
int test_start() {
    //esp_nimble_hci_init();

    int err = 0;

    // Initialize the host and controller stack
    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NimBLE port");
        return ESP_FAIL;
    }
    
    // Initialize the required NimBLE host configuration parameters and callback
    // [TODO]
    
    ble_hs_cfg.sync_cb = on_ble_hs_sync_stub;
    ble_hs_cfg.reset_cb = on_ble_hs_reset_stub;

    // Perform application specific tasks/initialization
    // [TODO]
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    err = ble_gatts_count_cfg(gatt_server_services);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed. Maybe the svcs array contains an invalid resource definition?");
        return ESP_FAIL;
    }
    err = ble_gatts_add_svcs(gatt_server_services);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed. Possible heap exhaustion.");
        return ESP_FAIL;
    }

    err = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set failed.");
    }

    // Run the thread for host stack
    nimble_port_freertos_init( hostTaskFn_stub );
    ESP_LOGI(TAG, "Finished test initialization");
    return ESP_OK;
}
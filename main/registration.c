////#include "esp_nimble_hci.h" #include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

//#include "nimble"
#include "esp_log.h"
#include "registration.h"

#define MIN_WIFI_SSID_LENGTH 1
#define MAX_WIFI_SSID_LENGTH 20
#define MIN_WIFI_PSK_LENGTH 1
#define MAX_WIFI_PSK_LENGTH 40
#define MIN_USER_ID_LENGTH 16
#define MAX_USER_ID_LENGTH 16

static const char* TAG = "registration";

int test_value = 0;
struct __sau_gatt_chr_vals {
    char wifi_ssid[MAX_WIFI_SSID_LENGTH+1];
    char wifi_psk[MAX_WIFI_PSK_LENGTH+1];
    char user_id[MAX_USER_ID_LENGTH+1]; 
    uint8_t network_state;
} sau_gatt_registration_service_chr_values;

static uint8_t sau_ble_hs_id_addr_type;
static uint16_t conn_handle = 0U;

struct __sau_gatt_registration_service_chr_handles {
    uint16_t wifi_ssid_characteristic_handle;
    uint16_t wifi_psk_characteristic_handle;
    uint16_t user_id_characteristic_handle;
    uint16_t network_state_characteristic_handle;
} sau_gatt_registration_service_chr_handles;

static void sau_registration_ble_advertise();

/* https://mynewt.apache.org/latest/tutorials/ble/bleprph/bleprph-sections/bleprph-chr-access.html */
static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

int on_gatt_wifi_ssid_charatecteristic_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "In GATT wifi_ssid characteristic access callback stub");
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t rx_len = 0;
        ESP_LOGI(TAG, "PREVIOUS provided wifi_ssid = {%s}", sau_gatt_registration_service_chr_values.wifi_ssid); //
        int err =  gatt_svr_chr_write(ctxt->om, MIN_WIFI_SSID_LENGTH, MAX_WIFI_SSID_LENGTH, sau_gatt_registration_service_chr_values.wifi_ssid, &rx_len);
        if (err == 0) {
            ESP_LOGI(TAG, "Successfully written characteristic value");
            memset(sau_gatt_registration_service_chr_values.wifi_ssid + rx_len, 0, MAX_WIFI_SSID_LENGTH - rx_len);
            ESP_LOGI(TAG, "Provided wifi_ssid = {%s}", sau_gatt_registration_service_chr_values.wifi_ssid);
        } else {
            ESP_LOGE(TAG, "Failed to write characteristic value");
        }
        return err;
    }
    else return BLE_ATT_OP_ERROR_RSP;
}

int on_gatt_wifi_psk_charatecteristic_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "In GATT wifi_psk characteristic access callback stub");
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t rx_len = 0;
        int err = gatt_svr_chr_write(ctxt->om, MIN_WIFI_PSK_LENGTH, MAX_WIFI_PSK_LENGTH, sau_gatt_registration_service_chr_values.wifi_psk, &rx_len);
        if (err == 0) {
            ESP_LOGI(TAG, "Successfully written characteristic value");
            memset(sau_gatt_registration_service_chr_values.wifi_psk + rx_len, 0, MAX_WIFI_PSK_LENGTH - rx_len);
            ESP_LOGI(TAG, "Provided wifi_psk = {%s}", sau_gatt_registration_service_chr_values.wifi_psk);
        } else {
            ESP_LOGE(TAG, "Failed to write characteristic value");
        }
        return err;
    }
    else return BLE_ATT_OP_ERROR_RSP;
}

int on_gatt_user_id_charatecteristic_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "In GATT user_id characteristic access callback stub");
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t rx_len = 0;
        int err = gatt_svr_chr_write(ctxt->om, MIN_USER_ID_LENGTH, MAX_USER_ID_LENGTH, sau_gatt_registration_service_chr_values.user_id, &rx_len);
        if (err == 0) {
            ESP_LOGI(TAG, "Successfully written characteristic value");
            memset(sau_gatt_registration_service_chr_values.user_id + rx_len, 0, MAX_USER_ID_LENGTH - rx_len);
            ESP_LOGI(TAG, "Provided user_id = {%s}", sau_gatt_registration_service_chr_values.user_id);

            ///
            struct os_mbuf *om = ble_hs_mbuf_from_flat(&sau_gatt_registration_service_chr_values.network_state, 1U);
            
            if (om == NULL) {
                ESP_LOGE(TAG, "Failed to allocate mbuf for GATT notification.");
            } else {
                if (0 != ble_gatts_notify_custom(conn_handle, sau_gatt_registration_service_chr_handles.network_state_characteristic_handle, om)) {
                    ESP_LOGE(TAG, "Failed to notify BLE central.");
                } else {
                    ESP_LOGI(TAG, "Sent GATT notification to BLE central.");
                    sau_gatt_registration_service_chr_values.network_state = !sau_gatt_registration_service_chr_values.network_state;
                }
            }
            ///
        } else {
            ESP_LOGE(TAG, "Failed to write characteristic value");
        }
        return err;
    }
    else return BLE_ATT_OP_ERROR_RSP;
}

int on_gatt_network_state_characteristic_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "In GATT network_state characteristic access callback stub");
    return 0;
}



/* Stub */
static const struct ble_gatt_svc_def gatt_server_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = /*SAU_GATT_REGISTRATION_SERVICE_UUID*/BLE_UUID128_DECLARE(0x82, 0x10, 0x05, 0xa5, 0x11, 0x12, 0x4d, 0xc1, 0x8a, 0x09, 0xa6, 0xfa, 0x67, 0xcb, 0x65, 0x9c),
        .includes = NULL,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = /*SAU_GATT_REGISTRATION_SERVICE_WIFI_SSID_CHARACTERISTIC_UUID*/BLE_UUID128_DECLARE(0xd8, 0xc9, 0x91, 0xaa, 0xa2, 0xbe, 0x48, 0x23, 0x85, 0x1a, 0xf3, 0xbc, 0xf0, 0xdd, 0xcd, 0xc4),
                .access_cb = on_gatt_wifi_ssid_charatecteristic_access,
                //.arg = NULL,
                //.descriptors = NULL,
                .flags = BLE_GATT_CHR_F_WRITE, //WRITE_ENC??
                //.min_key_size = 0U,
                .val_handle = &sau_gatt_registration_service_chr_handles.wifi_ssid_characteristic_handle
            },
            {   
                .uuid = /*SAU_GATT_REGISTRATION_SERVICE_WIFI_PSK_CHARACTERISTIC_UUID*/BLE_UUID128_DECLARE(0xd8, 0x13, 0x2b, 0xb8, 0xf1, 0xe9, 0x4a, 0xac, 0x83, 0x9d, 0x80, 0x70, 0x46, 0xeb, 0x44, 0x4e),
                .access_cb = on_gatt_wifi_psk_charatecteristic_access,
                //.arg = NULL,
                //.descriptors = NULL,
                .flags = BLE_GATT_CHR_F_WRITE, //WRITE_ENC??
                //.min_key_size = 0U,
                .val_handle = &sau_gatt_registration_service_chr_handles.wifi_psk_characteristic_handle
            },
            {
                .uuid = /*SAU_GATT_REGISTRATION_SERVICE_USER_ID_CHARACTERISTIC_UUID*/BLE_UUID128_DECLARE(0xfc, 0x20, 0xcd, 0xdd, 0x14, 0x9c, 0x47, 0xa4, 0xa5, 0x55, 0x4f, 0xbe, 0x35, 0x04, 0xb4, 0xb8),
                .access_cb = on_gatt_user_id_charatecteristic_access,
                //.arg = NULL,
                //.descriptors = NULL,
                .flags = BLE_GATT_CHR_F_WRITE, //WRITE_ENC??
                //.min_key_size = 0U,
                .val_handle = &sau_gatt_registration_service_chr_handles.user_id_characteristic_handle
            },
            {
                .uuid = /*SAU_GATT_REGISTRATION_SERVICE_NETWORK_STATE_CHARACTERISTIC_UUID*/BLE_UUID128_DECLARE(0xc7, 0x2a, 0x4c, 0x9c, 0xd9, 0x3e, 0x41, 0x41, 0xa9, 0x68, 0xd8, 0x86, 0x3f, 0x6b, 0x0a, 0xdd),
                .access_cb = on_gatt_network_state_characteristic_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &sau_gatt_registration_service_chr_handles.network_state_characteristic_handle
            },
            {
                0, /* VERY IMPORTANT!!! - it denotes the end of characteristics*/
            }
        }
    },
    {
        0, /* VERY IMPORTANT!!! - it denotes the end of services*/
    }
};




void hostTaskFn_stub(void* arg) {
    //This is a stub
    ESP_LOGI(TAG, "I'm in the host task stub function");


    nimble_port_run();
    nimble_port_freertos_deinit(); //This function will return only when nimble_port_stop() is executed
    
    /*while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Host task stub function heartbeat log");
    }*/
}

/*void test_config_ble_nimble() {
    
}*/

/**
 * @attention Might need additional event handling in future
*/
int ble_gap_event_handler(struct ble_gap_event *event, void *arg) {
    ESP_LOGI(TAG, "In on_ble_gap_adv_start callback.");
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Event --- BLE_GAP_EVENT_CONNECT");
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "It's a successful connection");
                conn_handle = event->connect.conn_handle;
            } else {
                ESP_LOGI(TAG, "It's a failed connection");
                sau_registration_ble_advertise();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Event --- BLE_GAP_EVENT_DISCONNECT");
            sau_registration_ble_advertise();
            break;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Event --- BLE_GAP_EVENT_ADV_COMPLETE");
            break;
        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Event --- BLE_GAP_EVENT_SUBSCRIBE");
            break;
        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "Event --- BLE_GAP_EVENT_MTU");
            break;
        default:
            ESP_LOGI(TAG, "Event --- [Other Event (%u)]", event->type);
            break;
    }
    return 0;
}

static void sau_registration_ble_advertise() {
    ESP_LOGI(TAG, "Preparing to start GAP advertising...");
    int err = 0;
    struct ble_hs_adv_fields adv_fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .name = (uint8_t*)DEVICE_NAME,
        .name_len = DEVICE_NAME_LENGTH,
        .name_is_complete = 1, //?
        .tx_pwr_lvl_is_present = 1,
        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO
        //...
    };
    err = ble_gap_adv_set_fields(&adv_fields);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed. Already advertising or too large data to fit in an advertisement, or other issue?");
        return;
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND, /* undirected-connectable; Vol 3, Part C, Section 9.3.4 and Vol 6, Part B, Section 2.3.1.1. */
        .disc_mode = BLE_GAP_DISC_MODE_GEN /* general-discoverable; 3.C.9.2.4 */
    };
    
    ESP_LOGI(TAG, "Starting GAP advertising...");
    err = ble_gap_adv_start(sau_ble_hs_id_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_handler, NULL);
    
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed.");
        return;
    }
    ESP_LOGI(TAG, "Successfully started GAP advertising!");
    return;
}

void on_ble_hs_sync_stub() {
    ESP_LOGI(TAG, "In ble_hs sync stub");

    int err = 0;
    err = ble_hs_id_infer_auto(BLE_HS_F_USE_PRIVATE_ADDR, &sau_ble_hs_id_addr_type);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed. Non-suitable BLE host address might be one of the reasons.");
        return;
    }
    uint8_t  addr_val_buf[6] = {0};

    int is_nrpa = 0;
    err = ble_hs_id_copy_addr(sau_ble_hs_id_addr_type, addr_val_buf, &is_nrpa);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ble_hs_id_copy_addr failed.");
        return;
    }
    switch (is_nrpa) {
        case 0:
            ESP_LOGI(TAG, "ble_hs_id_copy_addr determined that the BLE host address is NOT a non-resolvable private address (NRPA)");
            break;
        case 1:
            ESP_LOGI(TAG, "ble_hs_id_copy_addr determined that the BLE host address IS a non-resolvable private address (NRPA)");
            break;
        default: //Shouldn't happen
            ESP_LOGE(TAG, "ble_hs_id_copy_addr determined that the BLE host address is of UNKNOWN TYPE (!)");
            return; 
    }

    ESP_LOGI(TAG, "The BLE host addr of the device is %02x:%02x:%02x:%02x:%02x:%02x", addr_val_buf[5], addr_val_buf[4], addr_val_buf[3], addr_val_buf[2], addr_val_buf[1], addr_val_buf[0]);

    /* Begin GAP Advertising */
    sau_registration_ble_advertise();
}

void on_ble_hs_reset_stub(int reason) {
    ESP_LOGI(TAG, "In ble_hs reset stub");
}

/**
 * Stub function for testing NimBLE port API
*/
int test_start() {
    //esp_nimble_hci_init();
    //sau_gatt_registration_service_chr_handles.network_state_characteristic_handle = NETWORK_STATE_WIFI_DISCONNECTED;
    //memset(sau_gatt_registration_service_chr_values.wifi_ssid, 0, MAX_WIFI_SSID_LENGTH);
    //memset(sau_gatt_registration_service_chr_values.wifi_psk, 0, MAX_WIFI_PSK_LENGTH);
    //memset(sau_gatt_registration_service_chr_values.user_id, 0, MAX_USER_ID_LENGTH);

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
    ESP_LOGI(TAG, "Successfully returned from ble_svc_gap_init.");
    ble_svc_gatt_init();
    ESP_LOGI(TAG, "Successfully returned from ble_svc_gatt_init.");

    err = ble_gatts_count_cfg(gatt_server_services);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed. Maybe the svcs array contains an invalid resource definition?");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully returned from ble_gatts_count_cfg.");
    err = ble_gatts_add_svcs(gatt_server_services);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed. Possible heap exhaustion.");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully returned from ble_gatts_add_svcs.");

    err = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (err != 0) {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set failed.");
    }
    ESP_LOGI(TAG, "Successfully returned from ble_svc_gap_device_name_set.");
    
    // Run the thread for host stack
    nimble_port_freertos_init( hostTaskFn_stub );
    ESP_LOGI(TAG, "Finished test initialization");
    return ESP_OK;
}
////#include "esp_nimble_hci.h" #include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "nvs_flash.h" //
#include "esp_random.h"

//#include "nimble"
#include "esp_log.h"
#include "registration.h"
#include "sau_heap_debug.h"

static const char* TAG = "registration"; 

static SemaphoreHandle_t s_semph_get_registration = NULL;
static SemaphoreHandle_t s_semph_shutdown_ble = NULL;

static uint8_t sau_ble_hs_id_addr_type;
static uint16_t conn_handle = 0U;

struct __sau_gatt_registration_service_chr_handles {
    uint16_t wifi_ssid_characteristic_handle;
    uint16_t wifi_psk_characteristic_handle;
    uint16_t user_id_characteristic_handle;
    uint16_t network_state_characteristic_handle;
} sau_gatt_registration_service_chr_handles;

struct __sau_gatt_chr_vals sau_gatt_registration_service_chr_values;

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
            xSemaphoreGive(s_semph_get_registration); // unblock task which triggers wifi connectivity verification
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

/**
 * @note sau_gatt_registration_service_chr_values.network_state needs to be first set!
*/
void notify_ble_central() {
    ///
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&sau_gatt_registration_service_chr_values.network_state, 1U);
    
    if (om == NULL) {
        ESP_LOGE(TAG, "Failed to allocate mbuf for GATT notification.");
    } else {
        if (0 != ble_gatts_notify_custom(conn_handle, sau_gatt_registration_service_chr_handles.network_state_characteristic_handle, om)) {
            ESP_LOGE(TAG, "Failed to notify BLE central.");
        } else {
            ESP_LOGI(TAG, "Sent GATT notification to BLE central.");
        }
    }
    ///
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


    nimble_port_run(); //This function will return only when nimble_port_stop() is executed
    ESP_LOGI(TAG, "nimble_port_run returned");

    xSemaphoreGive(s_semph_shutdown_ble);
    nimble_port_freertos_deinit();
    //ESP_LOGI(TAG, "after nimble_port_freertos_deinit");
    /*esp_err_t err = nimble_port_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_deinit failed.");
        exit(EXIT_FAILURE);
    }*/

    /*while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Host task stub function heartbeat log");
    }*/
}

/*void test_config_ble_nimble() {
    
}*/

static unsigned char is_ble_shutting_down = 0U;
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
            if (is_ble_shutting_down == 0U) {
                sau_registration_ble_advertise();
            }
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
 * Activate BLE interface using NimBLE port API
*/
int ble_interface_start() {
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

/**
 * Deactivate BLE interface with NimBLE port API
*/
int ble_interface_stop() {
    is_ble_shutting_down = 1U;

    s_semph_shutdown_ble = xSemaphoreCreateBinary();
    if (s_semph_get_registration == NULL) {
        ESP_LOGE(TAG, "No memory for BLE shutdown semaphore");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "BLE shutdown semaphore created successfully");

    int nimblePortStopRetVal = nimble_port_stop();
    if (nimblePortStopRetVal != 0) {
        ESP_LOGE(TAG, "nimble_port_stop failed with code [%d].", nimblePortStopRetVal);
        exit(EXIT_FAILURE);
    }
    esp_err_t err = nimble_port_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_deinit failed.");
        return 1;
    }
    ESP_LOGI(TAG, "Waiting for complete BLE shutdown...");
    xSemaphoreTake(s_semph_shutdown_ble, portMAX_DELAY);
    vSemaphoreDelete(s_semph_shutdown_ble);
    return 0;
}


//////////////////////
#include "esp_vfs_fat.h"
//#include "diskio_impl.h" 

#define REGISTRATION_FILE_PATH "/spiflash/registra.dat"

/*struct __fatfs_helpers {
    FATFS* pVfsFat;
    ff_diskio_impl_t ffDiskIoImpl;
};*/

/*esp_err_t __registration_fatfs_init(struct __fatfs_helpers* pFatfsHelpers) {
    //nvs_open()
    //nvs_open_from_partition()

    pFatfsHelpers->pVfsFat = 0;
    esp_err_t err = esp_vfs_fat_register("/spiflash", "fatfs", 1LLU, &pFatfsHelpers->pVfsFat);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failure while calling esp_vfs_fat_register (ESP_ERR_INVALID_STATE). Maybe it was already called earlier?");
        } else if (err == ESP_ERR_NO_MEM) {
            ESP_LOGE(TAG, "Failure while calling esp_vfs_fat_register (ESP_ERR_NO_MEM).");
        } else {
            ESP_LOGE(TAG, "Failure while calling esp_vfs_fat_register (Other error).");
        }
        return ESP_FAIL;
    }

    pFatfsHelpers->ffDiskIoImpl = (ff_diskio_impl_t){};
    ff_diskio_register(0x0U, &pFatfsHelpers->ffDiskIoImpl); // OK??
    FRESULT fRes = f_mount(pFatfsHelpers->pVfsFat, "fatfs", 0U);
    if (fRes != FR_OK) {
        ESP_LOGE(TAG, "Failure while calling f_mount.");
        return ESP_FAIL;
    }
    //f_fdisk()
    //f_mkfs()
    ESP_LOGI(TAG, "Diskio driver registered for pdrv=0x0");
    return ESP_OK;
}*/

/*esp_err_t __registration_fatfs_deinit(struct __fatfs_helpers* pFatfsHelpers) {
    FRESULT fRes = f_mount(NULL, "fatfs", 0U); //unmount fs
    if (fRes != FR_OK) {
        ESP_LOGE(TAG, "Failed to unmount filesystem.");
        return ESP_FAIL;
    }
    ff_diskio_register(0x0U, NULL); // unregister diskio driver
    
    esp_err_t err = esp_vfs_fat_unregister_path("/spiflash");
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failure while calling esp_vfs_fat_unregister_path (ESP_ERR_INVALID_STATE). Seems like FATFS is not registered in VFS.");
            return ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "Failure while calling esp_vfs_fat_unregister_path (Other error).");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "FATFS unregistered from VFS");
    return ESP_OK;
}*/

/**
 * @returns 0 if drive is partitioned, 1 if it is not partitioned, 2 on error
*/
/*int __registration_volume_guard(struct __fatfs_helpers* pFatfsHelpers) {
    DWORD nfreeClust = 0;
    FRESULT fResult = f_getfree(REGISTRATION_FILE_PATH, &nfreeClust, &pFatfsHelpers->pVfsFat);
    if (fResult != FR_OK) {
        return 2;
    }
    if (pFatfsHelpers->pVfsFat->n_fatent - 2 == nfreeClust) {
        return 1;
    } else {
        return 0;
    }
}*/

/*esp_err_t __registration_volume_prepare() {
    LBA_t partitionTable[] = {0x400};//?
    FRESULT fResult = f_fdisk(0x0U, partitionTable, NULL);
    if (fResult != FR_OK) {
        ESP_LOGE(TAG, "f_fdisk failed.");
        return ESP_FAIL;
    }
    void* workBuf = malloc (FF_MAX_SS);
    fResult = f_mkfs("/spiflash", NULL, workBuf, FF_MAX_SS);
    free (workBuf);
    if (fResult != FR_OK) {
        ESP_LOGE(TAG, "f_mkfs failed.");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully partitioned the drive and formatted the filesystem.");
    return ESP_OK;
}*/

/**
 * @side_effect Partitions drive and formats filesystem if neccessary
*/
/*registration_status_t __registration_status_get(struct __fatfs_helpers* pFatfsHelpers) {
    int volGuard_retVal = __registration_volume_guard(pFatfsHelpers);
    if (volGuard_retVal == 1) {
        ESP_LOGI(TAG, "Drive not partitioned. Partitioning and initializing filesystem...");
        esp_err_t volPrepare_err = __registration_volume_prepare();
        if (volPrepare_err != ESP_OK) {
            ESP_LOGE(TAG, "__registration_volume_prepare errored.");
            exit(EXIT_FAILURE);
        }
        return DEVICE_UNREGISTERED;
    } else if (volGuard_retVal == 2) {
        ESP_LOGE(TAG, "__registration_volume_guard errored.");
        return DEVICE_REGISTRATION_STATUS_UNKNOWN;
    } else if (volGuard_retVal != 0) {
        ESP_LOGE(TAG, "__registration_volume_guard returned with unknown status code.");
        return DEVICE_REGISTRATION_STATUS_UNKNOWN;
    }

    FILINFO finfo = {};
    FRESULT fResult = f_stat(REGISTRATION_FILE_PATH, &finfo);
    if (fResult == FR_OK) {
        ESP_LOGI(TAG, "%s file exists", REGISTRATION_FILE_PATH);
        return DEVICE_REGISTERED;
    } else if (fResult == FR_NO_FILE) {
        ESP_LOGI(TAG, "%s file doesn't exist", REGISTRATION_FILE_PATH);
        return DEVICE_UNREGISTERED;
    } else {
        ESP_LOGE(TAG, "f_stat failed for %s", REGISTRATION_FILE_PATH);
        return DEVICE_REGISTRATION_STATUS_UNKNOWN;
    }
}*/

/*registration_status_t registration_check_device_registered() {
    struct __fatfs_helpers fatfsHelpers = {};
    esp_err_t err = __registration_fatfs_init(&fatfsHelpers);
    if (err == ESP_OK) {
        // [TODO] Do something with the filesystem, optionally use additional configuration
        registration_status_t registrationStatus = __registration_status_get(&fatfsHelpers);
        
        err = __registration_fatfs_deinit(&fatfsHelpers);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "FatFS deinitialization failed");
        }
        return registrationStatus;
    } else {
        ESP_LOGE(TAG, "FatFS initialization failed");
        return DEVICE_REGISTRATION_STATUS_UNKNOWN;
    }
}*/

struct ___fatfs_helpers {
    esp_vfs_fat_mount_config_t mountConfig;
    wl_handle_t wlHandle;
};

esp_err_t ___registration_fatfs_init(struct ___fatfs_helpers* out_pFatFsHelpers) {
    *out_pFatFsHelpers = (struct ___fatfs_helpers){
        .mountConfig = {
            .allocation_unit_size = 0, // use sector-size alloc unit
            .disk_status_check_enable = false,
            .format_if_mount_failed = true,
            .max_files = 1
        },
        .wlHandle = 0
    };
    
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl("/spiflash", "fatfs", &out_pFatFsHelpers->mountConfig, &out_pFatFsHelpers->wlHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Couldn't mount the filesystem or formatting failed. (esp_vfs_fat_spiflash_mount_rw_wl failure).");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ___registration_fatfs_deinit(struct ___fatfs_helpers* pFatfsHelpers) {
    esp_err_t err = esp_vfs_fat_spiflash_unmount_rw_wl("/spiflash", pFatfsHelpers->wlHandle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_vfs_fat_spiflash_unmount_rw_wl failed. Probably esp_vfs_fat_spiflash_mount_rw_wl hasn't been called!");
            
        } else {
            ESP_LOGE(TAG, "esp_vfs_fat_spiflash_unmount_rw_wl failed (Other error)");
        }
        return ESP_FAIL; 
    }
    return ESP_OK;
}

#define yields ,
#define with ,
#define both(x, y) ((x) && (y))

UINT ___registration_fio_read_str(FIL* pFil, char** pStr, UINT maxLength) {
    char c = '?';
    UINT nc = 0U;
    UINT n = 0U;
    while(f_read(pFil, &c, 1U, &nc)
          yields both(nc == 1U, c != '\n' && n < maxLength)) {
        (*pStr)[n++] = c;
    }
    (*pStr)[n] = '\0';
    return n;
}

esp_err_t ___registration_fio_read_u32(FIL* pFil, uint32_t* out_pVal) {
    UINT nb = 0U;
    f_read(pFil, (void*)out_pVal, 4U, &nb);
    if (nb != 4U) {
        ESP_LOGE(TAG, "Failed to read 4 bytes of u32 from file.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*void logi(char* msg, ...) {
    ESP_LOGI(TAG, msg, __VALIST);
}*/

void ___registration_file_closed_message(FRESULT fr, char* filePath) {
    if (fr == FR_OK) {
        ESP_LOGI(TAG, "Closed %s", filePath);
    } else {
        ESP_LOGE(TAG, "Failed to close %s", filePath);
    }
}

#define registration_fio_FALLBACK(___fr, ___pFil) return (___fr=f_close(___pFil), ___registration_file_closed_message(___fr, REGISTRATION_FILE_PATH), ESP_FAIL)
#define registration_fio_SUCCEED_DISPOSE(___fr, ___pFil) return (___fr=f_close(___pFil), ___registration_file_closed_message(___fr, REGISTRATION_FILE_PATH), ESP_OK);

esp_err_t ___registration_fio_fetch_data(registration_data_t* out_pRegistrationData) {
    FIL fil = {};
    FRESULT fr = FR_OK;
    fr = f_open(&fil, REGISTRATION_FILE_PATH, FA_READ);
    if (fr != FR_OK) {
        ESP_LOGE(TAG, "Failed to open file %s in r mode.", REGISTRATION_FILE_PATH);
        return ESP_FAIL;
    }
    UINT ssidLength = ___registration_fio_read_str(&fil, (char**)&out_pRegistrationData->pCharacteristics->wifi_ssid, MAX_WIFI_SSID_LENGTH);
    if (ssidLength < MIN_WIFI_SSID_LENGTH || ssidLength > MAX_WIFI_SSID_LENGTH) {
        ESP_LOGE(TAG, "Detected invalid SSID length in %s !", REGISTRATION_FILE_PATH);
        //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_FAIL);
        registration_fio_FALLBACK(fr, &fil);
    }
    UINT pskLength = ___registration_fio_read_str(&fil, (char**)&out_pRegistrationData->pCharacteristics->wifi_psk, MAX_WIFI_PSK_LENGTH);
    if (pskLength < MIN_WIFI_PSK_LENGTH || pskLength > MAX_WIFI_PSK_LENGTH) {
        ESP_LOGE(TAG, "Detected invalid PSK length in %s !", REGISTRATION_FILE_PATH);
        //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_FAIL);
        registration_fio_FALLBACK(fr, &fil);
    }
    UINT uidLength = ___registration_fio_read_str(&fil, (char**)&out_pRegistrationData->pCharacteristics->user_id, MAX_USER_ID_LENGTH);
    if (uidLength < MIN_USER_ID_LENGTH || uidLength > MAX_USER_ID_LENGTH) {
        ESP_LOGE(TAG, "Detected invalid UID length in %s !", REGISTRATION_FILE_PATH);
        //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_FAIL);
        registration_fio_FALLBACK(fr, &fil);
    }
    
    /*esp_err_t err = ___registration_fio_read_u32(&fil, &out_pRegistrationData->cam_id);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read cam_id from %s !", REGISTRATION_FILE_PATH);
        //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_FAIL);
        registration_fio_FALLBACK(fr, &fil);
    }*/

    UINT cidLength = ___registration_fio_read_str(&fil, (char**)&out_pRegistrationData->cam_id, CID_LENGTH);
    if (cidLength != CID_LENGTH) {
        ESP_LOGE(TAG, "Detected invalid cid length in %s !", REGISTRATION_FILE_PATH);
        registration_fio_FALLBACK(fr, &fil);
    }

    UINT ckeyLength = ___registration_fio_read_str(&fil, (char**)&out_pRegistrationData->ckey, CKEY_LENGTH);
    if (ckeyLength != CKEY_LENGTH) {
        ESP_LOGE(TAG, "Detected invalid ckey length in %s !", REGISTRATION_FILE_PATH);
        //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_FAIL);
        registration_fio_FALLBACK(fr, &fil);
    }
    
    //return (fr=f_close(&fil), ___registration_file_closed_message(fr, REGISTRATION_FILE_PATH), ESP_OK);
    registration_fio_SUCCEED_DISPOSE(fr, &fil);
}

esp_err_t ___registration_fio_write_data(registration_data_t* pRegistrationData) {
    FIL fil = {};
    FRESULT fr = FR_OK;
    fr = f_open(&fil, REGISTRATION_FILE_PATH, FA_CREATE_NEW | FA_WRITE);
    if (fr != FR_OK) {
        ESP_LOGE(TAG, "Failed to open file %s in wx mode.", REGISTRATION_FILE_PATH);
        return ESP_FAIL;
    }
    // [TODO] write structured data to file, close it, check for errors

    return 0;
}

static void __semph_operation_assert_success(int result, char* pcSemphOpName) {
    if (result != pdTRUE) {
        ESP_LOGE(TAG, "%s failed.", pcSemphOpName);
        assert(0);
    }
}

/**
 * @note registrationNetworkConnectivityCheckCallback will be called each time the client writes to the wifi_psk characteristic
*/
esp_err_t registration_main(registration_data_t* out_pRegistrationData, 
                            registrationCallback_Function registrationNetworkConnectivityCheckCallback, 
                            registrationCallback_Function registrationServerCommmunicationCallback) {
                                
    registration_init(out_pRegistrationData);

    struct ___fatfs_helpers fatfsHelpers = {};
    esp_err_t err = ___registration_fatfs_init(&fatfsHelpers);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "fatfs initialization failed");
        return DEVICE_REGISTRATION_STATUS_UNKNOWN;
    } else { ESP_LOGI(TAG, "fatfs initialization succeeded");  }


    registration_status_t registrationStatus = registration_check_device_registered(out_pRegistrationData);
    
    switch (registrationStatus) {
        case DEVICE_REGISTERED:
            ESP_LOGI(TAG, "Device is registered.");
            break;
        case DEVICE_UNREGISTERED:
            ESP_LOGI(TAG, "Device is NOT registered.");
            break;
        case DEVICE_REGISTRATION_STATUS_UNKNOWN:
            ESP_LOGE(TAG, "Device registration check failed.");
            return ESP_FAIL;
        default:
            ESP_LOGE(TAG, "Unexpected device registration check result.");
            return ESP_FAIL;
    }

    /*[DEBUG]*/sau_heap_debug_info();
    ///*[DEBUG]*/memset(out_pRegistrationData->pCharacteristics->wifi_ssid, 0, sizeof(out_pRegistrationData->pCharacteristics->wifi_ssid)); memset(out_pRegistrationData->pCharacteristics->wifi_psk, 0, sizeof(out_pRegistrationData->pCharacteristics->wifi_psk)); memcpy(out_pRegistrationData->pCharacteristics->wifi_ssid, "ama", strlen("ama")); memcpy(out_pRegistrationData->pCharacteristics->wifi_psk, "2a0m0o3n", strlen("2a0m0o3n")); registrationStatus = DEVICE_REGISTERED; registrationNetworkConnectivityCheckCallback(out_pRegistrationData);
    if (registrationStatus == DEVICE_UNREGISTERED) {
        //Start BLE
        int rv = ble_interface_start();
        if (rv != 0) {
            ESP_LOGE(TAG, "ble_interface_start returned with failure code [%d]", rv);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "ble_interface_start returned successfully");
        /*[DEBUG]*/sau_heap_debug_info();

        //Wait for registration data and check WiFi network credentials
        s_semph_get_registration = xSemaphoreCreateBinary();
        if (s_semph_get_registration == NULL) {
            ESP_LOGE(TAG, "No memory for registration semaphore");
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGI(TAG, "Registration semaphore created successfully");
        /*[DEBUG]*/sau_heap_debug_info();

        registration_network_state_t networkState = NETWORK_STATE_WIFI_DISCONNECTED;
        do {
            ESP_LOGI(TAG, "Waiting for mobile app registration...");
            __semph_operation_assert_success(xSemaphoreTake(s_semph_get_registration, portMAX_DELAY), "xSemaphoreTake");
            ESP_LOGI(TAG, "xSemaphoreTake returned (registration semaphore)");
            /*[DEBUG]*/sau_heap_debug_info();

            ///*[DEBUG]*/ memset(out_pRegistrationData->pCharacteristics->wifi_ssid, 0, sizeof(out_pRegistrationData->pCharacteristics->wifi_ssid)); memset(out_pRegistrationData->pCharacteristics->wifi_psk, 0, sizeof(out_pRegistrationData->pCharacteristics->wifi_psk)); memcpy(out_pRegistrationData->pCharacteristics->wifi_ssid, "ama", strlen("ama")); memcpy(out_pRegistrationData->pCharacteristics->wifi_psk, "2a0m0o3n", strlen("2a0m0o3n")); registrationNetworkConnectivityCheckCallback(out_pRegistrationData); networkState = NETWORK_STATE_WIFI_CONNECTED;
            networkState = registrationNetworkConnectivityCheckCallback(out_pRegistrationData, NULL);
            ///*[debug]*/ networkState = wifi_connect(out_pRegistrationData->pCharacteristics->wifi_ssid, out_pRegistrationData->pCharacteristics->wifi_psk) == ESP_OK;
            sau_gatt_registration_service_chr_values.network_state = (uint8_t)networkState;
            /*[DEBUG]*/sau_heap_debug_info();
            notify_ble_central();
            /*[DEBUG]*/sau_heap_debug_info();
        } while(networkState == NETWORK_STATE_WIFI_DISCONNECTED);

        vSemaphoreDelete(s_semph_get_registration);
        if (networkState != NETWORK_STATE_WIFI_CONNECTED) {
            ESP_LOGE(TAG, "Unexpected registration network state.");
            return ESP_FAIL;
        }
        // Stop BLE
        ESP_LOGI(TAG, "Before ble shutdown");
        /*[DEBUG]*/sau_heap_debug_info();
        rv = ble_interface_stop();
        if (rv != 0) {
            ESP_LOGE(TAG, "ble_interface_stop returned with failure code [%d]", rv);
            return ESP_FAIL;
        }
        
        ESP_LOGI(TAG, "After ble shutdown");
        /*[DEBUG]*/sau_heap_debug_info();


        //TCP connection to server
        SemaphoreHandle_t s_semph_serv_comm_callback_sync = xSemaphoreCreateBinary();
        registrationServerCommmunicationCallback(out_pRegistrationData, s_semph_serv_comm_callback_sync);
        
        //Generate ckey
        esp_fill_random(out_pRegistrationData->ckey, CKEY_LENGTH); // Hardware RNG

        ESP_LOGI(TAG, "Waiting for first stage registration communication completion...");
        __semph_operation_assert_success(xSemaphoreTake(s_semph_serv_comm_callback_sync, portMAX_DELAY), "xSemaphoreTake"); // Wait for server communication callback first stage completion
        //Write registration data to file
        ESP_LOGI(TAG, "Writing registration data to file...");
        ESP_LOGE(TAG, "Not implemented."); assert(0); // [TODO]

        __semph_operation_assert_success(xSemaphoreGive(s_semph_serv_comm_callback_sync), "xSemaphoreGive"); // Signal server communication callback to proceed
        __semph_operation_assert_success(xSemaphoreTake(s_semph_serv_comm_callback_sync, portMAX_DELAY), "xSemaphoreTake"); // Wait for server communication callback to finish
        vSemaphoreDelete(s_semph_serv_comm_callback_sync);

    } else {
        //fetch registration data from file [TODO]
        // call registrationNetworkConnectivityCheckCallback (connect WiFi) [TODO] // Check connectivity in loop until connection is obtained? Or fallback to registration (probably not)
    }
    /*[debug]*/sau_heap_debug_info();

    err = ___registration_fatfs_deinit(&fatfsHelpers);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "fatfs deinitialization failed");
        exit(EXIT_FAILURE);
    } else { ESP_LOGI(TAG, "fatfs deinitialization succeeeded");  }

    // (CAMAU will be started by the caller)
    ESP_LOGI(TAG, "End of registration_main");

    return ESP_OK;
}

/**
 * @param out_pRegistrationData If the device has been already registered, the structure pointed by this argument is filled with registration data.
 * @note Requires FatFS to be initialized for partition `fatfs` in spiflash 
*/
registration_status_t registration_check_device_registered(registration_data_t* out_pRegistrationData) {

    registration_status_t registrationStatus = DEVICE_REGISTRATION_STATUS_UNKNOWN;

    FILINFO finfo = {};
    FRESULT fResult = f_stat(REGISTRATION_FILE_PATH, &finfo);
    if (fResult == FR_OK) {
        ESP_LOGI(TAG, "%s file exists", REGISTRATION_FILE_PATH);
        registrationStatus = DEVICE_REGISTERED;
    } else if (fResult == FR_NO_FILE) {
        ESP_LOGI(TAG, "%s file doesn't exist", REGISTRATION_FILE_PATH);
        registrationStatus = DEVICE_UNREGISTERED;
    } else if (fResult == FR_NO_PATH) {
        ESP_LOGI(TAG, "%s path not found", REGISTRATION_FILE_PATH);
        registrationStatus = DEVICE_UNREGISTERED;
    } else {
        ESP_LOGE(TAG, "f_stat failed for %s with unexpected error code [%u]", REGISTRATION_FILE_PATH, fResult);
        registrationStatus = DEVICE_REGISTRATION_STATUS_UNKNOWN;
    }

    if (registrationStatus == DEVICE_REGISTERED) {
        // Fill structure pointed by `out_pRegistrationData`
        esp_err_t err = ___registration_fio_fetch_data(out_pRegistrationData);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Successfully fetched registration data from file.");
        } else {
            ESP_LOGE(TAG, "Failed to fetch registration data from file.");
        }
    }

    return registrationStatus;
}

void registration_init(registration_data_t* out_pRegistrationData) {
    out_pRegistrationData->pCharacteristics = &sau_gatt_registration_service_chr_values;
}

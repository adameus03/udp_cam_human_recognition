#ifndef __REGISTRATION_H
#define __REGISTRATION_H

#include "host/ble_uuid.h"

#define DEVICE_NAME "SAU Camera Device"
#define DEVICE_NAME_LENGTH 17U

#define BLE_HS_F_USE_PRIVATE_ADDR 0

//#define SAU_GATT_REGISTRATION_SERVICE_UUID 0x10000000000000000000000000000000LLU
//#define SAU_GATT_REGISTRATION_SERVICE_WIFI_SSID_CHARACTERISTIC_UUID 0x10000000000000000000000000000001LLU
//#define SAU_GATT_REGISTRATION_SERVICE_WIFI_PSK_CHARACTERISTIC_UUID 0x10000000000000000000000000000002LLU
//#define SAU_GATT_REGISTRATION_SERVICE_USER_ID_CHARACTERISTIC_UUID 0x10000000000000000000000000000003LLU

/*static const int8_t WIFI_CREDENTIALS_SERVICE_UUID[16] = {
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};*/

/*//821005a5-1112-4dc1-8a09-a6fa67cb659c
#define SAU_GATT_REGISTRATION_SERVICE_UUID BLE_UUID128_DECLARE(\
    0x82, 0x10, 0x05, 0xa5, 0x11, 0x12, 0x4d, 0xc1, 0x8a, 0x09, 0xa6, 0xfa, 0x67, 0xcb, 0x65, 0x9c\
)
//d8c991aa-a2be-4823-851a-f3bcf0ddcdc4
#define SAU_GATT_REGISTRATION_SERVICE_WIFI_SSID_CHARACTERISTIC_UUID BLE_UUID128_DECLARE(\
    0xd8, 0xc9, 0x91, 0xaa, 0xa2, 0xbe, 0x48, 0x23, 0x85, 0x1a, 0xf3, 0xbc, 0xf0, 0xdd, 0xcd, 0xc4\
)
//d8132bb8-f1e9-46ac-839d-807046eb444e
#define SAU_GATT_REGISTRATION_SERVICE_WIFI_PSK_CHARACTERISTIC_UUID BLE_UUID128_DECLARE(\
    0xd8, 0x13, 0x2b, 0xb8, 0xf1, 0xe9, 0x4a, 0xac, 0x83, 0x9d, 0x80, 0x70, 0x46, 0xeb, 0x44, 0x4e\
)
//fc20cddd-149c-47a4-a555-4fbe3504b4b8
#define SAU_GATT_REGISTRATION_SERVICE_USER_ID_CHARACTERISTIC_UUID BLE_UUID128_DECLARE(\
    0xfc, 0x20, 0xcd, 0xdd, 0x14, 0x9c, 0x47, 0xa4, 0xa5, 0x55, 0x4f, 0xbe, 0x35, 0x04, 0xb4, 0xb8\
)
//c72a4c9c-d93e-4141-a968-d8863f6b0add
#define SAU_GATT_REGISTRATION_SERVICE_NETWORK_STATE_CHARACTERISTIC_UUID BLE_UUID128_DECLARE(\
    0xc7, 0x2a, 0x4c, 0x9c, 0xd9, 0x3e, 0x41, 0x41, 0xa9, 0x68, 0xd8, 0x86, 0x3f, 0x6b, 0x0a, 0xdd\
)*/


/*static const ble_uuid_t* SAU_GATT_REGISTRATION_SERVICE_UUID = BLE_UUID128_DECLARE(
    0x82, 0x10, 0x05, 0xa5, 0x11, 0x12, 0x4d, 0xc1, 0x8a, 0x09, 0xa6, 0xfa, 0x67, 0xcb, 0x65, 0x9c
);
static const ble_uuid_t* SAU_GATT_REGISTRATION_SERVICE_WIFI_SSID_CHARACTERISTIC_UUID = BLE_UUID128_DECLARE(\
    0xd8, 0xc9, 0x91, 0xaa, 0xa2, 0xbe, 0x48, 0x23, 0x85, 0x1a, 0xf3, 0xbc, 0xf0, 0xdd, 0xcd, 0xc4\
);
static const ble_uuid_t* SAU_GATT_REGISTRATION_SERVICE_WIFI_PSK_CHARACTERISTIC_UUID = BLE_UUID128_DECLARE(\
    0xd8, 0x13, 0x2b, 0xb8, 0xf1, 0xe9, 0x4a, 0xac, 0x83, 0x9d, 0x80, 0x70, 0x46, 0xeb, 0x44, 0x4e\
);
static const ble_uuid_t* SAU_GATT_REGISTRATION_SERVICE_USER_ID_CHARACTERISTIC_UUID = BLE_UUID128_DECLARE(\
    0xfc, 0x20, 0xcd, 0xdd, 0x14, 0x9c, 0x47, 0xa4, 0xa5, 0x55, 0x4f, 0xbe, 0x35, 0x04, 0xb4, 0xb8\
);*/

#define MIN_WIFI_SSID_LENGTH 1
#define MAX_WIFI_SSID_LENGTH 20
#define MIN_WIFI_PSK_LENGTH 1
#define MAX_WIFI_PSK_LENGTH 40
#define MIN_USER_ID_LENGTH 16
#define MAX_USER_ID_LENGTH 16

#define CID_LENGTH 16
#define CKEY_LENGTH 16

typedef enum {
    DEVICE_UNREGISTERED = 0U,
    DEVICE_REGISTERED = 1U,
    DEVICE_REGISTRATION_STATUS_UNKNOWN = 2U
} registration_status_t;

typedef enum {
    NETWORK_STATE_WIFI_DISCONNECTED = 0U,
    NETWORK_STATE_WIFI_CONNECTED = 1U
} registration_network_state_t;

struct __sau_gatt_chr_vals {
    char wifi_ssid[MAX_WIFI_SSID_LENGTH+1];
    char wifi_psk[MAX_WIFI_PSK_LENGTH+1];
    char user_id[MAX_USER_ID_LENGTH+1]; 
    uint8_t network_state;
};

typedef struct {
    struct __sau_gatt_chr_vals* pCharacteristics; 
    uint32_t cam_id;
    char ckey[CKEY_LENGTH+1];
} registration_data_t;

typedef uint32_t (*registrationCallback_Function)(registration_data_t* pRegistrationData);

int ble_interface_start();

void registration_init(registration_data_t* out_pRegistrationData);
registration_status_t registration_check_device_registered(registration_data_t* out_pRegistrationData);
registration_network_state_t registration_check_wifi_credentials_and_connect(const char* ssid, const char* psk);

esp_err_t registration_main(registration_data_t* out_pRegistrationData, 
                            registrationCallback_Function registrationNetworkConnectivityCheckCallback, 
                            registrationCallback_Function registrationServerCommmunicationCallback);

#endif
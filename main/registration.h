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

typedef enum {
    DEVICE_UNREGISTERED,
    DEVICE_REGISTERED
} registration_status_t;

int test_start();

#endif
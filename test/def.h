#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <assert.h>

#define SERV_IP_ADDR "0.0.0.0"
#define TCP_PORT 3334

extern int client_sock_handler(int client_sock);
static inline int client_sock_setup(int client_sock) {
    struct timeval timeout;
    timeout.tv_sec = 0;    //
    timeout.tv_usec = 0;   // No timeout, but in real application, it could be controlled in a different way (limit wait time between request and response)

    if (setsockopt (client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                sizeof timeout) < 0) {
        fprintf(stderr, "setsockopt failed\n");
        return 1;
    }

    if (setsockopt (client_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                sizeof timeout) < 0) {
        fprintf(stderr, "setsockopt failed\n");
        return 1;
    }
}

/*
    TCP COMMUNICATIONS - data structures for application control
*/

#define COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH 6
#define COMM_CSID_LENGTH 16

#define MIN_WIFI_SSID_LENGTH 1
#define MAX_WIFI_SSID_LENGTH 20
#define MIN_WIFI_PSK_LENGTH 1
#define MAX_WIFI_PSK_LENGTH 40
#define MIN_USER_ID_LENGTH 16
#define MAX_USER_ID_LENGTH 16

#define CID_LENGTH 16
#define CKEY_LENGTH 16


typedef union { // op = APP_CONTROL_OP_REGISTER
    struct {
        char user_id[MAX_USER_ID_LENGTH]; // UID obtained from the client app, no null terminator
        char cid[CID_LENGTH]; // Cam ID - generated by server, no null terminator
        char ckey[CKEY_LENGTH]; // Cam auth key - generated by cam, no null terminator
        uint8_t dev_mac[COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH]; // WiFi sta if mac addr
    };
    uint8_t raw[MAX_USER_ID_LENGTH + COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH + CID_LENGTH + CKEY_LENGTH];
} __attribute__((packed)) application_registration_section_t; // size = 54

typedef union { // op = APP_CONTROL_OP_INITCOMM
    struct {
        char cid[CID_LENGTH]; // Cam ID, no null terminator
        char ckey[CKEY_LENGTH]; // Cam auth key, no null terminator
    };
    uint8_t raw[CID_LENGTH + CKEY_LENGTH];
} __attribute__((packed)) application_initcomm_section_t; // size = 32

#define APP_CONTROL_CAM_CONFIG_RES_OK 0U
#define APP_CONTROL_CAM_CONFIG_RES_JPEG_QUALITY_UNSUPPORED 1U
#define APP_CONTROL_CAM_CONFIG_RES_FRAMESIZE_UNSUPPORTED 2U

typedef union { // op = APP_CONTROL_OP_CAM_SET_CONFIG or op = APP_CONTROL_OP_CAM_GET_CONFIG
    struct {
        int32_t jpeg_quality;
        uint8_t vflip_enable;
        uint8_t hmirror_enable;
        uint8_t framesize; // see espressif__esp32-camera/driver/include/sensor.h:framesize_t
        uint8_t res; // operation result code
    };
    uint8_t raw[8];
} __attribute__((packed)) application_camera_config_section_t; // size = 8

#define MAX_NUM_SUPPORTED_FRAMESIZES 20

typedef union { // op = APP_CONTROL_OP_CAM_GET_CAPS
    struct {
        int32_t min_jpeg_quality;
        int32_t max_jpeg_quality;
        uint8_t num_supported_framesizes;
        uint8_t supported_framesizes[MAX_NUM_SUPPORTED_FRAMESIZES];
    };
    uint8_t raw[9 + MAX_NUM_SUPPORTED_FRAMESIZES];
} __attribute__((packed)) application_camera_caps_section_t; //size = 29

typedef union { // op = APP_CONTROL_OP_ENERGY_SAVING_SCHED_SLEEP or op = APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_NETIF_SCHED
    struct {
        uint32_t energy_saving_starts_in_seconds;
        uint32_t energy_saving_duration_seconds;
    };
    uint8_t raw[8];
} __attribute__((packed)) application_sched_section_t; //size = 8

#define DEVICE_NAME_MAX_LENGTH 20
#define FIRMWARE_VERSION_MAX_LENGTH 20 

typedef union { // op = APP_CONTROL_OP_GET_DEVICE_INFO
    struct {
        uint32_t num_resets;
        uint32_t seconds_since_last_reset;
        uint32_t seconds_since_last_firmware_update;
        uint32_t seconds_since_prod_flash;
        char device_name[DEVICE_NAME_MAX_LENGTH];
        char firmware_version[FIRMWARE_VERSION_MAX_LENGTH];
        uint8_t supports_avc;
        uint8_t supports_hevc;
        uint8_t supports_dma;
    };
    uint8_t raw[19 + DEVICE_NAME_MAX_LENGTH + FIRMWARE_VERSION_MAX_LENGTH];
} __attribute__((packed)) application_device_info_section_t; //size = 59

#define APP_CONTROL_OP_NOP 0x0U
#define APP_CONTROL_OP_REGISTER 0x1U //device registration
#define APP_CONTROL_OP_UNREGISTER 0x2U // server-triggered unregistration
#define APP_CONTROL_OP_INITCOMM 0x3U //for registered device - when tcp comm init (use ckey). Occurs when device initiates connection to server
#define APP_CONTROL_OP_CAM_SET_CONFIG 0x4U
#define APP_CONTROL_OP_CAM_GET_CONFIG 0x5U
#define APP_CONTROL_OP_CAM_GET_CAPS 0x6U
#define APP_CONTROL_OP_CAM_START_UNCONDITIONAL_STREAM 0x7U
#define APP_CONTROL_OP_CAM_STOP_UNCONDITIONAL_STREAM 0x8U
#define APP_CONTROL_OP_ANALYSE 0x9U //Make the server AI analyse images and decide when to stop video transmission (UDP) with a terminating segment (op = APP_CONTROL_OP_CAM_STOP_ANALYSER_STREAM)
#define APP_CONTROL_OP_CAM_STOP_ANALYSER_STREAM 0xAU // Terminate camera analyser-triggered UDP video stream as the server AI decided, that the report can be ignored
#define APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_ANALYSER 0xBU
#define APP_CONTROL_OP_ENERY_SAVING_WAKEUP_ANALYSER 0xCU
#define APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_NETIF 0xDU // Shuts the connection until the analyser reactivates it or the device gets reset for some reason
#define APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_NETIF_SCHED 0xEU // Shutdown the connection for some time (analyser will reactivate it if needed)
#define APP_CONTROL_OP_ENERGY_SAVING_SCHED_SLEEP 0xFU // Put device to sleep for some time
#define APP_CONTROL_OP_OTA 0x10U // OTA Firmware update
#define APP_CONTROL_OP_SOFTWARE_DEVICE_RESET 0x11U // Trigger device software reset
#define APP_CONTROL_OP_GET_DEVICE_INFO 0x12U
#define APP_CONTROL_OP_UNKNOWN 0x7FU

/*
    The MSB bit of op specified in segment header indicates whether it's request or response
        0 - request
        1 - response
*/

#define OP_DIR_REQUEST(op) ((op) & ~(1U << 7)) 
#define OP_DIR_RESPONSE(op) ((op) | (1U << 7))

typedef struct {
    char csid[COMM_CSID_LENGTH]; // cam session ID (zeroed-out means unspecified), no null terminator
    uint32_t data_length;
    uint8_t op; //operation type
} __attribute__((packed)) application_control_segment_info_t; //size = 21


#define COMM_MAX_APP_SEG_DATA_LENGTH 59
#define COMM_MAX_APP_SEG_SIZE sizeof(application_control_segment_info_t) + COMM_MAX_APP_SEG_DATA_LENGTH

typedef union {
    struct {
        union {
            application_control_segment_info_t info;
            uint8_t raw[sizeof(application_control_segment_info_t)];
        } header;
        uint8_t data[COMM_MAX_APP_SEG_DATA_LENGTH];
    };
    uint8_t raw[COMM_MAX_APP_SEG_SIZE];
} __attribute__((packed)) application_control_segment_t; // size = 80

/* Helpers */

static inline void print_hex(const void* u8data, size_t len) {
    const uint8_t* data = (const uint8_t*)u8data;
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
}
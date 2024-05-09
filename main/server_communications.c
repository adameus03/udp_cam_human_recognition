/*
 * markers: warning, tidy
*/

#include "server_communications.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <string.h> /** @tidy Not already included? */

#include "analyser.h"
#include "misc.h"

//#pragma pack(1) // Force compiler to pack struct members together

static const char * TAG = "server_communications";
#define HOST_IP_ADDR /*"192.168.1.15"*//*"192.168.173.32"*//*"192.168.116.32"*//*"192.168.43.32"*//**//*"192.168.34.32"*//*"192.168.123.32"*/"192.168.1.15"
#define UDP_PORT 3333
#define TCP_PORT 3334

static char __comm_csid[COMM_CSID_LENGTH];
static uint8_t __comm_session_initiated = 0U;

static app_comm_credentials_t __comm_credentials = {};

char host_ip[] = HOST_IP_ADDR;
int addr_family = AF_INET;
int ip_protocol = IPPROTO_IP;
int udp_sock = -1;
int tcp_sock = -1;
struct sockaddr_in dest_addr;

esp_err_t begin_udp_stream() {
#if SERVER_COMMUNICATIONS_USE_WOLFSSL == 0
    ESP_LOGI(TAG, "Initialising UDP stream");
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    udp_sock = socket(addr_family, SOCK_DGRAM/*experiment*/, ip_protocol);
    if (udp_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    // Set 100ms timeout for sending data
    struct timeval tx_timeout;
    tx_timeout.tv_sec = 0;
    tx_timeout.tv_usec = 100000;

    struct timeval rx_timeout;
    rx_timeout.tv_sec = 0;
    rx_timeout.tv_usec = 100000;

    int rv = setsockopt (udp_sock, SOL_SOCKET, SO_SNDTIMEO, &tx_timeout, sizeof tx_timeout);
    if (rv < 0) {
        ESP_LOGE(TAG, "Failed to set tx timeout for udp (does it even matter...?)");
    }
    rv = setsockopt (udp_sock, SOL_SOCKET, SO_RCVTIMEO, &rx_timeout, sizeof rx_timeout);
    if (rv < 0) {
        ESP_LOGE(TAG, "Failed to set rx timeout for udp (Who will send us udp segments though...?)");
    }

    // set socket to blocking mode (clear O_NONBLOCK)
    int opts = fcntl(udp_sock, F_GETFL);
    opts = opts & (~O_NONBLOCK);
    fcntl(udp_sock, F_SETFL, opts);

    ESP_LOGI(TAG, "Socket created, for sending to %s:%d", HOST_IP_ADDR, UDP_PORT);
    return ESP_OK;
#else
    ESP_LOGE(TAG, "Not implemented");
    return ESP_FAIL;
#endif
}

void end_udp_stream() {
#if SERVER_COMMUNICATIONS_USE_WOLFSSL == 0
    ESP_LOGI(TAG, "Ending UDP stream");
    if (udp_sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket...");
        shutdown(udp_sock, 0);
        close(udp_sock);
    }
    ESP_LOGI(TAG, "Socket closed");
    x_on_udp_transmission_end();
#else 
    ESP_LOGE(TAG, "Not implemented");
    return ESP_FAIL;
#endif
}

/*typedef union {
    struct {
        uint32_t pkt_idx; // Chunk index (assigns chunk index to IP packet)
        jfif_chunk_type_t chunk_type; // JFIF_INTERMEDIATE_CHUNK, JFIF_FIRST_CHUNK, JFIF_LAST_CHUNK or JFIF_ONLY_CHUNK
        uint8_t data[jfif_chunk_segment_max_data_size]; // JFIF chunk data buffer
    } structure;
    uint8_t raw[jfif_chunk_segment_total_size];
} jfif_chunk_segment_t;*/

typedef struct {
    union {
        struct {
            uint32_t pkt_idx; // Chunk index (assigns chunk index to IP packet)
            jfif_chunk_type_t chunk_type; // JFIF_INTERMEDIATE_CHUNK, JFIF_FIRST_CHUNK, JFIF_LAST_CHUNK or JFIF_ONLY_CHUNK
            //char csid[COMM_CSID_LENGTH];
        };
        uint8_t raw[APP_DESC_SEGMENT_SIZE];
    } __attribute__((packed)) desc;
    uint8_t* pData; //JFIF chunk data buffer ptr
} jfif_chunk_segment_t;

/**
 * @brief Transforms a JFIF chunk to an application layer segment
 * @param jfif_buf_ptr JFIF chunk pointer
 * @param len Chunk length
 * @param packet_idx Chunk index (assigns chunk index to IP packet)
 * @param chunk_type JFIF_INTERMEDIATE_CHUNK, JFIF_FIRST_CHUNK, JFIF_LAST_CHUNK or JFIF_ONLY_CHUNK provided using the JFIF_CHUNK_TYPE macro
 * 
 * 
*/
/*void transform_segment(uint8_t** jfif_buf_ptr, size_t len, size_t packet_idx, jfif_chunk_type_t chunk_type) {
    *jfif_buf_ptr = realloc(*jfif_buf_ptr, len + APP_DESC_SEGMENT_SIZE); // Make place for 4 bytes of chunk index and 1 byte for chunk type
    (*jfif_buf_ptr)[len] = len & 0xff; // Storing len in LSB order
    (*jfif_buf_ptr)[len + 1] = (len >> 0x8) & 0xff;
    (*jfif_buf_ptr)[len + 2] = (len >> 0x10) & 0xff;
    (*jfif_buf_ptr)[len + 3] = (len >> 0x18) & 0xff;

    (*jfif_buf_ptr)[len + 4] = (uint8_t)chunk_type;
}*/


esp_err_t transmit_udp(uint8_t* data, size_t len) {
#if SERVER_COMMUNICATIONS_USE_WOLFSSL == 0
    if (len > MAX_UDP_DATA_SIZE) {
        ESP_LOGE(TAG, "Max UDP data size exceeded");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Sending %d bytes...", len);
    assert(data != NULL);
    // [replace] int err = sendto(udp_sock, data, MAX_UDP_DATA_SIZE, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    int err = sendto(udp_sock, data, len, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending JFIF chunk via UDP: errno %d", errno);
        return ESP_FAIL;
    } else {/*lwip_writev*/
        ESP_LOGI(TAG, "No errors occurred during sending JFIF chunk");
        return ESP_OK;
    }
#else 
    ESP_LOGE(TAG, "Not implemented");
    return ESP_FAIL;
#endif
}

esp_err_t transmit_jfif(uint8_t** jfif_buf_ptr, size_t len) {
#if SERVER_COMMUNICATIONS_USE_WOLFSSL == 0
    static uint32_t pkt_idx = 0U;
    uint32_t remaining_bytes = len;
    //[replace with #a] uint8_t* txPtr = *jfif_buf_ptr;
    //static const uint32_t MAX_JFIF_CHUNK_SZ = MAX_UDP_DATA_SIZE - APP_DESC_SEGMENT_SIZE;

    uint32_t chunk_data_len = -1U;
    //transform_segment(jfif_buf_ptr, chunk_len, pkt_idx, JFIF_FIRST_CHUNK);
    
    jfif_chunk_segment_t seg;
    //[replace] seg.structure.pkt_idx = pkt_idx;
    seg.pData = *jfif_buf_ptr; // #a
    seg.desc.pkt_idx = pkt_idx;
    
    if (len <= jfif_chunk_segment_max_data_size) {
        chunk_data_len = len;
        //[replace] seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_ONLY_CHUNK);
        seg.desc.chunk_type = JFIF_CHUNK_TYPE(JFIF_ONLY_CHUNK);
    } else {
        chunk_data_len = jfif_chunk_segment_max_data_size;
        //[replace] seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_FIRST_CHUNK);
        seg.desc.chunk_type = JFIF_CHUNK_TYPE(JFIF_FIRST_CHUNK);
    }

    // Set CSID
    memcpy(seg.desc.raw + 5, __comm_csid, COMM_CSID_LENGTH);

    //[remove] memcpy(seg.structure.data, txPtr, chunk_data_len);
    //[replace] transmit_udp(seg.raw, chunk_data_len + APP_DESC_SEGMENT_SIZE);

    uint8_t* buf = (uint8_t*)malloc(chunk_data_len + APP_DESC_SEGMENT_SIZE);
    memcpy(buf, seg.desc.raw, APP_DESC_SEGMENT_SIZE);
    memcpy(buf + APP_DESC_SEGMENT_SIZE, seg.pData, chunk_data_len);
    transmit_udp(buf, chunk_data_len + APP_DESC_SEGMENT_SIZE);
    free(buf);
    
    //transmit_udp(seg.desc.raw, APP_DESC_SEGMENT_SIZE);
    //transmit_udp(seg.pData, chunk_data_len);

    pkt_idx++;
    remaining_bytes -= chunk_data_len;
    
    while (remaining_bytes > 0) { /** @warning remaining_bytes is unsigned, watch out */
        //[replace] txPtr += chunk_data_len;
        seg.pData += chunk_data_len;
        //[replace] seg.structure.pkt_idx = pkt_idx;
        seg.desc.pkt_idx = pkt_idx;
        if (remaining_bytes > jfif_chunk_segment_max_data_size) {
            //[replace] seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK);
            seg.desc.chunk_type = JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK);
        }
        else {
            //[replace] seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);
            seg.desc.chunk_type = JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);
            chunk_data_len = remaining_bytes;
        }
        /* [replace] seg.structure.chunk_type = remaining_bytes > jfif_chunk_segment_max_data_size ? 
            JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK) : 
            JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);*/
        seg.desc.chunk_type = remaining_bytes > jfif_chunk_segment_max_data_size ? 
            JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK) : 
            JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);
        //return ESP_OK; /** @test */

        // Set CSID
        memcpy(seg.desc.raw + 5, __comm_csid, COMM_CSID_LENGTH);

        //[remove] memcpy(seg.structure.data, txPtr, chunk_data_len);
        //[replace]transmit_udp(seg.raw, chunk_data_len + APP_DESC_SEGMENT_SIZE);

        uint8_t* buf = (uint8_t*)malloc(chunk_data_len + APP_DESC_SEGMENT_SIZE);
        memcpy(buf, seg.desc.raw, APP_DESC_SEGMENT_SIZE);
        memcpy(buf + APP_DESC_SEGMENT_SIZE, seg.pData, chunk_data_len);
        transmit_udp(buf, chunk_data_len + APP_DESC_SEGMENT_SIZE);
        free(buf);

        //transmit_udp(seg.desc.raw, APP_DESC_SEGMENT_SIZE);
        //transmit_udp(seg.pData, chunk_data_len);

        pkt_idx++;
        remaining_bytes -= chunk_data_len;
    }
    return ESP_OK; /** @warning I haven't returned ESP_FAIL anywhere in this function */
#else 
    ESP_LOGE(TAG, "Not implemented");
    return ESP_FAIL;
#endif
}



/*#define MAX_UDP_DATA_SIZE 16384
#define CHUNK_DELAY_MS 100

esp_err_t transmit_udp(uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "Sending decoy signal");
    int err = sendto(udp_sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Decoy signal sent, waiting for server to respond");
    err = recvfrom(udp_sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, NULL);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        return ESP_FAIL;
    }
    size_t blk_size = MAX_UDP_DATA_SIZE;//
    size_t num_blks = len / blk_size;
    size_t rem = len % blk_size;
    ESP_LOGI(TAG, "Server responded, starting jpeg transmission (%d = %d x %d + %d)", len, num_blks, blk_size, rem);
    uint16_t err_count = 0;
    for (size_t i = 0; i < num_blks; i++) {
        int err = sendto(udp_sock, data + i * blk_size, blk_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        vTaskDelay(CHUNK_DELAY_MS / portTICK_PERIOD_MS);
        if (err < 0) {
            err_count++;
        }
    }
    if (rem > 0) {
        int err = sendto(udp_sock, data + num_blks * blk_size, rem, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            err_count++;
        }
    }
    if (err_count > 0) {
        ESP_LOGE(TAG, "%d errors occurred during sending jpeg", err_count);
        ESP_LOGI(TAG, "Transmission complete");
        return ESP_FAIL;
    }
    else {
        ESP_LOGI(TAG, "No errors occurred during sending jpeg");
        return ESP_OK;
    }
}*/

/*esp_err_t transmit_udp(uint8_t* data, size_t len) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "Transmitting UDP data");
    ESP_LOGD(TAG, "Sending data of length %d", len);

    unsigned int err_count = 0;
    // blk_size = 2^15
    size_t blk_size = 32768;//
    size_t num_blks = len / blk_size;
    size_t rem = len % blk_size;

    ESP_LOGD(TAG, "Sending %d blocks of size %d, remainder %d", num_blks, blk_size, rem);
    
    unsigned char server_alive = 0;
    for (size_t i = 0; i < num_blks; i++) {
        int err = sendto(udp_sock, data + i * blk_size, blk_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            // Don't print an error message for each block, it's too much
            //ESP_LOGD(TAG, "Error occurred during sending blk %d: errno %d", i, errno);
            err_count++;
            //break;
        }
        
        
        if (!server_alive) {
            // receive empty packet to confirm that the data was received
            // uint8_t pingbuf[1];
            // err = recvfrom(udp_sock, pingbuf, 1, 0, (struct sockaddr *)&dest_addr, NULL);

            // if (err < 0) { // probably server is dead
            //     ESP_LOGI(TAG, "Server is not alive");
            //         return ESP_FAIL;
            // }

            server_alive = 1;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);

        //else if (err != blk_size) {
        //    ESP_LOGE(TAG, "FATAL error occurred during sending blk %d: sent %d bytes, expected %d", i, err, blk_size);
        //}
    }
    if (err_count == 0 && rem > 0) {
        int err = sendto(udp_sock, data + num_blks * blk_size, rem, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            //ESP_LOGD(TAG, "Error occurred during sending: errno %d", errno);
            err_count++;
        }
        //recvfrom(udp_sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, NULL);
        //else if (err != rem) {
        //    ESP_LOGE(TAG, "FATAL error occurred during sending: sent %d bytes, expected %d", err, rem);
        //}
    }
    
    // /^// Send a message to say the data is complete
    // int err = sendto(udp_sock, "END_01234321", 12, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); // END_01234321 is a unique message to indicate the end of the data
    // if (err < 0) {
    //     ESP_LOGD(TAG, "Error occurred during sending end marker!: errno %d", errno);
    //     err_count++;
    // }^/

    if (err_count > 0) {
        ESP_LOGE(TAG, "%d errors occurred during sending", err_count);
        // Resolve the error and print the error message
        //const char *err_msg = esp_err_to_name(err);
        //ESP_LOGE(TAG, "Error message: %s", err_msg);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Message sent");

    ESP_LOGI(TAG, "Now synchronising with server");
    //
    //  esp32 (frame chunks)---> server
    //  esp32 <--- server (acknowledgement) // server is ready to receive the next frame
    //  esp32 (aknowledgement)---> server // esp32 is ready to send the next frame
        
    //

    return ESP_OK;
}*/

void app_comm_set_credentials(char* cid, char* ckey) {
    memcpy(__comm_credentials.cid, cid, CID_LENGTH);
    memcpy(__comm_credentials.ckey, ckey, CKEY_LENGTH);
}

///////////////

/*
    TCP COMMUNICATIONS - data structures for application control
*/

typedef union { // op = APP_CONTROL_OP_REGISTER
    struct {
        char user_id[MAX_USER_ID_LENGTH]; // UID obtained from the client app, no null terminator
        char cid[CID_LENGTH]; // Cam ID - generated by server, no null terminator
        char ckey[CKEY_LENGTH]; // Cam auth key - generated by cam, no null terminator
        uint8_t dev_mac[COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH]; // WiFi sta if mac addr
    };
    uint8_t raw[MAX_USER_ID_LENGTH + COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH + CID_LENGTH + CKEY_LENGTH];
} __attribute__((packed)) application_registration_section_t; // size = 54

typedef union { // op = APP_CONTROL_OP_UNREGISTER
    struct {
        uint8_t succeeded; // 0x01 if succeeded, 0x00 otherwise
    };
    uint8_t raw[1];
} __attribute__((packed)) application_unregistration_section_t; //size = 1

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
#define APP_CONTROL_OP_ENERGY_SAVING_WAKEUP_ANALYSER 0xCU
#define APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_NETIF 0xDU // Shuts the connection until the analyser reactivates it or the device gets reset for some reason
#define APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_NETIF_SCHED 0xEU // Shutdown the connection for some time (analyser will reactivate it if needed)
#define APP_CONTROL_OP_ENERGY_SAVING_SCHED_SLEEP 0xFU // Put device to sleep for some time
#define APP_CONTROL_OP_OTA 0x10U // OTA Firmware update
#define APP_CONTROL_OP_SOFTWARE_DEVICE_RESET 0x11U // Trigger device software reset
#define APP_CONTROL_OP_GET_DEVICE_INFO 0x12U
#define APP_CONTROL_OP_LOGS_MODE_MINIMAL 0x13U
#define APP_CONTROL_OP_LOGS_MODE_COMPLETE 0x14U

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



/*void __application_control_segment_alloc_data (application_control_segment_t* pSegment) {
    pSegment->pData = (uint8_t*) malloc(pSegment->header.info.data_length * sizeof(uint8_t));
}

void __application_control_segment_free_data (application_control_segment_t* pSegment) {
    free (pSegment->pData);
}*/


/**
 * TCP Server
 * 
 *  Tasks:
 *      1. TCP receiver task (performs application segment demultiplexing by writing to queues #2 and #3)
 *      2. TCP transmitter task (reads from queue #1)
 * 
 *  Queues: 
 *      1. TCP tx queue
 *      2. TCP rx request queue
 *      3. TCP rx response queue
*/

#define COMM_TCP_RX_QUEUE_MAX_LENGTH 10
#define COMM_TCP_TX_QUEUE_MAX_LENGTH 10
#define COMM_TCP_REQ_QUEUE_MAX_LENGTH 10
#define COMM_TCP_RES_QUEUE_MAX_LENGTH 10

//static QueueHandle_t __tcp_rx_queue;
static QueueHandle_t __tcp_tx_queue;
static QueueHandle_t __tcp_req_queue;
static QueueHandle_t __tcp_res_queue;

static SemaphoreHandle_t __tcp_shutdown_semph = NULL;
static SemaphoreHandle_t __tcp_ready_semph = NULL;
static SemaphoreHandle_t __tcp_tx_queue_empty_semph = NULL; // [TODO] [WARNING]

void reset_tcp_conn() {
    vTaskDelay(100 / portTICK_PERIOD_MS); // ?
    ESP_LOGW(TAG, "Calling xSemaphoreGive(__tcp_shutdown_semph)...");
    if(xSemaphoreGive(__tcp_shutdown_semph) != pdTRUE) { // reset connection
        ESP_LOGW(TAG, "xSemaphoreGive(__tcp_shutdown_semph) failed. It seems like the connection was already reset.");
        //exit(EXIT_FAILURE);
    }
}

static void __semph_operation_assert_success(int result, char* pcSemphOpName) {
    if (result != pdTRUE) {
        ESP_LOGE(TAG, "%s failed.", pcSemphOpName);
        assert(0);
    }
}

void __tcp_demux(application_control_segment_t seg) {
    QueueHandle_t targetQueueHandle = NULL;
    if (seg.header.info.op == OP_DIR_REQUEST(seg.header.info.op)) {
        targetQueueHandle = __tcp_req_queue;
    } else {
        targetQueueHandle = __tcp_res_queue;
    }
    int result = xQueueSend(targetQueueHandle, &seg, portMAX_DELAY);
    if (result != pdTRUE){
        if (result == errQUEUE_FULL) {
            ESP_LOGE(TAG, "%s", targetQueueHandle == __tcp_req_queue ? "Trying to send to request queue which is full!" : "Trying to send to response queue which is full!");
        } else {
            if (targetQueueHandle == __tcp_req_queue) {
                ESP_LOGE(TAG, "xQueueSend(__tcp_req_queue, seg.raw, portMAX_DELAY) failed, [ret val: %d]", result);
            } else {
                ESP_LOGE(TAG, "xQueueSend(__tcp_res_queue, seg.raw, portMAX_DELAY) failed, [ret val: %d]", result);
            }
        }
        //reset_tcp_conn();
        ESP_LOGE(TAG, "Calling exit(EXIT_FAILURE) because xQueueSend failed even though it was called with portMAX_DELAY.");
        exit(EXIT_FAILURE);
    } else {
        ESP_LOGI(TAG, "%s", targetQueueHandle == __tcp_req_queue ? "Successfully demultiplexed an application segment into the request queue." : "Successfully demultiplexed an application segment into the response queue.");
    }
}

void __tcp_rx_task(void *pvParameters) {
    bool ready = false;
    while (1) {
        application_control_segment_t seg = {}; // [TODO] verify if xQueueSend copies its raw buffer (it should as docs say so), otherwise move it outside the loop
                                                // also, skip segment init?
        if (!ready) { //[TODO] Should this be done before second recv as well?
            TASKWISE_LOGF(ESP_LOGI, TAG, "tcp_rx_task", "Waiting for the connection to be ready...");
            __semph_operation_assert_success(xSemaphoreTake(__tcp_ready_semph, portMAX_DELAY), "xSemaphoreTake");
            ESP_LOGI(TAG, "Connection is now ready for __tcp_rx_task.");
            ready = true;
        }
        ESP_LOGW(TAG, "Waiting for tcp_sock recv in task__tcp_rx_task...");
        ssize_t rv = recv(tcp_sock, seg.header.raw, sizeof(seg.header), 0);
        if (rv == sizeof(seg.header)) {
            if (seg.header.info.data_length > 0) {
                rv = recv(tcp_sock, seg.data, seg.header.info.data_length, 0);
                if (rv == seg.header.info.data_length) {
                    
                    /*int result = xQueueSend(__tcp_rx_queue, &seg, portMAX_DELAY); // pass application segment to the rx queue
                    
                    if (result != pdTRUE){
                        if (result == errQUEUE_FULL) {
                            ESP_LOGE(TAG, "Trying to send to the __tcp_rx_queue queue which is full!");
                        } else {
                            ESP_LOGE(TAG, "xQueueSend(__tcp_rx_queue, &seg, portMAX_DELAY) failed, [ret val: %d]", result);
                        }
                        exit(EXIT_FAILURE);
                    } else {
                        ESP_LOGI(TAG, "Application control segment received into the RX queue.");
                    }*/
                    __tcp_demux(seg);
                } else {
                    if ((rv == 0) && (errno == 0)) {
                        ESP_LOGE(TAG, "Received empty seg data, when non-empty seg data was expected!");
                    } else if (rv > 0) {
                        if (rv < seg.header.info.data_length) {
                            ESP_LOGE(TAG, "Received incomplete seg data!");
                        } else {
                            ESP_LOGE(TAG, "Unexpectedly received more data bytes than expected seg data length!");
                        }
                    }
                    if (errno != 0) {
                        ESP_LOGE(TAG, "Error while receiving from TCP socket (trying to receive app seg data bytes) [errno = %d]", errno);
                    }
                    reset_tcp_conn();
                    ready = false;
                }
                
            } else {
                // Segment consists of header only, as there is no segment data
                __tcp_demux(seg);
            }
        } else {
            if ((rv == 0) && (errno == 0)) {
                ESP_LOGE(TAG, "Received empty header!");
            } else if (rv > 0) {
                if (rv < sizeof(seg.header)) {
                    ESP_LOGE(TAG, "Received incomplete header!");
                } else {
                    ESP_LOGE(TAG, "Unexpectedly received more header bytes than expected header length!");
                }
            }
            if (errno != 0) {
                ESP_LOGE(TAG, "Error while receiving from TCP socket (trying to receive app seg header bytes) [errno = %d]", errno);
            }
            reset_tcp_conn();
            ready = false;
        }
    }
}

static uint8_t is_awaiting_tx_queue_empty = 0U;

static void await_tx_queue_empty() { // [TODO] [WARNING]
    ESP_LOGE(TAG, "Skipping await for now, as its implementation has a bug.");
    /*is_awaiting_tx_queue_empty = 1U;
    xSemaphoreTake(__tcp_tx_queue_empty_semph, portMAX_DELAY);*/
}

void __tcp_tx_task(void* pvParameters) {
    bool ready = false;
    while (1) {
        application_control_segment_t seg = {}; //skip segment init?
        if (xQueueReceive(__tcp_tx_queue, &seg, portMAX_DELAY)) {
            
            uint32_t tx_seg_len = sizeof(application_control_segment_info_t) + seg.header.info.data_length;
            if (!ready) {
                ESP_LOGI(TAG, "tcp tx task waiting for the connection to be ready...");
                __semph_operation_assert_success(xSemaphoreTake(__tcp_ready_semph, portMAX_DELAY), "xSemaphoreTake");
                ESP_LOGI(TAG, "Connection is now ready for __tcp_tx_task.");
                ready = true;
            }
            ssize_t rv = send(tcp_sock, seg.raw, (size_t)tx_seg_len, 0);
            if (rv == tx_seg_len) {
                ESP_LOGI(TAG, "Application control segment from the TX queue was transmitted successfully.");
            } else {
                if (rv == 0) {
                    ESP_LOGE(TAG, "No segment bytes were transmitted for some reason.");
                } else if (rv > 0) {
                    if (rv < tx_seg_len) {
                        ESP_LOGE(TAG, "Transmitted incomplete app control segment!");
                    } else {
                        ESP_LOGE(TAG, "Unexpectedly transmitted more bytes than the size of the target app control segment!");
                    }
                } else {
                    ESP_LOGE(TAG, "Error while transmitting to TCP socket [errno = %d]", errno);
                }
                reset_tcp_conn();
                ready = false;
            }
        } else {
            ESP_LOGE(TAG, "Failure while receiving from the __tcp_tx_queue queue");
        }

        if (is_awaiting_tx_queue_empty) {
            if (0 == uxQueueMessagesWaiting(__tcp_tx_queue)) {
                xSemaphoreGive(__tcp_tx_queue_empty_semph);
            }
        }
    }
}

/*void __tcp_demux_task(void* pvParametrs) {
    while (1) {
        application_control_segment_t seg = {}; //skip segment init?
        if (xQueueReceive(__tcp_rx_queue, &seg, portMAX_DELAY)) {
            __tcp_demux(seg);
        } else {
            ESP_LOGE(TAG, "Failure while receiving from the __tcp_tx_queue queue");
        }
    }
}*/

//TaskHandle_t tcp_demux_task_handle = NULL;
TaskHandle_t tcp_tx_task_handle = NULL;
TaskHandle_t tcp_rx_task_handle = NULL;

TaskHandle_t tcp_app_initcomm_task_handle = NULL; // handle for a helper task that will be created to perform the APP_CONTROL_OP_INITCOMM operation

static app_tcp_comm_mode_t __comm_mode = APP_TCP_COMM_MODE_REGISTRATION;

void app_tcp_set_mode(app_tcp_comm_mode_t mode) {
    __comm_mode = mode;
}

static void comm_set_session_initiated() {
    __comm_session_initiated = 1U;
}

static void comm_set_session_uninitiated() {
    __comm_session_initiated = 0U;
}

static void comm_set_csid(char* csid) {
    memcpy(__comm_csid, csid, COMM_CSID_LENGTH);
}

static char* comm_get_csid() {
    return __comm_csid;
}

void tcp_connection_manage_task(void* pvParameters) { // [TODO] Keepalive ?
                                                      // [TODO] remember to delete queues/semaphore when not used? (Increase analyser caps then??)
    __tcp_shutdown_semph = xSemaphoreCreateBinary();
    __tcp_ready_semph = xSemaphoreCreateCounting(2, 0);
    __tcp_tx_queue_empty_semph = xSemaphoreCreateBinary();
    if (__tcp_shutdown_semph == 0) {
        ESP_LOGE(TAG, "No memory for __tcp_shutdown_semph");
        //return ESP_ERR_NO_MEM;
        exit(EXIT_FAILURE);
    }
    
    /*__tcp_rx_queue = xQueueCreate(COMM_TCP_RX_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_rx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_rx_queue");
        exit(EXIT_FAILURE);
    }*/

    __tcp_tx_queue = xQueueCreate(COMM_TCP_TX_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_tx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_tx_queue");
        exit(EXIT_FAILURE);
    }

    __tcp_req_queue = xQueueCreate(COMM_TCP_REQ_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_req_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_req_queue");
        exit(EXIT_FAILURE);
    }

    __tcp_res_queue = xQueueCreate(COMM_TCP_RES_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_req_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_res_queue");
        exit(EXIT_FAILURE);
    }

    // Start tcp arch tasks
    /*ESP_LOGI(TAG, "Creating task tcp_demux");
    if (pdPASS != xTaskCreate( __tcp_demux_task, "tcp_demux", 2048, NULL, 1, &tcp_demux_task_handle )) {
        ESP_LOGE(TAG, "Failed to create the tcp_demux task.");
        exit(EXIT_FAILURE);
    }*/

    ESP_LOGI(TAG, "Creating task tcp_tx");
    if (pdPASS != xTaskCreate( __tcp_tx_task, "tcp_tx", 2048, NULL, 1, &tcp_tx_task_handle )) {
        ESP_LOGE(TAG, "Failed to create the tcp_tx task.");
        exit(EXIT_FAILURE);
    }

    ESP_LOGI(TAG, "Creating task tcp_rx"); //
    if (pdPASS != xTaskCreate( __tcp_rx_task, "tcp_rx", 2304, NULL, 1, &tcp_rx_task_handle )) {
        ESP_LOGE(TAG, "Failed to create the tcp_rx task.");
        exit(EXIT_FAILURE);
    }

    ESP_LOGI(TAG, "All core application TCP architecture tasks created. Entering maintanance loop.");

    while (1) { // tcp managing infinite loop
#if SERVER_COMMUNICATIONS_USE_WOLF_SSL == 0
        struct sockaddr_in dest_addr;
        inet_pton(addr_family, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = addr_family;
        dest_addr.sin_port = htons(TCP_PORT);
        
        tcp_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (tcp_sock < 0) {
            ESP_LOGE(TAG, "Unable to create TCP socket: errno %d", errno);
            exit(EXIT_FAILURE);
        }
        ESP_LOGI(TAG, "Created TCP socket.");

        struct timeval timeout; // @attention check: make sure sockopts can share the same timeout memory
        // 0 timeout means wait indefinitely
        timeout.tv_sec = 0;/*5*/;
        timeout.tv_usec = 0; 

        int rv = setsockopt (tcp_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
        if (rv < 0) {
            ESP_LOGE(TAG, "Failed to set tx timeout for tcp.");
            exit(EXIT_FAILURE);
        }
        rv = setsockopt (tcp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        if (rv < 0) {
            ESP_LOGE(TAG, "Failed to set rx timeout for tcp.");
            exit(EXIT_FAILURE);
        }

        uint8_t recreate_socket = 0U;
        while (1) {
            rv = connect(tcp_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (rv == 0) {
                break;
            } else if (errno == ENOTCONN) { 
                ESP_LOGE(TAG, "TCP connect returned ENOTCONN, trying to create socket again...");
                shutdown(tcp_sock, SHUT_RDWR);
                close(tcp_sock);
                recreate_socket = 1U;
                break;
            } else  {
                ESP_LOGE(TAG, "Unable to connect TCP socket: errno %d; Waiting 3 seconds...", errno);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
            }
        }
        if (recreate_socket) {
            continue;
        }

        ESP_LOGI(TAG, "TCP socket connected successfully");
        // signal __tcp_ready_semph, so that the tcp tx and rx tasks can continue 
        __semph_operation_assert_success(xSemaphoreGive(__tcp_ready_semph), "xSemaphoreGive");  // this increases the semaphore count to 1
        __semph_operation_assert_success(xSemaphoreGive(__tcp_ready_semph), "xSemaphoreGive");  // this increases the semaphore count to 2 (both tx and rx tasks can continue)

        if (__comm_mode == APP_TCP_COMM_MODE_REGULAR) {
            /*char* pCsid = __comm_csid;
            tcp_app_init_comm(&pCsid);*/
            ESP_LOGI(TAG, "Creating task tcp_app_initcomm"); //
            if (pdPASS != xTaskCreate( tcp_app_initcomm_task, "tcp_app_initcomm", 2048, NULL, 1, &tcp_app_initcomm_task_handle )) {
                ESP_LOGE(TAG, "Failed to create the tcp_app_initcomm task.");
                exit(EXIT_FAILURE);
            }
        }

        ESP_LOGW(TAG, "Waiting for __tcp_shutdown_semph (semph should be given when there are connection issues)...");
        __semph_operation_assert_success(xSemaphoreTake(__tcp_shutdown_semph, portMAX_DELAY), "xSemaphoreTake"); // Wait until sock shutdown requested
        ESP_LOGI(TAG, "xSemaphoreTake(__tcp_shutdown_semph, portMAX_DELAY) returned.");

        comm_set_session_uninitiated();

        if (tcp_sock != -1) {
            ESP_LOGE(TAG, "Shutting down TCP socket...");
            shutdown(tcp_sock, 0);//SHUT_RDWR
            close(tcp_sock); 
        }

#else
        ESP_LOGE(TAG, "WolfSSL integration Not implemented");
        exit(EXIT_FAILURE);
#endif
    } // end of tcp managing infinite loop
}


#define COMM_UID_LENGTH MAX_USER_ID_LENGTH

void application_control_segment_set_data(application_control_segment_t* pSegment, uint8_t* pData) {
    memcpy(pSegment->data, pData, pSegment->header.info.data_length);
}

void application_registration_section_set_user_id(application_registration_section_t* pSection, char* uid) {
    memcpy(pSection->user_id, uid, COMM_UID_LENGTH);
}

void application_registration_section_set_dev_mac(application_registration_section_t* pSection, uint8_t* puMac) {
    memcpy(pSection->dev_mac, puMac, COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH);
}

void application_registration_section_set_ckey(application_registration_section_t* pSection, char* ckey) {
    /*if (ckey == NULL) { // debug
        ESP_LOGE(TAG, "Trying to set a NULL ckey!");
        exit(EXIT_FAILURE);
    }*/
    assert(ckey != NULL);
    memcpy(pSection->ckey, ckey, CKEY_LENGTH);
    //memcpy(pSection->ckey, "1351351351351357", CKEY_LENGTH);
}

void application_initcomm_section_set_cid(application_initcomm_section_t* pSection, char* cid) {
    memcpy(pSection->cid, cid, CID_LENGTH);
}

void application_initcomm_section_set_ckey(application_initcomm_section_t* pSection, char* ckey) {
    memcpy(pSection->ckey, ckey, CKEY_LENGTH);
}

static void __queue_operation_assert_success(int result, char* pcQueueName, char* pcQueueOpName, char* pcSuccessMessage) {
    if (result != pdTRUE){
        if (result == 0) {
            ESP_LOGE(TAG, "Trying to send/receive to/from the %s queue which is full/empty!", pcQueueName);
        } else {
            ESP_LOGE(TAG, "%s failed for %s, [ret val: %d]", pcQueueOpName, pcQueueName, result);
        }
        exit(EXIT_FAILURE);
    } else {
        ESP_LOGI(TAG, "%s", pcSuccessMessage);
    }
}

static void __buffer_assert_exists_nonzero_byte(char* buf, size_t len, char* errorMessage) {
    assert(len != 0);
    for (size_t i = 0; i < len; i++) {
        if (buf[i]) {
            return;
        }
    }
    ESP_LOGE(TAG, "%s", errorMessage);
    exit(EXIT_FAILURE);
}

void __tcp_app_send_control_segment(application_control_segment_t* pSegment) {
    int result = xQueueSend(__tcp_tx_queue, pSegment, portMAX_DELAY);
    __queue_operation_assert_success(result, "__tcp_tx_queue", "xQueueSend", "Application control segment queued to the __tcp_tx_queue queue.");
}

void tcp_app_send_response(application_control_segment_t* pControlSegment) {
    ESP_LOGI(TAG, "Responding to server...");
    pControlSegment->header.info.op = OP_DIR_RESPONSE(pControlSegment->header.info.op);
    __tcp_app_send_control_segment(pControlSegment);
}


#define TCP_APP_REQUEST_RESPONSE_SUCCESS 0
#define TCP_APP_REQUEST_RESPONSE_FAILURE -1
/**
 * Send application request to the server and await the response
*/
int tcp_app_request(application_control_segment_t* pControlSegment) {
    uint8_t requestOp = pControlSegment->header.info.op;
    if (requestOp != OP_DIR_REQUEST(requestOp)) {
        ESP_LOGE(TAG, "Trying to send a response segment as a request segment!");
        return TCP_APP_REQUEST_RESPONSE_FAILURE;
    }
    
    ESP_LOGI(TAG, "Sending application control segment to server");
    __tcp_app_send_control_segment(pControlSegment);
    ESP_LOGI(TAG, "Waiting for server response...");
    int result = xQueueReceive(__tcp_res_queue, pControlSegment->raw, portMAX_DELAY);
    __queue_operation_assert_success(result, "__tcp_res_queue", "xQueueReceive", "Received a response from the server.");
    uint8_t responseOp = pControlSegment->header.info.op;
    if (responseOp != OP_DIR_RESPONSE(responseOp)) {
        ESP_LOGE(TAG, "Received a request segment as a response segment!");
        return TCP_APP_REQUEST_RESPONSE_FAILURE;
    } else if (OP_DIR_REQUEST(responseOp) != requestOp) {
        ESP_LOGE(TAG, "Received a response segment with an unexpected op code! (Expected: %u, Received: %u)", requestOp, responseOp);
        return TCP_APP_REQUEST_RESPONSE_FAILURE;
    }
    return TCP_APP_REQUEST_RESPONSE_SUCCESS;
}

void tcp_app_handle_registration(char* pcUid_in, char** ppcCid_out, char** ppcCkey_in, SemaphoreHandle_t semphSync) {
    uint8_t mac[COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH];
    misc_get_netif_mac_wifi(mac);

    application_control_segment_t seg = {
        .header = {
            .info = {
                .data_length = sizeof(application_registration_section_t),
                .op = OP_DIR_REQUEST(APP_CONTROL_OP_REGISTER)
            }
        }
    };

    application_registration_section_t registrationSection = {};

    application_registration_section_set_user_id (&registrationSection, pcUid_in);
    application_registration_section_set_dev_mac(&registrationSection, mac);

    application_control_segment_set_data (&seg, registrationSection.raw);
    
    ESP_LOGI(TAG, "Requesting registration...");
    if(TCP_APP_REQUEST_RESPONSE_SUCCESS != tcp_app_request(&seg)) { // passes UID, mac to server ; obtains CID from server
        ESP_LOGE(TAG, "Failed to register with the server!");
        exit(EXIT_FAILURE);
    }

    char* cid = ((application_registration_section_t*)seg.data)->cid;
    __buffer_assert_exists_nonzero_byte(cid, CID_LENGTH, "Server was expected to generate a valid CID, but they seem to have failed");
    memcpy(*ppcCid_out, cid, CID_LENGTH);

    ESP_LOGI(TAG, "End of first stage of registration");//expect specific log after this
    ESP_LOGW(TAG, "semphSync = %p", semphSync);
    __semph_operation_assert_success(xSemaphoreGive(semphSync), "xSemaphoreGive"); //indicate end of first stage
    ESP_LOGI(TAG, "After xSemaphoreGive(semphSync)"); //debug
    vTaskDelay(100 / portTICK_PERIOD_MS); // ?
    __semph_operation_assert_success(xSemaphoreTake(semphSync, portMAX_DELAY), "xSemaphoreTake"); // wait for registration activities (storage of CID, generating ckey...)
    ESP_LOGI(TAG, "After xSemaphoreTake(semphSync, portMAX_DELAY)"); //debug

    seg.header.info.op = OP_DIR_REQUEST(seg.header.info.op);
    application_registration_section_set_ckey ((application_registration_section_t*)seg.data, *ppcCkey_in);
    
    if (TCP_APP_REQUEST_RESPONSE_SUCCESS != tcp_app_request(&seg)) { // passes CKEY to server ; obtains confirmation (any valid response) from server
        ESP_LOGE(TAG, "Failed to complete registration with the server!");
        exit(EXIT_FAILURE);
    }

    ESP_LOGI(TAG, "Calling final (xSemaphoreGive(semphSync), to indicate end of registration-related communications with server");
    __semph_operation_assert_success(xSemaphoreGive(semphSync), "xSemaphoreGive"); // indicate end of registration communications
}

void tcp_app_init_comm(char** ppCsid) {
    application_control_segment_t seg = {
        .header = {
            .info = {
                .data_length = sizeof(application_initcomm_section_t),
                .op = OP_DIR_REQUEST(APP_CONTROL_OP_INITCOMM)
            }
        }
    };

    application_initcomm_section_t initcommSection = {};
    application_initcomm_section_set_cid(&initcommSection, __comm_credentials.cid);
    application_initcomm_section_set_ckey(&initcommSection, __comm_credentials.ckey);

    application_control_segment_set_data(&seg, initcommSection.raw);

    ESP_LOGI(TAG, "Requesting initcomm...");
    if (TCP_APP_REQUEST_RESPONSE_SUCCESS != tcp_app_request(&seg)) { // passes CID, CKEY to server ; obtains CSID from server (if auth succeeded)
        ESP_LOGE(TAG, "Failed to initiate communications with the server!");
        exit(EXIT_FAILURE);
    }

    char* csid = seg.header.info.csid;
    __buffer_assert_exists_nonzero_byte(csid, COMM_CSID_LENGTH, "SERVER AUTHENTICATION FAILED!");

    comm_set_csid(csid);
    ESP_LOGI(TAG, "Server communications session csid obtained: %02x %02x %02x ... %02x", 
            (uint8_t)(csid[0]), (uint8_t)(csid[1]), (uint8_t)(csid[2]), (uint8_t)(csid[COMM_CSID_LENGTH - 1]));

    comm_set_session_initiated();
}

void tcp_app_initcomm_task(void* pvParameters) {
    char* pCsid = comm_get_csid();
    tcp_app_init_comm(&pCsid);
    vTaskDelete(NULL);
}

/*
    App request handlers (for requests received from the application server)
*/

static void tcp_app_handle_unregistration_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Unregistration requested by the server.");
    application_unregistration_section_t unregistrationSection = {};
    esp_err_t err = registration_unregister();
    if (err == ESP_OK) {
        unregistrationSection.succeeded = 1U;
        application_control_segment_set_data(pSeg, unregistrationSection.raw);
        
        tcp_app_send_response(pSeg);
        ESP_LOGI(TAG, "Waiting for queued tx segments");
        await_tx_queue_empty();

        ESP_LOGI(TAG, "Performing system reset due to unregistration - switching to BLE peripheral mode");
        ESP_LOGW(TAG, "###########################");
        ESP_LOGW(TAG, "#!!!    UNREGISTERED   !!!#");
        ESP_LOGW(TAG, "###########################");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Failed to unregister!");
        unregistrationSection.succeeded = 0U;
        application_control_segment_set_data(pSeg, unregistrationSection.raw);
        __tcp_app_send_control_segment(pSeg);
    }
}

static void tcp_app_handle_nop_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "NOP requested by the server.");
    tcp_app_send_response(pSeg); // Notify the server that we are still alive
}

static void tcp_app_handle_start_unconditional_stream_request(application_control_segment_t* pSeg) {
    //ESP_LOGE(TAG, "Not implemented"); assert(0); // [TODO] <<<<<<
    ESP_LOGI(TAG, "Unconditional stream requested by the server."); // [TODO] should the server or mobile app analyse it then?
    analyser_set_operation_mode(ANALYSER_OPERATION_MODE_UNCONDITIONAL_STREAM);
    tcp_app_send_response(pSeg);
}

static void tcp_app_handle_stop_unconditional_stream_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Unconditional stream discontinued by the server,");
    analyser_set_operation_mode(ANALYSER_OPERATION_MODE_ANALYSIS);
    //vTaskDelay(500 / portTICK_PERIOD_MS);
    tcp_app_send_response(pSeg);
}

static void tcp_app_handle_logs_mode_minimal_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Minimal logs mode requested by the server.");
    //esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("server_communications", ESP_LOG_NONE);
    esp_log_level_set("CAMAU_CONTROLLER", ESP_LOG_NONE);
    esp_log_level_set("analyser", ESP_LOG_NONE);
    
    tcp_app_send_response(pSeg);
}

static void tcp_app_handle_logs_mode_complete_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Complete logs mode requested by the server.");
    
    esp_log_level_set("server_communications", ESP_LOG_INFO);
    esp_log_level_set("CAMAU_CONTROLLER", ESP_LOG_INFO);
    esp_log_level_set("analyser", ESP_LOG_INFO);

    tcp_app_send_response(pSeg);
}

static void tcp_app_handle_software_device_reset_request(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Software device reset requested by the server. Waiting for queued tx segments...");
    await_tx_queue_empty();
    tcp_app_send_response(pSeg);
    ESP_LOGW(TAG, "Restarting the device now.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
}

static void tcp_app_handle_energy_saving_shutdown_analyser(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Analyser shutdown requested by the server,");
    analyser_shutdown();
    tcp_app_send_response(pSeg);
}

static void tcp_app_handle_energy_saving_wakeup_analyser(application_control_segment_t* pSeg) {
    ESP_LOGI(TAG, "Analyser wakeup requested by the server,");
    analyser_wakeup();
    tcp_app_send_response(pSeg);
}

/**
 * @brief Task handler for processing incoming requests
*/
void tcp_app_incoming_request_handler_task(void* pvParameters) {
    application_control_segment_t seg = {};
    while (1) {
        
        int result = xQueueReceive(__tcp_req_queue, seg.raw, portMAX_DELAY);
        __queue_operation_assert_success(result, "__tcp_req_queue", "xQueueReceive", "Received a request from the server.");

        ESP_LOGW(TAG, "RECEIVED SERVER REQUEST");
        switch (seg.header.info.op) {
            case APP_CONTROL_OP_NOP:
                tcp_app_handle_nop_request(&seg);
                break;
            case APP_CONTROL_OP_UNREGISTER:
                tcp_app_handle_unregistration_request(&seg);
                break;
            case APP_CONTROL_OP_CAM_START_UNCONDITIONAL_STREAM:
                tcp_app_handle_start_unconditional_stream_request(&seg);
                break;
            case APP_CONTROL_OP_CAM_STOP_UNCONDITIONAL_STREAM:
                tcp_app_handle_stop_unconditional_stream_request(&seg);
                break;
            case APP_CONTROL_OP_LOGS_MODE_MINIMAL:
                tcp_app_handle_logs_mode_minimal_request(&seg);
                break;
            case APP_CONTROL_OP_LOGS_MODE_COMPLETE:
                tcp_app_handle_logs_mode_complete_request(&seg);
                break;
            case APP_CONTROL_OP_SOFTWARE_DEVICE_RESET:
                tcp_app_handle_software_device_reset_request(&seg);
                break;
            case APP_CONTROL_OP_ENERGY_SAVING_SHUTDOWN_ANALYSER:
                tcp_app_handle_energy_saving_shutdown_analyser(&seg);
                break;
            case APP_CONTROL_OP_ENERGY_SAVING_WAKEUP_ANALYSER:
                tcp_app_handle_energy_saving_wakeup_analyser(&seg);
                break;
            default:
                ESP_LOGE(TAG, "Detected unknown or unhandled request! (op = %u)", seg.header.info.op);
                break;
        }
    }
}
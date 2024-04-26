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

#include "registration.h"

//#pragma pack(1) // Force compiler to pack struct members together

static const char * TAG = "server_communications";
#define HOST_IP_ADDR /*"192.168.1.15"*//*"192.168.173.32"*//*"192.168.116.32"*//*"192.168.43.32"*/"192.168.34.32"
#define UDP_PORT 3333
#define TCP_PORT 3334

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

typedef union {
    struct {
        uint32_t pkt_idx; // Chunk index (assigns chunk index to IP packet)
        jfif_chunk_type_t chunk_type; // JFIF_INTERMEDIATE_CHUNK, JFIF_FIRST_CHUNK, JFIF_LAST_CHUNK or JFIF_ONLY_CHUNK
        uint8_t data[jfif_chunk_segment_max_data_size]; // JFIF chunk data buffer
    } structure;
    uint8_t raw[jfif_chunk_segment_total_size];
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
    int err = sendto(udp_sock, data, MAX_UDP_DATA_SIZE, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
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
    uint8_t* txPtr = *jfif_buf_ptr;
    //static const uint32_t MAX_JFIF_CHUNK_SZ = MAX_UDP_DATA_SIZE - APP_DESC_SEGMENT_SIZE;

    uint32_t chunk_data_len = -1U;
    //transform_segment(jfif_buf_ptr, chunk_len, pkt_idx, JFIF_FIRST_CHUNK);
    
    jfif_chunk_segment_t seg;
    seg.structure.pkt_idx = pkt_idx;
    if (len <= jfif_chunk_segment_max_data_size) {
        chunk_data_len = len;
        seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_ONLY_CHUNK);
    } else {
        chunk_data_len = jfif_chunk_segment_max_data_size;
        seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_FIRST_CHUNK);
    }
    memcpy(seg.structure.data, txPtr, chunk_data_len);

    //transmit_udp(*jfif_buf_ptr, chunk_data_len + APP_DESC_SEGMENT_SIZE);
    transmit_udp(seg.raw, chunk_data_len + APP_DESC_SEGMENT_SIZE);
    pkt_idx++;
    remaining_bytes -= chunk_data_len;
    
    while (remaining_bytes > 0) { /** @warning remaining_bytes is unsigned, watch out */
        txPtr += chunk_data_len;
        seg.structure.pkt_idx = pkt_idx;
        if (remaining_bytes > jfif_chunk_segment_max_data_size) {
            seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK);
        }
        else {
            seg.structure.chunk_type = JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);
            chunk_data_len = remaining_bytes;
        }
        seg.structure.chunk_type = remaining_bytes > jfif_chunk_segment_max_data_size ? 
            JFIF_CHUNK_TYPE(JFIF_INTERMEDIATE_CHUNK) : 
            JFIF_CHUNK_TYPE(JFIF_LAST_CHUNK);
        //return ESP_OK; /** @test */
        memcpy(seg.structure.data, txPtr, chunk_data_len);

        transmit_udp(seg.raw, chunk_data_len + APP_DESC_SEGMENT_SIZE);
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
///////////////

/*
    TCP COMMUNICATIONS - data structures for application control
*/

#define COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH 6
#define COMM_CSID_LENGTH 16

typedef union { // op = APP_CONTROL_OP_REGISTER
    struct {
        char user_id[MAX_USER_ID_LENGTH]; // UID obtained from the client app
        char cid[CID_LENGTH]; // Cam ID - generated by server
        char ckey[CKEY_LENGTH]; // Cam auth key - generated by cam
        uint8_t dev_mac[COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH]; // WiFi sta if mac addr
    };
    uint8_t raw[MAX_USER_ID_LENGTH + COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH + CID_LENGTH + CKEY_LENGTH];
} __attribute__((packed)) application_registration_section_t; // size = 54

typedef union { // op = APP_CONTROL_OP_INITCOMM
    struct {
        char cid[CID_LENGTH]; // Cam ID
        char ckey[CKEY_LENGTH]; // Cam auth key
        char csid[COMM_CSID_LENGTH]; // Cam session ID
    };
    uint8_t raw[CID_LENGTH + CKEY_LENGTH + COMM_CSID_LENGTH];
} __attribute__((packed)) application_initcomm_section_t; // size = 48

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

#define REQUEST(op) (op)
#define RESPONSE(op) (op) | (1U << 7) 

typedef struct {
    char csid[COMM_CSID_LENGTH]; // cam session ID (zeroed-out means unspecified)
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
 *      1. TCP receiver task (writes to queue #1)
 *      2. TCP transmitter task (reads from queue #2)
 *      3. rx demultiplexer task (reads from queue #2, writes to queues #3 and #4)
 * 
 *  Queues: 
 *      1. TCP rx queue
 *      2. TCP tx queue
 *      3. Client-initiated communications 
 *      4. Server-initiated communications
*/

#define COMM_TCP_RX_QUEUE_MAX_LENGTH 10
#define COMM_TCP_TX_QUEUE_MAX_LENGTH 10
#define COMM_TCP_REQ_QUEUE_MAX_LENGTH 10
#define COMM_TCP_RES_QUEUE_MAX_LENGTH 10

static QueueHandle_t __tcp_rx_queue;
static QueueHandle_t __tcp_tx_queue;
static QueueHandle_t __tcp_req_queue;
static QueueHandle_t __tcp_res_queue;

static SemaphoreHandle_t __tcp_shutdown_semph;


void __tcp_rx_task(void *pvParameters) {
    while (1) {
        application_control_segment_t seg = {}; // [TODO] verify if xQueueSend copies its raw buffer (it should as docs say so), otherwise move it outside the loop
                                                // also, skip segment init?
        ssize_t rv = recv(tcp_sock, seg.header.raw, sizeof(seg.header), 0);
        if (rv == sizeof(seg.header)) {
            if (seg.header.info.data_length > 0) {
                rv = recv(tcp_sock, seg.data, seg.header.info.data_length, 0);
                if (rv == seg.header.info.data_length) {
                    
                    int result = xQueueSend(__tcp_rx_queue, seg.raw, 0); // pass application segment to the rx queue
                    
                    if (result != pdTRUE){
                        if (result == errQUEUE_FULL) {
                            ESP_LOGE(TAG, "Trying to send to the __tcp_rx_queue queue which is full!");
                        } else {
                            ESP_LOGE(TAG, "xQueueSend(__tcp_rx_queue, seg.raw, 0) failed, [ret val: %d]", result);
                        }
                        xSemaphoreGive(__tcp_shutdown_semph); // reset connection
                    } else {
                        ESP_LOGI(TAG, "Application control segment received into the RX queue.");
                    }
                } else {
                    if (rv == 0) {
                        ESP_LOGE(TAG, "Received empty seg data, when non-empty seg data was expected!");
                    } else if (rv > 0) {
                        if (rv < seg.header.info.data_length) {
                            ESP_LOGE(TAG, "Received incomplete seg data!");
                        } else {
                            ESP_LOGE(TAG, "Unexpectedly received more data bytes than expected seg data length!");
                        }
                    } else {
                        ESP_LOGE(TAG, "Error while receiving from TCP socket (trying to receive app seg data bytes) [errno = %d]", errno);
                    }
                    xSemaphoreGive(__tcp_shutdown_semph); // reset connection
                }
                
            }
        } else {
            if (rv == 0) {
                ESP_LOGE(TAG, "Received empty header!");
            } else if (rv > 0) {
                if (rv < sizeof(seg.header)) {
                    ESP_LOGE(TAG, "Received incomplete header!");
                } else {
                    ESP_LOGE(TAG, "Unexpectedly received more header bytes than expected header length!");
                }
            } else {
                ESP_LOGE(TAG, "Error while receiving from TCP socket (trying to receive app seg header bytes) [errno = %d]", errno);
            }
            xSemaphoreGive(__tcp_shutdown_semph); // reset connection
        }
    }
}

void __tcp_tx_task(void* pvParameters) {
    while (1) {
        application_control_segment_t seg = {}; //skip segment init?
        if (xQueueReceive(__tcp_tx_queue, seg.raw, 0)) {
            
            uint32_t tx_seg_len = sizeof(application_control_segment_info_t) + seg.header.info.data_length;
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
                xSemaphoreGive(__tcp_shutdown_semph); // reset connection
            }
        } else {
            ESP_LOGE(TAG, "Failure while receiving from the __tcp_tx_queue queue");
            xSemaphoreGive(__tcp_shutdown_semph); // reset connection
        }
    }
}

void __tcp_demux_task(void* pvParametrs) {
    while (1) {
        ESP_LOGE(TAG, "Not implemented");
    }
}


void tcp_connection_manage_task(void* pvParameters) { // [TODO] Keepalive ?
    __tcp_shutdown_semph = xSemaphoreCreateBinary();
    if (__tcp_shutdown_semph == 0) {
        ESP_LOGE(TAG, "No memory for __tcp_shutdown_semph");
        //return ESP_ERR_NO_MEM;
        exit(EXIT_FAILURE);
    }
    
    __tcp_rx_queue = xQueueCreate(COMM_TCP_RX_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_rx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_rx_queue");
        exit(EXIT_FAILURE);
    }

    __tcp_tx_queue = xQueueCreate(COMM_TCP_TX_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_tx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_tx_queue");
        exit(EXIT_FAILURE);
    }

    __tcp_req_queue = xQueueCreate(COMM_TCP_REQ_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_rx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_req_queue");
        exit(EXIT_FAILURE);
    }

    __tcp_res_queue = xQueueCreate(COMM_TCP_RES_QUEUE_MAX_LENGTH, COMM_MAX_APP_SEG_SIZE);
    if (__tcp_tx_queue == 0) {
        ESP_LOGE(TAG, "Couldn't create __tcp_res_queue");
        exit(EXIT_FAILURE);
    }


    while (1) { // tcp managing infinite loop

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

        while (1) {
            rv = connect(tcp_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (rv == 0) {
                break;
            } else  {
                ESP_LOGE(TAG, "Unable to connect TCP socket: errno %d", errno);
            }
        }
        
        ESP_LOGI(TAG, "TCP socket connected successfully");

        xSemaphoreTake(__tcp_shutdown_semph, portMAX_DELAY);

        if (tcp_sock != -1) {
            ESP_LOGE(TAG, "Shutting down TCP socket...");
            shutdown(tcp_sock, 0);//SHUT_RDWR
            close(tcp_sock); 
        }

    } // end of tcp managing infinite loop
}
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "registration.h"

#define SERVER_COMMUNICATIONS_USE_WOLF_SSL 0 // If set to 1, the traffic will be encrypted

#define COMM_CSID_LENGTH 16
#define COMM_NETIF_WIFI_STA_MAC_ADDR_LENGTH 6

#define APP_DESC_SEGMENT_SIZE (5 + COMM_CSID_LENGTH)
#define MAX_UDP_DATA_SIZE /*65500*//*32750*/16375/*4096*/
#define jfif_chunk_segment_total_size MAX_UDP_DATA_SIZE
#define jfif_chunk_segment_max_data_size (MAX_UDP_DATA_SIZE - APP_DESC_SEGMENT_SIZE)

typedef enum {
    APP_TCP_COMM_MODE_REGISTRATION,
    APP_TCP_COMM_MODE_REGULAR
} app_tcp_comm_mode_t;

typedef struct {
    char cid[CID_LENGTH];
    char ckey[CKEY_LENGTH];
} app_comm_credentials_t;

typedef enum {
    JFIF_INTERMEDIATE_CHUNK = 0x0U,
    JFIF_FIRST_CHUNK = 0x1U,
    JFIF_LAST_CHUNK = 0x2U,
    JFIF_ONLY_CHUNK = 0x3U
} jfif_chunk_enum_t;

typedef unsigned char jfif_chunk_type_t;

#define JFIF_CHUNK_TYPE(x) (unsigned char)(x)

esp_err_t begin_udp_stream();
void end_udp_stream();
void transform_segment(uint8_t** jfif_buf_ptr, size_t len, size_t packet_idx, jfif_chunk_type_t chunk_type);
esp_err_t transmit_udp(uint8_t* data, size_t len);
esp_err_t transmit_jfif(uint8_t** jfif_buf_ptr, size_t len);
extern void x_on_udp_transmission_end();


//void app_comm_set_registration_data(registration_data_t registrationData);
void app_comm_set_credentials(char* cid, char* ckey);

void app_tcp_set_mode(app_tcp_comm_mode_t mode);
//void setup_and_manage_tcp_connection();
void tcp_connection_manage_task(void* pvParameters);
void tcp_app_incoming_request_handler_task(void* pvParameters);

void tcp_app_handle_registration(char* pcUid_in, char** ppcCid_out, char** ppcCkey_in, SemaphoreHandle_t semphSync);
//void tcp_app_send()
void tcp_app_init_comm(char** ppCsid);
void tcp_app_initcomm_task(void* pvParameters);

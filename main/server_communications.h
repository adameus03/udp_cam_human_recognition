#include <stdint.h>
#include "esp_err.h"

#define SERVER_COMMUNICATIONS_USE_WOLF_SSL 0 // If set to 1, the traffic will be encrypted


#define APP_DESC_SEGMENT_SIZE 5
#define MAX_UDP_DATA_SIZE /*65500*//*32750*/16375/*4096*/
#define jfif_chunk_segment_total_size MAX_UDP_DATA_SIZE
#define jfif_chunk_segment_max_data_size (MAX_UDP_DATA_SIZE - APP_DESC_SEGMENT_SIZE)

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


//void setup_and_manage_tcp_connection();
void tcp_connection_manage_task(void* pvParameters);
void tcp_app_incoming_request_handler_task(void* pvParameters);

void tcp_app_handle_registration_1(char* pcUid_in, uint8_t* puMac_in, char** ppcCid_out);
//void tcp_app_send()
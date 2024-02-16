#include "server_communications.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//#pragma pack(1) // Force compiler to pack struct members together

static const char * TAG = "server_communications";
#define HOST_IP_ADDR "192.168.1.15"
#define PORT 3333

char host_ip[] = HOST_IP_ADDR;
int addr_family = AF_INET;
int ip_protocol = IPPROTO_IP;
int sock = -1;
struct sockaddr_in dest_addr;

esp_err_t begin_udp_stream() {
    ESP_LOGI(TAG, "Initialising UDP stream");
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    sock = socket(addr_family, SOCK_DGRAM/*experiment*/, ip_protocol);
    if (sock < 0) {
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

    setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tx_timeout, sizeof tx_timeout);
    setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &rx_timeout, sizeof rx_timeout);

    // set socket to blocking mode (clear O_NONBLOCK)
    int opts = fcntl(sock, F_GETFL);
    opts = opts & (~O_NONBLOCK);
    fcntl(sock, F_SETFL, opts);

    ESP_LOGI(TAG, "Socket created, for sending to %s:%d", HOST_IP_ADDR, PORT);
    return ESP_OK;
}

void end_udp_stream() {
    ESP_LOGI(TAG, "Ending UDP stream");
    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket...");
        shutdown(sock, 0);
        close(sock);
    }
    ESP_LOGI(TAG, "Socket closed");
    x_on_udp_transmission_end();
}

#define MAX_UDP_DATA_SIZE /*32768*/ 16384
#define CHUNK_DELAY_MS 100

esp_err_t transmit_udp(uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "Sending decoy signal");
    int err = sendto(sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Decoy signal sent, waiting for server to respond");
    err = recvfrom(sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, NULL);
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
        int err = sendto(sock, data + i * blk_size, blk_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        vTaskDelay(CHUNK_DELAY_MS / portTICK_PERIOD_MS);
        if (err < 0) {
            err_count++;
        }
    }
    if (rem > 0) {
        int err = sendto(sock, data + num_blks * blk_size, rem, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
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
}

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
        int err = sendto(sock, data + i * blk_size, blk_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            // Don't print an error message for each block, it's too much
            //ESP_LOGD(TAG, "Error occurred during sending blk %d: errno %d", i, errno);
            err_count++;
            //break;
        }
        
        
        if (!server_alive) {
            // receive empty packet to confirm that the data was received
            // uint8_t pingbuf[1];
            // err = recvfrom(sock, pingbuf, 1, 0, (struct sockaddr *)&dest_addr, NULL);

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
        int err = sendto(sock, data + num_blks * blk_size, rem, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            //ESP_LOGD(TAG, "Error occurred during sending: errno %d", errno);
            err_count++;
        }
        //recvfrom(sock, NULL, 0, 0, (struct sockaddr *)&dest_addr, NULL);
        //else if (err != rem) {
        //    ESP_LOGE(TAG, "FATAL error occurred during sending: sent %d bytes, expected %d", err, rem);
        //}
    }
    
    // /^// Send a message to say the data is complete
    // int err = sendto(sock, "END_01234321", 12, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); // END_01234321 is a unique message to indicate the end of the data
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
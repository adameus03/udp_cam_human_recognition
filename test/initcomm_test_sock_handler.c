#include "test_handlers.h"

int client_sock_handler(int client_sock) {
    

    //uint8_t rx_buf[COMM_MAX_APP_SEG_SIZE];
    //memset(rx_buf, 0, COMM_MAX_APP_SEG_SIZE);
    //printf("Ha! And now, I won't receive anything. Deal with it!\n");
    //exit(EXIT_FAILURE);

    return initcomm_sock_handler(client_sock);

}
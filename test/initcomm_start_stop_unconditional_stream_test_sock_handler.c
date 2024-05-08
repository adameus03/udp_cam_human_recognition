#include "test_handlers.h"

int client_sock_handler(int client_sock) {
    
    if (0 != initcomm_sock_handler(client_sock)) {
        return 1;
    }
    if (0 != start_unconditional_stream_sock_handler(client_sock)) {
        return 1;
    }
    if (0 != stop_unconditional_stream_sock_handler(client_sock)) {
        return 1;
    }
    return 0;

}
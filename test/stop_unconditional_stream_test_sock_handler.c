#include "test_handlers.h"

int client_sock_handler(int client_sock) {

    return stop_unconditional_stream_sock_handler(client_sock);

}
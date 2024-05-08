#include "test_handlers.h"

int client_sock_handler(int client_sock) {

    return registration_sock_handler(client_sock);
    /*// RECEIVE ANY FURTHER REQUEST HEADER just for testing purposes (for now; and to keep the connection alive)

    rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf("Received seg header has op = %u\n", seg.header.info.op);

    printf("ENDING FOR NOW\n");*/

    //return 0;

}
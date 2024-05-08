#include "def.h"

char host_ip[] = SERV_IP_ADDR;
int addr_family = AF_INET;
int ip_protocol = IPPROTO_IP;
int tcp_sock = -1;
struct sockaddr_in serv_addr;

int main() {
    printf("esp dummy server test\n");

    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP_ADDR);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);

    printf("After set server params\n");

    int tcp_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (tcp_sock < 0) {
        fprintf(stderr, "Failed to create socket!\n");
        return 1;
    }

    printf("Created socket successfully\n");

    int rv = bind(tcp_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rv != 0) {
        fprintf(stderr, "Failed to bind socket!\n");
        return 1;
    }

    printf("Socket bind successfully\n");

    printf("Now listening...\n");
    rv = listen(tcp_sock, 1);
    if (rv != 0) {
        fprintf(stderr, "Failed to listen socket\n");
        return 1;
    }

    printf("Listen returned\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    int client_sock;

    client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    //shutdown(client_sock, 0);
    //return 0;
    if (client_sock < 0) {
        fprintf(stderr, "Failed to accept (%d)\n", client_sock);
        return 1;
    }

    printf("Accepted\n");
    
    printf("Calling client_sock_handler for sockfd = %d\n", client_sock);
    rv = client_sock_setup(client_sock);
    if (rv == 0) {
        printf("client_sock_setup returned successully\n");
    } else {
        fprintf(stderr, "client_sock_setup returned error code (%d)\n", rv);
        return 1;
    }
    
    rv = client_sock_handler(client_sock);
    if (rv == 0) {
        printf("client_sock_handler returned successully\n");
    } else  {
        fprintf(stderr, "client_sock_handler returned error code (%d)\n", rv);
        return 1;
    }
    if (client_sock != -1) {
        printf("Shutting down client_sock\n");
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
    }
    if (tcp_sock != -1) {
        printf("Shutting down tcp_sock\n");
        shutdown(tcp_sock, SHUT_RDWR);
        close(tcp_sock);
    }

    return 0;
}

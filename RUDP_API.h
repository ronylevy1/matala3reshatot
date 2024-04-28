#ifndef RUDP_API_H
#define RUDP_API_H

#include <stdbool.h>
#include <netinet/in.h>

typedef struct {
    int socket_fd;
    bool isServer;
    bool isConnected;
    struct sockaddr_in dest_addr;
} RUDP_Socket;

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port);
RUDP_Socket* rudp_connect(RUDP_Socket* sockfd, const char *dest_ip, unsigned short int dest_port);
RUDP_Socket* rudp_accept(RUDP_Socket* sockfd);
ssize_t rudp_recv(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size);
ssize_t rudp_send(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size);
RUDP_Socket* rudp_disconnect(RUDP_Socket* sockfd);
RUDP_Socket* rudp_close(RUDP_Socket* sockfd);
int rudp_listen(RUDP_Socket* sockfd, int backlog);

#endif /* RUDP_API_H */

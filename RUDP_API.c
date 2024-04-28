#include "RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    RUDP_Socket* build = malloc(sizeof(RUDP_Socket));
    if (build == NULL) {
        perror("Error allocating memory for socket structure");
        return NULL;
    }
    
    build->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (build->socket_fd == -1) {
        perror("Error opening socket");
        free(build);
        return NULL;
    }
    
    build->isServer = isServer;

    if (isServer) {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(listen_port);

        if (bind(build->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error binding socket");
            close(build->socket_fd);
            free(build);
            return NULL;
        }
    }

    return build;
}

RUDP_Socket* rudp_connect(RUDP_Socket* sockfd, const char *dest_ip, unsigned short int dest_port){
     struct sockaddr_in dest_addr;
    
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    dest_addr.sin_addr.s_addr = inet_addr(dest_ip);
    if (dest_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invalid IP\n");
        return NULL; 
    }

    if (connect(sockfd->socket_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Error connecting");
        return NULL;  
    }

    return sockfd;
}

RUDP_Socket* rudp_accept(RUDP_Socket* sockfd) {
    if (!sockfd->isServer) {
        fprintf(stderr, "Error: Cannot accept on a client socket\n");
        return NULL;
    }

    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    
    ssize_t bytes_received = recvfrom(sockfd->socket_fd, NULL, 0, 0, (struct sockaddr *)&cli_addr, &len);
    if (bytes_received == -1) {
        perror("Error receiving connection request");
        return NULL;
    }

    RUDP_Socket* new_sock = malloc(sizeof(RUDP_Socket));
    if (new_sock == NULL) {
        perror("Error allocating memory for new socket");
        return NULL;
    }

    new_sock->socket_fd = sockfd->socket_fd;
    new_sock->isServer = false;
    new_sock->isConnected = true;
    new_sock->dest_addr = cli_addr;

    return new_sock;
}

ssize_t rudp_recv(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size) {
    if (!sockfd->isConnected) {
        fprintf(stderr, "Error: Socket is not connected\n");
        return -1;
    }

    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    ssize_t bytes_received = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, (struct sockaddr*)&sender_addr, &sender_addr_len);
    if (bytes_received == 0) {
        sockfd->isConnected = false;
    } else if (bytes_received == -1) {
        perror("Error receiving data");
    }

    return bytes_received;
}

ssize_t rudp_send(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size){
    ssize_t total_bytes_sent = 0;
    ssize_t bytes_sent = 0;
    char *ptr = (char*)buffer;

    while (total_bytes_sent < buffer_size) {
        bytes_sent = sendto(sockfd->socket_fd, ptr + total_bytes_sent, buffer_size - total_bytes_sent, 0,
                            (struct sockaddr *)&(sockfd->dest_addr), sizeof(sockfd->dest_addr));
        
        if (bytes_sent == -1) {
            perror("Error sending data");
            return -1;
        } else if (bytes_sent == 0) {
            printf("FIN packet received.\n");
            return 0;
        }

        total_bytes_sent += bytes_sent;
    }

    return total_bytes_sent;
}

RUDP_Socket* rudp_disconnect(RUDP_Socket* sockfd) {
    if (sockfd->isConnected) {
        sockfd->isConnected = false;
        return sockfd;
    }
    return NULL;
}

RUDP_Socket* rudp_close(RUDP_Socket* sockfd) {
    close(sockfd->socket_fd);
    free(sockfd);
    return NULL;
}

int rudp_listen(RUDP_Socket* sockfd, int backlog) {
    if (listen(sockfd->socket_fd, backlog) == -1) {
        perror("Error listening for connections");
        return -1;
    }

    return 0;
}

#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "RUDP_API.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    // Allocate memory for the RUDP socket structure
    RUDP_Socket* build = malloc(sizeof(RUDP_Socket));
    if (build == NULL) {
        perror("Error allocating memory for socket structure");
        return NULL;
    }
    
    // Create a UDP socket
    build->socket_fd = socket(AF_INET, SOCK_DGRAM, 0); // 0 for the UDP protocol
    if (build->socket_fd == -1) {
        perror("Error opening socket");
        free(build); // Free the allocated memory
        return NULL;
    }
    
    // Set whether the socket is a server or client
    build->isServer = isServer;

    // If it's a server socket, bind it to the specified port
    if (isServer) {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all available interfaces
        server_addr.sin_port = htons(listen_port); // Set the port

        // Bind the socket
        if (bind(build->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error binding socket");
            close(build->socket_fd);
            free(build);
            return NULL;
        }
    }

    return build;
}


// Tries to connect to the other side via RUDP to given IP and port. Returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to server.
RUDP_Socket* rudp_connect(RUDP_Socket* sockfd, const char *dest_ip, unsigned short int dest_port){
    
    memset(&sockfd->dest_addr, 0, sizeof(struct sockaddr_in));
    sockfd->dest_addr.sin_family = AF_INET;
    sockfd->dest_addr.sin_port = htons(dest_port);
    sockfd->dest_addr.sin_addr.s_addr = inet_addr(dest_ip);
    if (sockfd->dest_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invaild ip\n");
        return NULL; 
    }

    if (connect(sockfd->socket_fd, (struct sockaddr *)&sockfd->dest_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error connecting");
        return NULL;  
    }

    sockfd->isConnected = true;

    return sockfd;
}

// Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to client.
RUDP_Socket* rudp_accept(RUDP_Socket* sockfd) {
    if (!sockfd->isServer) {
        fprintf(stderr, "Error: Cannot accept on a client socket\n");
        return NULL;
    }

    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    // Receive a message to establish connection
    char buffer[60000] = {0}; 
    ssize_t bytes_received = recvfrom(sockfd->socket_fd, buffer, 60000, 0, (struct sockaddr *)&cli_addr, &len);
    if (bytes_received == -1) {
        perror("Error receiving connection request");
        return NULL;
    }

    // Create a new RUDP_Socket for the accepted connection
    RUDP_Socket* new_sock = (RUDP_Socket*)malloc(sizeof(RUDP_Socket));
    if (new_sock == NULL) {
        perror("Error allocating memory for new socket");
        return NULL;
    }

    // Initialize the new socket parameters
    new_sock->socket_fd = sockfd->socket_fd; // Reuse the same socket file descriptor
    new_sock->isServer = false; // This socket is now a client
    new_sock->isConnected = true;
    new_sock->dest_addr = cli_addr;

    return new_sock;
}



// Receives data from the other side and put it into the buffer. Returns the number of received bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
ssize_t rudp_recv(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size) {
    if (!sockfd->isConnected) {
        fprintf(stderr, "Error: Socket is not connected\n");
        return -1;
    }

    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    ssize_t bytes_received = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, (struct sockaddr*)&sender_addr, &sender_addr_len);
    if (bytes_received == 0) {
        // Got FIN packet (disconnect)
        sockfd->isConnected = false;
    } else if (bytes_received == -1) {
        perror("Error receiving data");
    }

    return bytes_received;
}



// Sends data stores in buffer to the other side. Returns the number of sent bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
ssize_t rudp_send(RUDP_Socket* sockfd, void *buffer, unsigned int buffer_size){
    ssize_t total_bytes_sent = 0;
    ssize_t bytes_sent = 0;
    char *ptr = (char*)buffer;

    // Keep sending until all data in the buffer is sent
    while (total_bytes_sent < buffer_size) {
        // Attempt to send data
        bytes_sent = sendto(sockfd->socket_fd, ptr + total_bytes_sent, buffer_size - total_bytes_sent, 0,NULL,0);
        
        // Check for errors
        if (bytes_sent == -1) {
            perror("Error sending data");
            return -1; // Error sending data
        } else if (bytes_sent == 0) {
            printf("FIN packet received.\n");
            return 0; // FIN packet received
        }

        // Update the total number of bytes sent
        total_bytes_sent += bytes_sent;
    }

    return total_bytes_sent;
}


// Disconnects from an actively connected socket. Returns 1 on success, 0 when the socket is already disconnected (failure).
RUDP_Socket* rudp_disconnect(RUDP_Socket* sockfd) {
    // Disconnect only if the socket is currently connected
    if (sockfd->isConnected) {
        sockfd->isConnected = false;
        return sockfd; // Success
    }
    return 0; // Already disconnected
}

// This function releases all the memory allocation and resources of the socket.
RUDP_Socket* rudp_close(RUDP_Socket* sockfd) {
    // Close the UDP socket
    close(sockfd->socket_fd);
    free(sockfd);
    return NULL;
}


// Function to listen for incoming connections on the RUDP socket
int rudp_listen(RUDP_Socket* sockfd, int backlog) {
    // Listen for incoming connections
    if (listen(sockfd->socket_fd, backlog) == -1) {
        perror("Error listening for connections");
        return -1;
    }

    return 0; // Success
}
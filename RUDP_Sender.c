#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include "RUDP_API.h"

    

#define BUFFER_SIZE 1024

char *util_generate_random_data(unsigned int size)
{
    char *buffer = NULL;
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    if (buffer == NULL)
        return NULL;
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}


int main(int argc, char *argv[]){
    char *exit_msg = "EXIT";

    if (argc != 5)
    {
        printf("Usage: %s <ip> -p <port> \n", argv[0]);
        return 1;
    }

    char *port = argv[4];
    int receiver_port = atoi(port);
    char *ip = argv[2];

    int size = 2 * 1024;
    char *data = util_generate_random_data(size);

    RUDP_Socket *sock = rudp_socket(false,receiver_port);

    if (sock == NULL)
    {
        perror("socket creation failed\n");
        return 1;
    }

    fprintf(stdout, "Connecting to %s:%d...\n", ip, receiver_port);

    if (rudp_connect(sock, ip, receiver_port) == NULL) 
    {
        perror("connect(2)");
        close(sock->socket_fd);
        return 1;
    }

    fprintf(stdout, "Successfully connected to the server!\n");

    int bytes_sent = 0;
    int total_bytes = 0;

    while (1)
    {

        while (total_bytes < size)
        {
            bytes_sent = rudp_send(sock, data + total_bytes, size - total_bytes);

            if (bytes_sent <= 0)
            {
                perror("Sending message failed");
                close(sock->socket_fd);
                break;
            }
            total_bytes += bytes_sent;
        }

        total_bytes = 0;

        printf("Send the file again? y or n: ");
        char user_choose;
        scanf(" %c", &user_choose);

        if (user_choose == 'n')
        {
            break;
        }
        while (user_choose != 'y' && user_choose != 'n')
        {
            printf("What you enter isn't legal\n");
            printf("Send the file again ? y or n: ");
            scanf(" %c", &user_choose);
        }
    }

    bytes_sent = rudp_send(sock, exit_msg, strlen(exit_msg));

    if (bytes_sent <= 0)
    {
        perror("Sending exit message failed");
    }

    rudp_disconnect(sock);
    rudp_close(sock);
    free(data);
    fprintf(stdout, "Connection closed!\n");

    return 0;
}

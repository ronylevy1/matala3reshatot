#include <stdio.h>      // Standard input/output library
#include <arpa/inet.h>  // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h>     // For the close function
#include <string.h>     // For the memset function
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include "RUDP_API.h"



/*
 * maximum number of senders is 1.
 */
#define MAX_SENDERS 1

/*
 * @brief The buffer size to store the received message.
 * @note The default buffer size is 1024.
 */
#define BUFFER_SIZE 1024

char *util_generate_random_data(unsigned int size)
{
    char *buffer = NULL;
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("not valid");
        return 1;
    }
    char *port = argv[1];


    ssize_t bytes_received = 0;
    clock_t beginning_t, finish_t;
    double total_t;

    char buffer[BUFFER_SIZE];
    int port_num = atoi(port);

    // Try to create a RUDP socket (IPv4, stream-based, default protocol).
    RUDP_Socket *sock = rudp_socket(true, port_num);
    if (sock == NULL)
    {
        perror("socket(2)");
        return 1;
    }

    RUDP_Socket *sender_sock = rudp_accept(sock);
    if (sender_sock == NULL)
    {
        perror("accept(2)");
        rudp_close(sock);
        return 1;
    }

    struct sockaddr_in sender;
    memset(&sender, 0, sizeof(sender));
    // Print a message to the standard output to indicate that a new client has connected.
    fprintf(stdout, "Sender %s:%d connected\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));

    // Create a buffer to store the received message.

    // Create the file that we adding an infromation to
    FILE *file = fopen("print_file", "w+");
    if (file == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    fprintf(file, "\n\n- - - - - - - - - - - - - - -\n");
    fprintf(file, "-       * Statistics *       -\n");
    double average_t = 0;
    double average_s = 0;

    bool listen_flag = true;
    int opt = 1;
    while (listen_flag)
    {
        // size_t total_bytes_sent = 0;

        beginning_t = clock();

        // Receive a message from the sender and store it in the buffer.
        bytes_received = rudp_recv(sender_sock, buffer, 2 * 1024 * 1024);

        // total_bytes_sent += bytes_received;

        // If the message receiving failed, print an error message and return 1.
        if (bytes_received < 0)
        {
            perror("message receiving failed");
            close(sender_sock->socket_fd);
            close(sock->socket_fd);
            return 1;
        }

        const char *exit_msg = "EXIT";
        if (strncmp(buffer, exit_msg, strlen(exit_msg)) == 0)
        {
            listen_flag = false;
            break;
        }

        finish_t = clock();

        total_t = ((double)finish_t - beginning_t) / CLOCKS_PER_SEC;

        average_t = average_t + total_t;
        average_s = average_s + (2 / total_t);

        fprintf(file, "- Run #%d Data: Time=%fms ; Speed=%fMB/s\n", opt, total_t, (2 / total_t));

        opt++;
    }
    
    fprintf(file, "- Average time: %fms\n", average_t / (opt - 1));
    fprintf(file, "- Average bandwidth: %fMB/S\n", average_s / (opt - 1));
    fprintf(file, "- - - - - - - - - - - - - - - \n");

    rewind(file);
    char print[1000];
    while (fgets(print, 1000, file) != NULL)
    {
        printf("%s", print);
    }

    // Check if the file is close
    if (fclose(file) != 0)
    {
        perror("Error closing file");
        return 1;
    }
    fprintf(stdout, "Sender %s:%d disconnected \n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));

    fprintf(stdout, "Receiver end.\n");

    rudp_close(sock);
    return 0;
}

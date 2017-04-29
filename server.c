#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "err.h"
#include "utils.h"

// TODO how to define this value
#define BUFFER_SIZE 1000
#define MAX_CLIENTS 42
struct sockaddr_in* clients[MAX_CLIENTS];

/**
 * Add client address to an array, if there is a place for it.
 */
void add_client_address(struct sockaddr_in* client_address) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] == NULL) {
            clients[i] = client_address;
            break;
        }
}

/**
 * Send udp datagram.
 * @buffer message to send
 * @len length of message to send
 * @client_address pointer to a structure with a client address
 * @sock socket
 */
void send_udp(char* buffer, size_t len, struct sockaddr_in* client_address, socklen_t snda_len, int sock) {
    buffer[len] = '\0';
    int sflags = 0;
    ssize_t snd_len = sendto(sock, buffer, (size_t) len, sflags,
                     (struct sockaddr *) client_address, snda_len);
    if (snd_len != len)
        syserr("error on sending datagram to client socket");

}

/**
 * Receive udp datagram.
 * @buffer buffer for the message
 * @client_address pointer to a structure with client address
 * @sock socket
 * @return length of received message
 */
size_t receive_udp(char* buffer, socklen_t* rcva_len, int sock) {
    struct sockaddr_in* client_address = malloc(sizeof(struct sockaddr_in));
    *rcva_len = (socklen_t) sizeof(*client_address);
    int flags = 0; // we do not request anything special
    size_t len = (size_t) recvfrom(sock, buffer, sizeof(buffer), flags,
                   (struct sockaddr *) client_address, rcva_len);

    add_client_address(client_address);

    if(len < 0) {
        syserr("error on datagram from client socket");
    }
    else {
        (void) printf("read from socket: %zd bytes: %.*s\n", len, (int) len, buffer);
    }
    return len;
}

/**
 * Appends content of the file to the buffer.
 * @buffer buffer
 * @filename name of the file
 * @len position to start append
 * @return new length of the buffer
 */
size_t append_file_content(char* buffer, char* filename, size_t len) {
    size_t read_bytes = 0;
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
        syserr("No such file: %s", filename);

    read_bytes = fread(buffer + len, sizeof(char), BUFFER_SIZE - len - 1, fp);
    fclose(fp);

    len += read_bytes;
    return len;
}

/**
 * Send udp datagram to all clients.
 * @buffer message to send
 * @len length of message to send
 * @sock socket
 */
void send_to_all(char* buffer, size_t len, socklen_t snda_len, int sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL)
            send_udp(buffer, len, clients[i], snda_len, sock);
    }
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fatal("Usage: %s port filename... \n", argv[0]);
    }

    if (!is_num(argv[1]))
        fatal("Port should be a number");

    uint16_t port = atoi(argv[1]);
    char* filename = argv[2];

    printf("%d %s\n", port, filename);

    //********************************************

    int sock;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char buffer[BUFFER_SIZE];
    socklen_t snda_len, rcva_len;
    ssize_t len;

    sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (sock < 0)
        syserr("socket");

    server_address.sin_family = AF_INET; //IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //listening on all interfaces
    server_address.sin_port = htons(port);

    //********************************************

    // bin the socket to a concrete address
    if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    snda_len = (socklen_t) sizeof(client_address);

    do {
            len = receive_udp(buffer, &rcva_len, sock);
            len = append_file_content(buffer, filename, len);
            send_to_all(buffer, len, snda_len, sock);
    } while(len > 0);


    return 0;
}

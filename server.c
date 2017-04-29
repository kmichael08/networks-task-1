#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "err.h"

#define BUFFER_SIZE 1000
#define MAX_CLIENTS 42

/* Check if string is a number */
int is_num(char* num) {
    int i = 0;
    size_t len = strlen(num);
    for (; i < len; i++) {
        if (!isdigit(*(num+i)))
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fatal("Usage: %s port filename... \n", argv[0]);
    }

    if (!is_num(argv[1]))
        fatal("Port should be a number");

    uint16_t port = atoi(argv[1]);
    char* filename = argv[2];

    FILE *fp;
    fp = fopen(filename, "r");

    if (fp == NULL)
        syserr("No such file: %s", filename);

    printf("%d %s\n", port, filename);

    //********************************************

    int sock;
    int flags, sflags;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char buffer[BUFFER_SIZE];
    socklen_t snda_len, rcva_len;
    ssize_t len, snd_len;

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

    for(;;) {
        do {
            rcva_len = (socklen_t) sizeof(client_address);
            flags = 0; // we do not request anything special
            len = recvfrom(sock, buffer, sizeof(buffer), flags,
                           (struct sockaddr *) &client_address, &rcva_len);
            if(len < 0)
                syserr("error on datagram from client socket");
            else {
                (void) printf("read from socket: %zd bytes: %.*s\n", len, (int) len, buffer);
                sflags = 0;
                snd_len = sendto(sock, buffer, (size_t) len, sflags,
                                 (struct sockaddr *) &client_address, snda_len);
                if (snd_len != len)
                    syserr("error on sending datagram to client socket");

            }

        } while(len > 0);
    }

    fclose(fp);
    return 0;
}

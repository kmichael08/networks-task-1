#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "err.h"
#include "utils.h"

// TODO how to define this value
#define BUFFER_SIZE 1000
#define secs_1717_year -7983878400
#define secs_4243_year 71697398400

int main(int argc, char* argv[]) {

    if (argc < 4)
        fatal("Usage: %s timestamp c host [port]", argv[0]);

    if (!is_num(argv[1]))
        fatal("timestamp should be a number");

    /* TODO May not work properly!! */
    int64_t timestamp = atoll(argv[1]);
    char *timestamp_str = argv[1];

    /* TODO check this values also in C */
    if (timestamp < secs_1717_year || timestamp >= secs_4243_year)
        fatal("Year should be in [1717, 4242]");

    if (!is_one_char(argv[2]))
        fatal("Just one character");

    char *c = argv[2];
    char *host = argv[3];

    int port = 20160;

    if (argc == 5) {
        if (!is_num(argv[4]))
            fatal("Port should be a number");
        port = atoi(argv[4]);
    }

    printf("%lld %s %s %d\n", timestamp, c, host, port);

    //**************************************************
    int sock;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    int flags, sflags;
    char buffer[BUFFER_SIZE];
    size_t len;
    ssize_t snd_len, rcv_len;
    struct sockaddr_in my_address;
    struct sockaddr_in srvr_address;
    socklen_t rcva_len;

    // 'converting' host/port in string to struct addrinfo
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    if (getaddrinfo(host, NULL, &addr_hints, &addr_result) != 0) {
        syserr("getaddrinfo");
    }

    my_address.sin_family = AF_INET; // IPv4
    my_address.sin_addr.s_addr =
            ((struct sockaddr_in *) (addr_result->ai_addr))->sin_addr.s_addr; // address IP

    my_address.sin_port = htons((uint16_t) port); // port from the command line
    freeaddrinfo(addr_result);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    size_t timestamp_len = strnlen(timestamp_str, BUFFER_SIZE);
    strncpy(buffer, timestamp_str, timestamp_len);
    strncpy(buffer + timestamp_len, c, 2);

    len = timestamp_len + 1;
    sflags = 0;
    rcva_len = (socklen_t) sizeof(my_address);
    snd_len = sendto(sock, buffer, len, sflags,
                     (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != (ssize_t) len) {
        syserr("partial/failed write");
    }

    // RECEIVING DATAGRAMS
    while(1) {
        (void) memset(buffer, 0, sizeof(buffer));
        flags = 0;
        len = (size_t) sizeof(buffer) - 1;
        rcva_len = (socklen_t) sizeof(srvr_address);
        rcv_len = recvfrom(sock, buffer, len, flags,
                           (struct sockaddr *) &srvr_address, &rcva_len);

        if (rcv_len < 0)
            syserr("read");

        (void) printf("%s", buffer);
    }

    if (close(sock) == -1) { //very rare errors can occur here, but then
        syserr("close"); //it's healthy to do the check
    }

    return 0;
}


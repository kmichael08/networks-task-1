#include <netdb.h>
#include <iostream>
#include "err.h"
#include "utils.h"

#define BUFFER_SIZE 65507
#define DEFAULT_PORT 20160

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

void init_connection(char* host, char* timestamp_str, char* c, int port) {

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

    char space[] = " \0";

    size_t timestamp_len = strnlen(timestamp_str, BUFFER_SIZE);
    strncpy(buffer, timestamp_str, timestamp_len + 1);
    strncpy(buffer + timestamp_len, space, 2);
    strncpy(buffer + timestamp_len + 1, c, 2);
    strncpy(buffer + timestamp_len + 2, space, 2);
    buffer[timestamp_len + 3] = '\0';

    len = timestamp_len + 4;
    sflags = 0;
    rcva_len = (socklen_t) sizeof(my_address);

}

int main(int argc, char* argv[]) {

    if (argc < 4)
        fatal("Usage: %s timestamp c host [port]", argv[0]);

    if (!is_num(argv[1]))
        fatal("timestamp should be a number");

    char *timestamp_str = argv[1];

    if (!is_one_char(argv[2]))
        fatal("Just one character");

    char *c = argv[2];
    char *host = argv[3];

    int port = DEFAULT_PORT;

    if (argc == 5) {
        try {
            port = std::stoi(argv[4]);
        }
        catch (std::invalid_argument) {
            syserr("Port should be a number");
        }
        catch (std::out_of_range) {
            syserr("Port is out of range");
        }
    }


    //**************************************************

    init_connection(host, timestamp_str, c, port);

    snd_len = sendto(sock, buffer, len, sflags,
                     (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != (ssize_t) len) {
        syserr("partial/failed write");
    }

    (void) memset(buffer, 0, sizeof(buffer));
    flags = 0;
    len = (size_t) sizeof(buffer) - 1;
    rcva_len = (socklen_t) sizeof(srvr_address);


    // RECEIVING DATAGRAMS
    while (true) {
        rcv_len = recvfrom(sock, buffer, len, flags,
                           (struct sockaddr *) &srvr_address, &rcva_len);

        if (rcv_len < 0)
            syserr("read");

        (void) printf("%s", buffer);
    }


}


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>

#include "err.h"

#define BUFFER_SIZE 1000
#define secs_1717_year -7983878400
#define secs_4243_year 71697398400

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

int is_one_char(char *c) {
    return strlen(c) == 1;
}

int main(int argc, char* argv[]) {

    if (argc < 4)
        fatal("Usage: %s timestamp c host [port]", argv[0]);

    if (!is_num(argv[1]))
        fatal("timestamp should be a number");

    /* TODO May not work properly!! */
    int64_t timestamp = atoll(argv[1]);

    /* TODO check this values also in C */
    if (timestamp < secs_1717_year || timestamp >= secs_4243_year)
        fatal("Year should be in [1717, 4242]");

    if (!is_one_char(argv[2]))
        fatal("Just one character");

    char c = *argv[2];
    char* host = argv[3];

    int port = 20160;

    if (argc == 5) {
        if (!is_num(argv[4]))
            fatal("Port should be a number");
        port = atoi(argv[4]);
    }

    printf("%lld %c %s %d", timestamp, c, host, port);

    return 0;
}


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

    int port = atoi(argv[1]);
    char* filename = argv[2];

    FILE *fp;
    fp = fopen(filename, "r");

    if (fp == NULL)
        syserr("No such file: %s", filename);

    printf("%d %s\n", port, filename);

    fclose(fp);
    return 0;
}

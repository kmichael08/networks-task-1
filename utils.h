#ifndef SIECI1_UTILS_H
#define SIECI1_UTILS_H
#include <ctype.h>
#include <string.h>

#define secs_1717_year -7983878400
#define secs_4243_year 71697398400

/* Check if string is a number */
int is_num(char* num) {
    int i = 0;
    size_t len = strlen(num);

    if (*num == '-' || *num == '+')
        i = 1;

    for (; i < len; i++) {
        if (!isdigit(*(num+i)))
            return 0;
    }
    return 1;
}

int is_one_char(char *c) {
    return strlen(c) == 1;
}

int valid_timestamp(char* message) {
    int64_t timestamp = atoll(message);
    /* TODO check this values also in C */
    return timestamp > secs_1717_year && timestamp < secs_4243_year;
}
#endif //SIECI1_UTILS_H

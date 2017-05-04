#ifndef SIECI1_UTILS_H
#define SIECI1_UTILS_H
#include <ctype.h>
#include <string.h>


/* Check if string is a number */
bool is_num(char* num) {
    size_t i = 0;
    size_t len = strlen(num);

    for (; i < len; i++) {
        if (!isdigit(*(num+i)))
            return false;
    }
    return true;
}

bool is_one_char(char *c) {
    return strlen(c) == 1;
}

#endif //SIECI1_UTILS_H

#include <stdio.h>
#include "helper.c"

const char *greet() {
    const char *msg = get_message();
    printf("[C] %s\n", msg);
    return msg;
}

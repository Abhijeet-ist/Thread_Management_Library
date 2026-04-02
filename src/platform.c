/**
 * @file platform.c
 * @brief Platform abstraction layer implementation
 */

#include "platform.h"
#include <stdio.h>

const char* platform_get_name(void) {
    return STHREAD_PLATFORM_NAME;
}

int platform_init(void) {
    return 0;
}

void platform_cleanup(void) {
}

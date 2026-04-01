/**
 * @file platform.c
 * @brief Platform abstraction layer implementation
 */

#include "platform.h"
#include <stdio.h>

/**
 * @brief Get platform name string
 */
const char* platform_get_name(void) {
    return STHREAD_PLATFORM_NAME;
}

/**
 * @brief Initialize platform-specific resources
 * 
 * Called once at library initialization
 */
int platform_init(void) {
    // Currently no platform-specific initialization needed
    return 0;
}

/**
 * @brief Cleanup platform-specific resources
 */
void platform_cleanup(void) {
    // Currently no platform-specific cleanup needed
}

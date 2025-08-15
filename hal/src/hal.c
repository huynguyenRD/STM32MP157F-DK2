/**
 * @file hal.c
 * @brief Main Hardware Abstraction Layer implementation for STM32MP157F-DK2
 * 
 * This file provides the main HAL system functions including initialization,
 * deinitialization, and version management for all HAL subsystems.
 * 
 * @author Huy Nguyen
 * @date August 2025
 */

#include "../include/hal.h"
#include <stdio.h>
#include <string.h>

/* HAL Version Information */
#define HAL_VERSION_MAJOR   1
#define HAL_VERSION_MINOR   0
#define HAL_VERSION_PATCH   0
#define HAL_VERSION_STRING  "1.0.0"

/* Global HAL state */
static bool hal_initialized = false;

hal_status_t hal_init(void)
{
    if (hal_initialized) {
        printf("HAL already initialized\n");
        return HAL_OK;
    }

    printf("Initializing STM32MP157F-DK2 HAL v%s...\n", HAL_VERSION_STRING);

    /* Initialize GPIO subsystem */
    hal_status_t gpio_status = hal_gpio_init();
    if (gpio_status != HAL_OK) {
        printf("Error: Failed to initialize GPIO subsystem\n");
        return gpio_status;
    }

    /* Button subsystem is part of GPIO, so no separate init needed */
    printf("Button subsystem initialized (placeholder)\n");

    hal_initialized = true;
    printf("HAL initialization complete\n");
    return HAL_OK;
}

hal_status_t hal_deinit(void)
{
    if (!hal_initialized) {
        printf("HAL not initialized\n");
        return HAL_OK;
    }

    printf("Deinitializing HAL subsystems...\n");

    /* Deinitialize touch subsystem if it was initialized */
    hal_touch_deinit();

    /* Deinitialize LCD subsystem if it was initialized */
    hal_lcd_deinit();

    /* Deinitialize GPIO subsystem */
    hal_gpio_deinit();

    printf("Button subsystem deinitialized (placeholder)\n");

    hal_initialized = false;
    printf("HAL deinitialization complete\n");
    return HAL_OK;
}

const char* hal_get_version(void)
{
    return HAL_VERSION_STRING;
}

bool hal_is_initialized(void)
{
    return hal_initialized;
}

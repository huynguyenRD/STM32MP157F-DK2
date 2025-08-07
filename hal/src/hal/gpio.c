/**
 * @file gpio.c
 * @brief GPIO and LED control implementation for STM32MP157F-DK2
 * 
 * This module provides GPIO control for the 4 user LEDs on the STM32MP157F-DK2
 * board using the Linux sysfs interface.
 * 
 * LED Mapping (STM32MP157F-DK2):
 * - Green LED  : LD4 (PA14)
 * - Orange LED : LD7 (PH7)  
 * - Red LED    : LD6 (PA13)
 * - Blue LED   : LD8 (PD11)
 * 
 * These LEDs are accessible via /sys/class/leds/ interface
 */

#include "../../include/hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* LED sysfs paths for STM32MP157F-DK2 */
static const char* led_paths[HAL_LED_COUNT] = {
    "/sys/class/leds/green:usr0/brightness",   /* HAL_LED_GREEN */
    "/sys/class/leds/red:usr1/brightness",     /* HAL_LED_RED */
    "/sys/class/leds/orange:usr2/brightness",  /* HAL_LED_ORANGE */
    "/sys/class/leds/blue:usr3/brightness"     /* HAL_LED_BLUE */
};

/* Internal state tracking */
static bool gpio_initialized = false;
static hal_led_state_t led_states[HAL_LED_COUNT] = {HAL_LED_OFF, HAL_LED_OFF, HAL_LED_OFF, HAL_LED_OFF};

/* Helper function to write to sysfs file */
static hal_status_t write_sysfs_file(const char* path, const char* value)
{
    int fd;
    ssize_t bytes_written;
    size_t value_len;
    
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Error: Could not open %s for writing: %s\n", path, strerror(errno));
        return HAL_ERROR;
    }
    
    value_len = strlen(value);
    bytes_written = write(fd, value, value_len);
    close(fd);
    
    if (bytes_written != (ssize_t)value_len) {
        printf("Error: Could not write to %s: %s\n", path, strerror(errno));
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* Helper function to read from sysfs file */
static hal_status_t read_sysfs_file(const char* path, char* buffer, size_t buffer_size)
{
    int fd;
    ssize_t bytes_read;
    
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Error: Could not open %s for reading: %s\n", path, strerror(errno));
        return HAL_ERROR;
    }
    
    bytes_read = read(fd, buffer, buffer_size - 1);
    close(fd);
    
    if (bytes_read < 0) {
        printf("Error: Could not read from %s: %s\n", path, strerror(errno));
        return HAL_ERROR;
    }
    
    buffer[bytes_read] = '\0';
    return HAL_OK;
}

hal_status_t hal_led_init(void)
{
    if (gpio_initialized) {
        return HAL_OK;
    }
    
    printf("Initializing GPIO/LED subsystem...\n");

    // Safely check if LED sysfs paths exist
    printf("Checking LED paths...\n");
    
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        printf("Checking LED %d: %s\n", i, led_paths[i]);
        if (led_paths[i] != NULL && access(led_paths[i], F_OK) == 0) {
            printf("  -> Available\n");
        } else {
            printf("  -> Not available\n");
        }
    }

    // Initialize LED states array safely
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        led_states[i] = HAL_LED_OFF;
    }

    gpio_initialized = true;
    printf("GPIO/LED subsystem initialized successfully\n");
    return HAL_OK;
}

hal_status_t hal_led_deinit(void)
{
    if (!gpio_initialized) {
        return HAL_OK;
    }

    printf("Deinitializing GPIO/LED subsystem...\n");
    /* Turn off all LEDs */
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        hal_led_set_state((hal_led_t)i, HAL_LED_OFF);
    }

    gpio_initialized = false;
    printf("GPIO/LED subsystem deinitialized\n");
    return HAL_OK;
}

hal_status_t hal_led_set_state(hal_led_t led, hal_led_state_t state)
{
    hal_status_t status;
    const char* value;
    
    if (!gpio_initialized) {
        return HAL_ERROR;
    }

    if (led >= HAL_LED_COUNT) {
        return HAL_INVALID_PARAM;
    }
    
    if (state != HAL_LED_OFF && state != HAL_LED_ON) {
        return HAL_INVALID_PARAM;
    }

    value = (state == HAL_LED_ON) ? "1" : "0";
    status = write_sysfs_file(led_paths[led], value);
    
    if (status == HAL_OK) {
        led_states[led] = state;
        printf("LED %d set to %s\n", led, (state == HAL_LED_ON) ? "ON" : "OFF");
    }
    
    return status;
}

hal_status_t hal_led_get_state(hal_led_t led, hal_led_state_t *state)
{
    char buffer[16];
    hal_status_t status;
    
    if (!gpio_initialized) {
        return HAL_ERROR;
    }
    
    if (led >= HAL_LED_COUNT || state == NULL) {
        return HAL_INVALID_PARAM;
    }

    status = read_sysfs_file(led_paths[led], buffer, sizeof(buffer));
    if (status != HAL_OK) {
        return status;
    }

    /* Parse the brightness value */
    int brightness = atoi(buffer);
    *state = (brightness > 0) ? HAL_LED_ON : HAL_LED_OFF;
    led_states[led] = *state;
    
    return HAL_OK;
}

hal_status_t hal_led_toggle(hal_led_t led)
{
    hal_led_state_t current_state;
    hal_status_t status;
    
    if (!gpio_initialized) {
        return HAL_ERROR;
    }

    if (led >= HAL_LED_COUNT) {
        return HAL_INVALID_PARAM;
    }
    
    status = hal_led_get_state(led, &current_state);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Toggle the state */
    hal_led_state_t new_state = (current_state == HAL_LED_ON) ? HAL_LED_OFF : HAL_LED_ON;
    return hal_led_set_state(led, new_state);
}

hal_status_t hal_led_set_pattern(uint8_t pattern)
{
    hal_status_t status;
    
    if (!gpio_initialized) {
        return HAL_ERROR;
    }
    
    printf("Setting LED pattern: 0x%02X\n", pattern);
    
    /* Set each LED based on the corresponding bit */
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        hal_led_state_t state = (pattern & (1 << i)) ? HAL_LED_ON : HAL_LED_OFF;
        status = hal_led_set_state((hal_led_t)i, state);
        if (status != HAL_OK) {
            printf("Error setting LED %d\n", i);
            return status;
        }
    }
    
    return HAL_OK;
}

/* Button functions - placeholder implementations */
hal_status_t hal_button_init(void)
{
    printf("Button subsystem initialized (placeholder)\n");
    return HAL_OK;
}

hal_status_t hal_button_deinit(void)
{
    printf("Button subsystem deinitialized (placeholder)\n");
    return HAL_OK;
}

hal_status_t hal_button_get_state(hal_button_t button, hal_button_state_t *state)
{
    if (button >= HAL_BUTTON_COUNT || state == NULL) {
        return HAL_INVALID_PARAM;
    }
    
    /* Placeholder - always return not pressed */
    *state = HAL_BUTTON_RELEASED;
    return HAL_OK;
}

/* HAL system functions */
hal_status_t hal_init(void)
{
    hal_status_t status;
    
    printf("Initializing HAL subsystems...\n");
    
    status = hal_led_init();
    if (status != HAL_OK) {
        printf("Failed to initialize LED subsystem\n");
        return status;
    }
    
    status = hal_button_init();
    if (status != HAL_OK) {
        printf("Failed to initialize button subsystem\n");
        return status;
    }
    
    printf("HAL initialization complete\n");
    return HAL_OK;
}

hal_status_t hal_deinit(void)
{
    printf("Deinitializing HAL subsystems...\n");
    
    hal_led_deinit();
    hal_button_deinit();
    
    printf("HAL deinitialization complete\n");
    return HAL_OK;
}

const char* hal_get_version(void)
{
    return "STM32MP157F-DK2 HAL v1.0.0";
}

bool hal_is_initialized(void)
{
    return gpio_initialized;
}

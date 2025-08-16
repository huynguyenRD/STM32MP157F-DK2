/**
 * @file touch.c
 * @brief Touch interface implementation for STM32MP157F-DK2
 * 
 * This module provides touch input functionality using the FocalTech FT6236
 * capacitive touch controller via Linux input event interface.
 * 
 * Hardware: FocalTech FT6236 connected via I2C
 * Interface: Linux input events (/dev/input/eventX)
 * Resolution: 480x800 pixels
 * Max touch points: 2 simultaneous touches
 * 
 * @author Huy Nguyen  
 * @date August 2025
 */

#define _GNU_SOURCE
#include "../../include/hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define NLONGS(nbits) (((nbits) + (8*sizeof(long)) - 1) / (8 * sizeof(unsigned long)))
unsigned long evbit[NLONGS(EV_MAX+1)] = {0};
unsigned long absbit[NLONGS(KEY_MAX+1)] = {0};

/* Touch device constants */
#define TOUCH_DEVICE_PATH_1     "/dev/input/event0"
#define TOUCH_DEVICE_PATH_2     "/dev/input/event1"
#define TOUCH_DEVICE_PATH_3     "/dev/input/event2"
#define MAX_TOUCH_DEVICES       8
#define POLL_TIMEOUT_MS         50

/* Internal state */
static bool touch_initialized = false;
static int touch_fd = -1;
static hal_touch_data_t current_touch_data;
static char touch_device_path[64] = {0};

/* Function prototypes */
static int find_touch_device(void);
static hal_touch_status_t process_input_event(struct input_event *event);
static void reset_touch_data(void);

hal_touch_status_t hal_touch_init(void)
{
    if (touch_initialized) {
        return HAL_TOUCH_OK;
    }

    printf("Initializing touch subsystem...\n");

    /* Find the touch input device */
    touch_fd = find_touch_device();
    if (touch_fd < 0) {
        printf("Error: No touch device found\n");
        return HAL_TOUCH_ERROR;
    }

    printf("Touch device opened: %s (fd=%d)\n", touch_device_path, touch_fd);

    /* Set non-blocking mode */
    int flags = fcntl(touch_fd, F_GETFL, 0);
    if (flags == -1) {
        printf("Error: Cannot get touch device flags: %s\n", strerror(errno));
        close(touch_fd);
        touch_fd = -1;
        return HAL_TOUCH_ERROR;
    }

    if (fcntl(touch_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        printf("Error: Cannot set non-blocking mode: %s\n", strerror(errno));
        close(touch_fd);
        touch_fd = -1;
        return HAL_TOUCH_ERROR;
    }

    /* Initialize touch data */
    reset_touch_data();

    touch_initialized = true;
    printf("Touch subsystem initialized successfully\n");
    return HAL_TOUCH_OK;
}

hal_touch_status_t hal_touch_deinit(void)
{
    if (!touch_initialized) {
        return HAL_TOUCH_OK;
    }

    printf("Deinitializing touch subsystem...\n");

    if (touch_fd >= 0) {
        close(touch_fd);
        touch_fd = -1;
    }

    reset_touch_data();
    touch_initialized = false;
    
    printf("Touch subsystem deinitialized\n");
    return HAL_TOUCH_OK;
}

hal_touch_status_t hal_touch_read(hal_touch_data_t *data)
{
    if (!touch_initialized || touch_fd < 0) {
        return HAL_TOUCH_NOT_INITIALIZED;
    }

    if (data == NULL) {
        return HAL_TOUCH_INVALID_PARAM;
    }

    struct input_event events[64];
    ssize_t bytes_read;
    bool data_updated = false;

    /* Read all available events */
    while ((bytes_read = read(touch_fd, events, sizeof(events))) > 0) {
        int num_events = bytes_read / sizeof(struct input_event);
        
        for (int i = 0; i < num_events; i++) {
            if (process_input_event(&events[i]) == HAL_TOUCH_OK) {
                data_updated = true;
            }
        }
    }

    /* Copy current touch data */
    memcpy(data, &current_touch_data, sizeof(hal_touch_data_t));

    return data_updated ? HAL_TOUCH_OK : HAL_TOUCH_NO_DATA;
}

bool hal_touch_is_touched(void)
{
    if (!touch_initialized) {
        return false;
    }

    hal_touch_data_t data;
    hal_touch_read(&data);

    return (data.count > 0);
}

hal_touch_status_t hal_touch_get_point(uint16_t *x, uint16_t *y)
{
    if (!touch_initialized) {
        return HAL_TOUCH_NOT_INITIALIZED;
    }

    if (x == NULL || y == NULL) {
        return HAL_TOUCH_INVALID_PARAM;
    }

    hal_touch_data_t data;
    hal_touch_status_t status = hal_touch_read(&data);

    if (status == HAL_TOUCH_OK && data.count > 0 && data.points[0].valid) {
        *x = data.points[0].x;
        *y = data.points[0].y;
        return HAL_TOUCH_OK;
    }

    return HAL_TOUCH_NO_DATA;
}

hal_touch_status_t hal_touch_calibrate(void)
{
    /* FT6236 typically doesn't need software calibration */
    printf("Touch calibration not required for FT6236\n");
    return HAL_TOUCH_OK;
}

/* Internal helper functions */

static int find_touch_device(void)
{
    char device_paths[][32] = {
        "/dev/input/event0",
        "/dev/input/event1", 
        "/dev/input/event2",
        "/dev/input/event3",
        "/dev/input/event4",
        "/dev/input/event5",
        "/dev/input/event6",
        "/dev/input/event7"
    };

    for (int i = 0; i < MAX_TOUCH_DEVICES; i++) {
        int fd = open(device_paths[i], O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            /* Check if this device supports touch events */
            unsigned long evbit = 0;
            if (ioctl(fd, EVIOCGBIT(0, EV_MAX), &evbit) >= 0) {
                if (evbit & (1 << EV_ABS)) {
                    /* Check for absolute positioning (touch/tablet) */
                    unsigned long absbit = 0;
                    if (ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), &absbit) >= 0) {
                        if ((absbit & (1 << ABS_X)) && (absbit & (1 << ABS_Y))) {
                            /* This looks like a touch device */
                            strcpy(touch_device_path, device_paths[i]);
                            printf("Found potential touch device: %s\n", device_paths[i]);
                            
                            /* Get device name for verification */
                            char name[256] = "Unknown";
                            if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
                                printf("Device name: %s\n", name);
                                
                                /* Look for FT6236 or touch-related keywords */
                                if (strstr(name, "ft6236") || strstr(name, "FT6236") ||
                                    strstr(name, "touch") || strstr(name, "Touch") ||
                                    strstr(name, "touchscreen") || strstr(name, "Touchscreen")) {
                                    printf("Touch device confirmed: %s\n", name);
                                    return fd;
                                }
                            }
                            
                            /* Even if name doesn't match, try this device */
                            return fd;
                        }
                    }
                }
            }
            close(fd);
        }
    }

    printf("No touch device found in /dev/input/event*\n");
    return -1;
}

static hal_touch_status_t process_input_event(struct input_event *event)
{
    static int current_slot = 0;
    static int tracking_id = -1;
    bool event_processed = false;

    switch (event->type) {
        case EV_ABS:
            switch (event->code) {
                case ABS_MT_SLOT:
                    current_slot = event->value;
                    if (current_slot >= HAL_TOUCH_MAX_POINTS) {
                        current_slot = 0;
                    }
                    break;

                case ABS_MT_TRACKING_ID:
                    tracking_id = event->value;
                    if (tracking_id == -1) {
                        /* Touch release */
                        if (current_slot < HAL_TOUCH_MAX_POINTS) {
                            current_touch_data.points[current_slot].valid = false;
                            current_touch_data.points[current_slot].event = HAL_TOUCH_EVENT_RELEASE;
                            event_processed = true;
                        }
                    } else {
                        /* Touch press */
                        if (current_slot < HAL_TOUCH_MAX_POINTS) {
                            current_touch_data.points[current_slot].valid = true;
                            current_touch_data.points[current_slot].id = current_slot;
                            current_touch_data.points[current_slot].event = HAL_TOUCH_EVENT_PRESS;
                            event_processed = true;
                        }
                    }
                    break;

                case ABS_MT_POSITION_X:
                case ABS_X:
                    if (current_slot < HAL_TOUCH_MAX_POINTS) {
                        /* Scale X coordinate to touch panel resolution */
                        current_touch_data.points[current_slot].x = 
                            (event->value * HAL_TOUCH_WIDTH) / 4096;  /* Assuming 12-bit resolution */
                        if (current_touch_data.points[current_slot].valid) {
                            current_touch_data.points[current_slot].event = HAL_TOUCH_EVENT_MOVE;
                            event_processed = true;
                        }
                    }
                    break;

                case ABS_MT_POSITION_Y:
                case ABS_Y:
                    if (current_slot < HAL_TOUCH_MAX_POINTS) {
                        /* Scale Y coordinate to touch panel resolution */
                        current_touch_data.points[current_slot].y = 
                            (event->value * HAL_TOUCH_HEIGHT) / 4096;  /* Assuming 12-bit resolution */
                        if (current_touch_data.points[current_slot].valid) {
                            current_touch_data.points[current_slot].event = HAL_TOUCH_EVENT_MOVE;
                            event_processed = true;
                        }
                    }
                    break;

                case ABS_MT_PRESSURE:
                case ABS_PRESSURE:
                    if (current_slot < HAL_TOUCH_MAX_POINTS) {
                        current_touch_data.points[current_slot].pressure = 
                            (event->value > 255) ? 255 : event->value;
                    }
                    break;

                default:
                    break;
            }
            break;

        case EV_SYN:
            if (event->code == SYN_REPORT) {
                /* End of touch report - update touch count and timestamp */
                current_touch_data.count = 0;
                for (int i = 0; i < HAL_TOUCH_MAX_POINTS; i++) {
                    if (current_touch_data.points[i].valid) {
                        current_touch_data.count++;
                    }
                }
                current_touch_data.timestamp = event->time.tv_sec * 1000 + event->time.tv_usec / 1000;
                event_processed = true;
            }
            break;

        default:
            break;
    }

    return event_processed ? HAL_TOUCH_OK : HAL_TOUCH_NO_DATA;
}

static void reset_touch_data(void)
{
    memset(&current_touch_data, 0, sizeof(hal_touch_data_t));
    
    for (int i = 0; i < HAL_TOUCH_MAX_POINTS; i++) {
        current_touch_data.points[i].valid = false;
        current_touch_data.points[i].event = HAL_TOUCH_EVENT_NONE;
    }
    
    current_touch_data.count = 0;
    current_touch_data.timestamp = 0;
}

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer for STM32MP157F-DK2
 * 
 * This file provides a unified interface to control the built-in hardware
 * features of the STM32MP157F-DK2 development board.
 * 
 * Currently supports:
 * - 4 LED control (Green LD5, Red LD6, Orange LD7, Blue LD8)
 * - 2 Button input (USER1, USER2)
 * 
 * @author Huy Nguyen
 * @date August 2025
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

/* HAL Return Codes */
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = -1,
    HAL_INVALID_PARAM = -2,
    HAL_NOT_SUPPORTED = -3,
    HAL_TIMEOUT = -4,
    HAL_BUSY = -5,
    HAL_NOT_INITIALIZED = -6
} hal_status_t;

/* LED Definitions - Based on actual STM32MP157F-DK2 configuration */
typedef enum {
    HAL_LED_GREEN = 0,      /* Green LD5 (PA14) */
    HAL_LED_RED = 1,        /* Red LD6 (PA13) */
    HAL_LED_ORANGE = 2,     /* Orange LD7 (PH7) */
    HAL_LED_BLUE = 3,       /* Blue LD8 (PD11) */
    HAL_LED_COUNT = 4       /* Total number of available LEDs */
} hal_led_t;

typedef enum {
    HAL_LED_OFF = 0,
    HAL_LED_ON = 1
} hal_led_state_t;

/* Button Definitions - Based on actual STM32MP157F-DK2 configuration */
typedef enum {
    HAL_BUTTON_USER1 = 0,   /* USER1 button (with Green LED LD5) */
    HAL_BUTTON_USER2 = 1,   /* USER2 button (with Red LED LD6) */
    HAL_BUTTON_COUNT = 2    /* Total number of available buttons */
} hal_button_t;

typedef enum {
    HAL_BUTTON_RELEASED = 0,
    HAL_BUTTON_PRESSED = 1
} hal_button_state_t;

/* LCD Definitions - LCD Display Specifications for STM32MP157F-DK2 */
#define LCD_WIDTH           480
#define LCD_HEIGHT          800
#define LCD_BPP             32      /* 32 bits per pixel (ARGB8888) */
#define LCD_BUFFER_SIZE     (LCD_WIDTH * LCD_HEIGHT * (LCD_BPP / 8))

/* Color definitions for ARGB8888 format (32-bit) - Fixed format */
#define LCD_COLOR_BLACK     0xFF000000
#define LCD_COLOR_WHITE     0xFFFFFFFF
#define LCD_COLOR_RED       0xFFFF0000
#define LCD_COLOR_GREEN     0xFF00FF00
#define LCD_COLOR_BLUE      0xFF0000FF
#define LCD_COLOR_YELLOW    0xFFFFFF00
#define LCD_COLOR_CYAN      0xFF00FFFF
#define LCD_COLOR_MAGENTA   0xFFFF00FF

typedef enum {
    HAL_LCD_OK = 0,
    HAL_LCD_ERROR,
    HAL_LCD_INVALID_PARAM,
    HAL_LCD_NOT_INITIALIZED
} hal_lcd_status_t;

typedef struct {
    uint16_t x;
    uint16_t y;
} hal_lcd_point_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} hal_lcd_rect_t;

/*=============================================================================
 * LED Control Functions
 *============================================================================*/

/**
 * @brief Initialize the LED subsystem
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_init(void);

/**
 * @brief Deinitialize the LED subsystem
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_deinit(void);

/**
 * @brief Set LED state
 * @param led LED identifier
 * @param state LED state (ON/OFF)
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_set_state(hal_led_t led, hal_led_state_t state);

/**
 * @brief Get LED state
 * @param led LED identifier
 * @param state Pointer to store LED state
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_get_state(hal_led_t led, hal_led_state_t *state);

/**
 * @brief Toggle LED state
 * @param led LED identifier
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_toggle(hal_led_t led);

/**
 * @brief Set LED pattern (4-bit pattern for all LEDs)
 * @param pattern Bit pattern where each bit represents an LED state
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_led_set_pattern(uint8_t pattern);

/*=============================================================================
 * Button Control Functions
 *============================================================================*/

/**
 * @brief Initialize the button subsystem
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_button_init(void);

/**
 * @brief Deinitialize the button subsystem
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_button_deinit(void);

/**
 * @brief Get button state
 * @param button Button identifier
 * @param state Pointer to store button state
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_button_get_state(hal_button_t button, hal_button_state_t *state);

/*=============================================================================
 * LCD Control Functions
 *============================================================================*/

/**
 * @brief Initialize the LCD subsystem
 * @return HAL_LCD_OK on success, error code otherwise
 */
hal_lcd_status_t hal_lcd_init(void);

/**
 * @brief Deinitialize the LCD subsystem
 * @return HAL_LCD_OK on success, error code otherwise
 */
hal_lcd_status_t hal_lcd_deinit(void);

/**
 * @brief Clear the LCD with specified color
 * @param color 32-bit ARGB color value
 * @return HAL_LCD_OK on success, error code otherwise
 */
hal_lcd_status_t hal_lcd_clear(uint32_t color);

/**
 * @brief Set a pixel on the LCD
 * @param x X coordinate
 * @param y Y coordinate  
 * @param color 32-bit ARGB color value
 * @return HAL_LCD_OK on success, error code otherwise
 */
hal_lcd_status_t hal_lcd_set_pixel(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief Draw a rectangle on the LCD
 * @param rect Rectangle coordinates and size
 * @param color 32-bit ARGB color value
 * @param filled true for filled rectangle, false for outline
 * @return HAL_LCD_OK on success, error code otherwise
 */
hal_lcd_status_t hal_lcd_draw_rectangle(hal_lcd_rect_t rect, uint32_t color, bool filled);

/*=============================================================================
 * HAL System Functions
 *============================================================================*/

/**
 * @brief Initialize all HAL subsystems
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_init(void);

/**
 * @brief Deinitialize all HAL subsystems
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_deinit(void);

/**
 * @brief Get HAL version string
 * @return Version string
 */
const char* hal_get_version(void);

/**
 * @brief Check if HAL is initialized
 * @return true if initialized, false otherwise
 */
bool hal_is_initialized(void);

#endif /* HAL_H */

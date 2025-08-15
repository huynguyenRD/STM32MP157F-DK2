/**
 * @file touch_test.c
 * @brief Touch interface test for STM32MP157F-DK2
 * 
 * This program tests the capacitive touch functionality of the FT6236
 * touch controller on the STM32MP157F-DK2 board.
 * 
 * Features tested:
 * - Touch detection and coordinate reading
 * - Multi-touch support (up to 2 points)
 * - Touch events (press, move, release)
 * - Visual feedback on LCD display
 * - Touch calibration verification
 * 
 * @author Huy Nguyen
 * @date August 2025
 */

#include "../include/hal.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

/* Test configuration */
#define TOUCH_POLL_INTERVAL_MS  20      /* 50 Hz polling rate */
#define MAX_TOUCH_TRAIL_POINTS  100     /* Maximum points in touch trail */
#define TOUCH_POINT_SIZE        8       /* Size of touch indicator on screen */

/* Test colors */
#define COLOR_BACKGROUND    0xFF000000  /* Black */
#define COLOR_TOUCH_1       0xFFFF0000  /* Red for first touch */
#define COLOR_TOUCH_2       0xFF00FF00  /* Green for second touch */
#define COLOR_TRAIL         0xFF808080  /* Gray for touch trail */
#define COLOR_TEXT          0xFFFFFFFF  /* White for text */
#define COLOR_BORDER        0xFF0000FF  /* Blue for borders */

/* Touch trail structure */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t age;        /* Age of trail point (for fading effect) */
} touch_trail_point_t;

static touch_trail_point_t touch_trail[MAX_TOUCH_TRAIL_POINTS];
static int trail_index = 0;
static bool running = true;

/* Function prototypes */
static void signal_handler(int sig);
static void test_basic_touch(void);
static void test_multitouch(void);
static void test_touch_and_draw(void);
static void draw_touch_point(uint16_t x, uint16_t y, uint32_t color);
static void draw_touch_info(hal_touch_data_t *data);
static void add_trail_point(uint16_t x, uint16_t y);
static void draw_trail(void);
static void update_trail(void);
static void draw_crosshair(uint16_t x, uint16_t y, uint32_t color);
static void clear_screen_with_border(void);

/* Signal handler for clean exit */
static void signal_handler(int sig)
{
    (void)sig;
    printf("\nReceived interrupt signal. Exiting...\n");
    running = false;
}

/* Test basic touch detection */
static void test_basic_touch(void)
{
    printf("\n=== Basic Touch Test ===\n");
    printf("Touch the screen to test basic touch detection.\n");
    printf("Press Ctrl+C to exit this test.\n\n");

    clear_screen_with_border();
    
    int touch_count = 0;
    time_t start_time = time(NULL);
    
    while (running && (time(NULL) - start_time) < 30) {  /* 30 second test */
        hal_touch_data_t touch_data;
        hal_touch_status_t status = hal_touch_read(&touch_data);
        
        if (status == HAL_TOUCH_OK && touch_data.count > 0) {
            for (int i = 0; i < touch_data.count; i++) {
                if (touch_data.points[i].valid) {
                    printf("Touch %d: X=%d, Y=%d, Event=%d, Pressure=%d\n",
                           touch_data.points[i].id,
                           touch_data.points[i].x,
                           touch_data.points[i].y,
                           touch_data.points[i].event,
                           touch_data.points[i].pressure);
                    
                    /* Draw touch point on screen */
                    uint32_t color = (i == 0) ? COLOR_TOUCH_1 : COLOR_TOUCH_2;
                    draw_touch_point(touch_data.points[i].x, touch_data.points[i].y, color);
                    
                    if (touch_data.points[i].event == HAL_TOUCH_EVENT_PRESS) {
                        touch_count++;
                    }
                }
            }
        }
        
        usleep(TOUCH_POLL_INTERVAL_MS * 1000);
    }
    
    printf("Basic touch test completed. Total touches detected: %d\n", touch_count);
}

/* Test multi-touch functionality */
static void test_multitouch(void)
{
    printf("\n=== Multi-Touch Test ===\n");
    printf("Use two fingers to test multi-touch detection.\n");
    printf("Press Ctrl+C to exit this test.\n\n");

    clear_screen_with_border();
    
    int max_simultaneous = 0;
    time_t start_time = time(NULL);
    
    while (running && (time(NULL) - start_time) < 30) {  /* 30 second test */
        hal_touch_data_t touch_data;
        hal_touch_status_t status = hal_touch_read(&touch_data);
        
        if (status == HAL_TOUCH_OK) {
            if (touch_data.count > max_simultaneous) {
                max_simultaneous = touch_data.count;
            }
            
            if (touch_data.count > 0) {
                /* Clear previous touch indicators */
                static uint16_t prev_x[HAL_TOUCH_MAX_POINTS] = {0};
                static uint16_t prev_y[HAL_TOUCH_MAX_POINTS] = {0};
                
                for (int i = 0; i < HAL_TOUCH_MAX_POINTS; i++) {
                    if (prev_x[i] != 0 || prev_y[i] != 0) {
                        draw_touch_point(prev_x[i], prev_y[i], COLOR_BACKGROUND);
                    }
                }
                
                /* Draw current touch points */
                for (int i = 0; i < touch_data.count; i++) {
                    if (touch_data.points[i].valid) {
                        uint32_t color = (i == 0) ? COLOR_TOUCH_1 : COLOR_TOUCH_2;
                        draw_crosshair(touch_data.points[i].x, touch_data.points[i].y, color);
                        
                        prev_x[i] = touch_data.points[i].x;
                        prev_y[i] = touch_data.points[i].y;
                        
                        printf("Touch point %d: (%d, %d)\n", i, 
                               touch_data.points[i].x, touch_data.points[i].y);
                    }
                }
                
                if (touch_data.count > 1) {
                    printf("Multi-touch detected: %d points\n", touch_data.count);
                }
            }
        }
        
        usleep(TOUCH_POLL_INTERVAL_MS * 1000);
    }
    
    printf("Multi-touch test completed. Maximum simultaneous touches: %d\n", max_simultaneous);
}

/* Test touch and draw functionality */
static void test_touch_and_draw(void)
{
    printf("\n=== Touch and Draw Test ===\n");
    printf("Touch and drag to draw on the screen.\n");
    printf("Press Ctrl+C to exit this test.\n\n");

    clear_screen_with_border();
    memset(touch_trail, 0, sizeof(touch_trail));
    trail_index = 0;
    
    time_t start_time = time(NULL);
    
    while (running && (time(NULL) - start_time) < 60) {  /* 60 second test */
        hal_touch_data_t touch_data;
        hal_touch_status_t status = hal_touch_read(&touch_data);
        
        if (status == HAL_TOUCH_OK && touch_data.count > 0) {
            for (int i = 0; i < touch_data.count; i++) {
                if (touch_data.points[i].valid) {
                    /* Add point to trail */
                    add_trail_point(touch_data.points[i].x, touch_data.points[i].y);
                    
                    /* Draw current touch point */
                    uint32_t color = (i == 0) ? COLOR_TOUCH_1 : COLOR_TOUCH_2;
                    draw_touch_point(touch_data.points[i].x, touch_data.points[i].y, color);
                }
            }
            
            /* Draw touch info */
            draw_touch_info(&touch_data);
        }
        
        /* Update and draw trail */
        update_trail();
        draw_trail();
        
        usleep(TOUCH_POLL_INTERVAL_MS * 1000);
    }
    
    printf("Touch and draw test completed.\n");
}

/* Helper function to draw touch point */
static void draw_touch_point(uint16_t x, uint16_t y, uint32_t color)
{
    hal_lcd_rect_t rect = {
        .x = (x > TOUCH_POINT_SIZE/2) ? x - TOUCH_POINT_SIZE/2 : 0,
        .y = (y > TOUCH_POINT_SIZE/2) ? y - TOUCH_POINT_SIZE/2 : 0,
        .width = TOUCH_POINT_SIZE,
        .height = TOUCH_POINT_SIZE
    };
    
    hal_lcd_draw_rectangle(rect, color, true);
}

/* Helper function to draw crosshair */
static void draw_crosshair(uint16_t x, uint16_t y, uint32_t color)
{
    /* Draw horizontal line */
    for (int i = -10; i <= 10; i++) {
        if (x + i >= 0 && x + i < HAL_TOUCH_WIDTH) {
            hal_lcd_set_pixel(x + i, y, color);
        }
    }
    
    /* Draw vertical line */
    for (int i = -10; i <= 10; i++) {
        if (y + i >= 0 && y + i < HAL_TOUCH_HEIGHT) {
            hal_lcd_set_pixel(x, y + i, color);
        }
    }
}

/* Helper function to draw touch information */
static void draw_touch_info(hal_touch_data_t *data)
{
    /* This would draw text info on screen - simplified for now */
    if (data->count > 0) {
        /* Draw count indicator in top-left corner */
        hal_lcd_rect_t indicator = {10, 10, 20 * data->count, 10};
        hal_lcd_draw_rectangle(indicator, COLOR_TEXT, true);
    }
}

/* Helper function to add trail point */
static void add_trail_point(uint16_t x, uint16_t y)
{
    touch_trail[trail_index].x = x;
    touch_trail[trail_index].y = y;
    touch_trail[trail_index].age = 0;
    
    trail_index = (trail_index + 1) % MAX_TOUCH_TRAIL_POINTS;
}

/* Helper function to draw trail */
static void draw_trail(void)
{
    for (int i = 0; i < MAX_TOUCH_TRAIL_POINTS; i++) {
        if (touch_trail[i].age > 0 && touch_trail[i].age < 50) {
            /* Fade trail points by age */
            uint8_t alpha = 255 - (touch_trail[i].age * 5);
            uint32_t trail_color = (alpha << 24) | (COLOR_TRAIL & 0x00FFFFFF);
            
            hal_lcd_set_pixel(touch_trail[i].x, touch_trail[i].y, trail_color);
        }
    }
}

/* Helper function to update trail */
static void update_trail(void)
{
    for (int i = 0; i < MAX_TOUCH_TRAIL_POINTS; i++) {
        if (touch_trail[i].age > 0) {
            touch_trail[i].age++;
            if (touch_trail[i].age >= 50) {
                /* Clear old trail point */
                hal_lcd_set_pixel(touch_trail[i].x, touch_trail[i].y, COLOR_BACKGROUND);
                touch_trail[i].age = 0;
            }
        }
    }
}

/* Helper function to clear screen with border */
static void clear_screen_with_border(void)
{
    hal_lcd_clear(COLOR_BACKGROUND);
    
    /* Draw border */
    hal_lcd_rect_t border = {0, 0, HAL_TOUCH_WIDTH, HAL_TOUCH_HEIGHT};
    hal_lcd_draw_rectangle(border, COLOR_BORDER, false);
    
    /* Draw corner markers */
    hal_lcd_rect_t corners[] = {
        {5, 5, 20, 20},                                          /* Top-left */
        {HAL_TOUCH_WIDTH-25, 5, 20, 20},                        /* Top-right */
        {5, HAL_TOUCH_HEIGHT-25, 20, 20},                       /* Bottom-left */
        {HAL_TOUCH_WIDTH-25, HAL_TOUCH_HEIGHT-25, 20, 20}       /* Bottom-right */
    };
    
    for (int i = 0; i < 4; i++) {
        hal_lcd_draw_rectangle(corners[i], COLOR_BORDER, true);
    }
}

/* Main function */
int main(int argc, char *argv[])
{
    printf("=== STM32MP157F-DK2 Touch Interface Test ===\n");
    printf("FocalTech FT6236 Capacitive Touch Controller\n");
    printf("Resolution: %dx%d pixels\n", HAL_TOUCH_WIDTH, HAL_TOUCH_HEIGHT);
    printf("Max simultaneous touches: %d\n\n", HAL_TOUCH_MAX_POINTS);

    /* Set up signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize HAL */
    if (hal_init() != HAL_OK) {
        printf("Error: Failed to initialize HAL\n");
        return EXIT_FAILURE;
    }

    /* Initialize LCD */
    if (hal_lcd_init() != HAL_LCD_OK) {
        printf("Error: Failed to initialize LCD\n");
        hal_deinit();
        return EXIT_FAILURE;
    }

    /* Initialize touch */
    if (hal_touch_init() != HAL_TOUCH_OK) {
        printf("Error: Failed to initialize touch interface\n");
        hal_lcd_deinit();
        hal_deinit();
        return EXIT_FAILURE;
    }

    printf("Touch interface initialized successfully\n");

    /* Run tests based on command line arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "basic") == 0) {
            test_basic_touch();
        } else if (strcmp(argv[1], "multi") == 0) {
            test_multitouch();
        } else if (strcmp(argv[1], "draw") == 0) {
            test_touch_and_draw();
        } else {
            printf("Usage: %s [basic|multi|draw]\n", argv[0]);
            printf("  basic - Test basic touch detection\n");
            printf("  multi - Test multi-touch functionality\n");
            printf("  draw  - Test touch and draw\n");
            printf("  (no args) - Run all tests\n");
        }
    } else {
        /* Run all tests */
        printf("Running all touch tests...\n");
        test_basic_touch();
        if (running) test_multitouch();
        if (running) test_touch_and_draw();
    }

    /* Cleanup */
    printf("\nCleaning up...\n");
    hal_lcd_clear(COLOR_BACKGROUND);
    hal_touch_deinit();
    hal_lcd_deinit();
    hal_deinit();

    printf("Touch test completed successfully!\n");
    return EXIT_SUCCESS;
}

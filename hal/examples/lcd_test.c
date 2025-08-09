#include "../include/hal.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

/* Test Colors */
#define TEST_WHITE      0xFFFFFFFF
#define TEST_BLACK      0xFF000000
#define TEST_RED        0xFFFF0000
#define TEST_GREEN      0xFF00FF00
#define TEST_BLUE       0xFF0000FF
#define TEST_YELLOW     0xFFFFFF00
#define TEST_MAGENTA    0xFFFF00FF
#define TEST_CYAN       0xFF00FFFF
#define TEST_ORANGE     0xFFFF8000
#define TEST_PURPLE     0xFF8000FF
#define TEST_GRAY       0xFF808080
#define TEST_DARK_GRAY  0xFF404040

/* Function Prototypes */
static void test_basic_colors(void);
static void test_pixel_drawing(void);
static void test_rectangles(void);
static void test_gradients(void);
static void test_patterns(void);
static void test_performance(void);
static void draw_gradient_horizontal(uint32_t color1, uint32_t color2);
static void draw_gradient_vertical(uint32_t color1, uint32_t color2);
static void draw_checkerboard(uint32_t color1, uint32_t color2, int size);
static void draw_color_bars(void);
static void draw_test_pattern(void);
static uint32_t blend_colors(uint32_t color1, uint32_t color2, float ratio);

/* Test basic color fills */
static void test_basic_colors(void)
{
    printf("\n=== Testing Basic Colors ===\n");
    
    const struct {
        uint32_t color;
        const char* name;
    } colors[] = {
        {TEST_BLACK,     "Black"},
        {TEST_WHITE,     "White"},
        {TEST_RED,       "Red"},
        {TEST_GREEN,     "Green"},
        {TEST_BLUE,      "Blue"},
        {TEST_YELLOW,    "Yellow"},
        {TEST_MAGENTA,   "Magenta"},
        {TEST_CYAN,      "Cyan"},
        {TEST_ORANGE,    "Orange"},
        {TEST_PURPLE,    "Purple"},
        {TEST_GRAY,      "Gray"},
        {TEST_DARK_GRAY, "Dark Gray"}
    };
    
    for (int i = 0; i < sizeof(colors)/sizeof(colors[0]); i++) {
        printf("Displaying %s (0x%08X)...\n", colors[i].name, colors[i].color);
        hal_lcd_clear(colors[i].color);
        sleep(1);
    }
}

/* Test individual pixel drawing */
static void test_pixel_drawing(void)
{
    printf("\n=== Testing Pixel Drawing ===\n");
    
    hal_lcd_clear(TEST_BLACK);
    
    printf("Drawing single pixels...\n");
    
    /* Draw corners */
    hal_lcd_set_pixel(0, 0, TEST_RED);                    // Top-left
    hal_lcd_set_pixel(LCD_WIDTH-1, 0, TEST_GREEN);        // Top-right
    hal_lcd_set_pixel(0, LCD_HEIGHT-1, TEST_BLUE);        // Bottom-left
    hal_lcd_set_pixel(LCD_WIDTH-1, LCD_HEIGHT-1, TEST_WHITE); // Bottom-right
    
    /* Draw center cross */
    uint16_t center_x = LCD_WIDTH / 2;
    uint16_t center_y = LCD_HEIGHT / 2;
    
    for (int i = -20; i <= 20; i++) {
        hal_lcd_set_pixel(center_x + i, center_y, TEST_YELLOW);     // Horizontal line
        hal_lcd_set_pixel(center_x, center_y + i, TEST_YELLOW);     // Vertical line
    }
    
    /* Draw diagonal lines */
    for (int i = 0; i < 100; i++) {
        hal_lcd_set_pixel(i, i, TEST_CYAN);                        // Top-left diagonal
        hal_lcd_set_pixel(LCD_WIDTH - 1 - i, i, TEST_MAGENTA);     // Top-right diagonal
    }
    
    printf("Pixel test complete. Press any key to continue...\n");
    sleep(3);
}

/* Test rectangle drawing */
static void test_rectangles(void)
{
    printf("\n=== Testing Rectangle Drawing ===\n");
    
    hal_lcd_clear(TEST_BLACK);
    
    /* Test filled rectangles */
    printf("Drawing filled rectangles...\n");
    
    hal_lcd_rect_t rect1 = {50, 50, 100, 80};
    hal_lcd_draw_rectangle(rect1, TEST_RED, true);
    
    hal_lcd_rect_t rect2 = {200, 100, 120, 100};
    hal_lcd_draw_rectangle(rect2, TEST_GREEN, true);
    
    hal_lcd_rect_t rect3 = {100, 250, 80, 60};
    hal_lcd_draw_rectangle(rect3, TEST_BLUE, true);
    
    sleep(2);
    
    /* Test outlined rectangles */
    printf("Drawing outlined rectangles...\n");
    
    hal_lcd_rect_t outline1 = {40, 40, 120, 100};
    hal_lcd_draw_rectangle(outline1, TEST_YELLOW, false);
    
    hal_lcd_rect_t outline2 = {190, 90, 140, 120};
    hal_lcd_draw_rectangle(outline2, TEST_CYAN, false);
    
    hal_lcd_rect_t outline3 = {90, 240, 100, 80};
    hal_lcd_draw_rectangle(outline3, TEST_MAGENTA, false);
    
    /* Draw nested rectangles */
    printf("Drawing nested rectangles...\n");
    for (int i = 0; i < 10; i++) {
        hal_lcd_rect_t nested = {
            (uint16_t)(320 + i * 5),
            (uint16_t)(400 + i * 5),
            (uint16_t)(100 - i * 10),
            (uint16_t)(100 - i * 10)
        };
        uint32_t color = TEST_WHITE - (i * 0x0F0F0F00);
        hal_lcd_draw_rectangle(nested, color, false);
    }
    
    sleep(3);
}

/* Test gradient effects */
static void test_gradients(void)
{
    printf("\n=== Testing Gradients ===\n");
    
    printf("Horizontal red to blue gradient...\n");
    draw_gradient_horizontal(TEST_RED, TEST_BLUE);
    sleep(2);
    
    printf("Vertical green to yellow gradient...\n");
    draw_gradient_vertical(TEST_GREEN, TEST_YELLOW);
    sleep(2);
    
    printf("Horizontal black to white gradient...\n");
    draw_gradient_horizontal(TEST_BLACK, TEST_WHITE);
    sleep(2);
}

/* Test patterns */
static void test_patterns(void)
{
    printf("\n=== Testing Patterns ===\n");
    
    printf("Drawing checkerboard pattern...\n");
    draw_checkerboard(TEST_RED, TEST_BLUE, 20);
    sleep(2);
    
    printf("Drawing small checkerboard...\n");
    draw_checkerboard(TEST_GREEN, TEST_MAGENTA, 10);
    sleep(2);
    
    printf("Drawing color bars...\n");
    draw_color_bars();
    sleep(2);
    
    printf("Drawing test pattern...\n");
    draw_test_pattern();
    sleep(3);
}

/* Test performance */
static void test_performance(void)
{
    printf("\n=== Testing Performance ===\n");
    
    clock_t start, end;
    double cpu_time_used;
    
    /* Test clear performance */
    printf("Testing clear performance...\n");
    start = clock();
    for (int i = 0; i < 100; i++) {
        hal_lcd_clear(TEST_RED);
        hal_lcd_clear(TEST_BLUE);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("200 clear operations took %f seconds\n", cpu_time_used);
    
    /* Test pixel drawing performance */
    printf("Testing pixel drawing performance...\n");
    hal_lcd_clear(TEST_BLACK);
    start = clock();
    for (int y = 0; y < LCD_HEIGHT; y += 4) {
        for (int x = 0; x < LCD_WIDTH; x += 4) {
            hal_lcd_set_pixel(x, y, TEST_WHITE);
        }
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Pixel grid drawing took %f seconds\n", cpu_time_used);
    
    sleep(2);
}

/* Helper function to draw horizontal gradient */
static void draw_gradient_horizontal(uint32_t color1, uint32_t color2)
{
    for (uint16_t x = 0; x < LCD_WIDTH; x++) {
        float ratio = (float)x / (LCD_WIDTH - 1);
        uint32_t color = blend_colors(color1, color2, ratio);
        
        for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
            hal_lcd_set_pixel(x, y, color);
        }
    }
}

/* Helper function to draw vertical gradient */
static void draw_gradient_vertical(uint32_t color1, uint32_t color2)
{
    for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
        float ratio = (float)y / (LCD_HEIGHT - 1);
        uint32_t color = blend_colors(color1, color2, ratio);
        
        for (uint16_t x = 0; x < LCD_WIDTH; x++) {
            hal_lcd_set_pixel(x, y, color);
        }
    }
}

/* Helper function to draw checkerboard */
static void draw_checkerboard(uint32_t color1, uint32_t color2, int size)
{
    hal_lcd_clear(TEST_BLACK);
    
    for (uint16_t y = 0; y < LCD_HEIGHT; y += size) {
        for (uint16_t x = 0; x < LCD_WIDTH; x += size) {
            hal_lcd_rect_t rect = {x, y, (uint16_t)size, (uint16_t)size};
            
            int checker_x = x / size;
            int checker_y = y / size;
            uint32_t color = ((checker_x + checker_y) % 2) ? color1 : color2;
            
            hal_lcd_draw_rectangle(rect, color, true);
        }
    }
}

/* Helper function to draw color bars */
static void draw_color_bars(void)
{
    const uint32_t colors[] = {
        TEST_WHITE, TEST_YELLOW, TEST_CYAN, TEST_GREEN,
        TEST_MAGENTA, TEST_RED, TEST_BLUE, TEST_BLACK
    };
    
    int bar_width = LCD_WIDTH / 8;
    
    for (int i = 0; i < 8; i++) {
        hal_lcd_rect_t bar = {
            (uint16_t)(i * bar_width),
            0,
            (uint16_t)bar_width,
            LCD_HEIGHT
        };
        hal_lcd_draw_rectangle(bar, colors[i], true);
    }
}

/* Helper function to draw test pattern */
static void draw_test_pattern(void)
{
    hal_lcd_clear(TEST_BLACK);
    
    /* Draw border */
    hal_lcd_rect_t border = {0, 0, LCD_WIDTH, LCD_HEIGHT};
    hal_lcd_draw_rectangle(border, TEST_WHITE, false);
    
    /* Draw center rectangle */
    hal_lcd_rect_t center = {LCD_WIDTH/4, LCD_HEIGHT/4, LCD_WIDTH/2, LCD_HEIGHT/2};
    hal_lcd_draw_rectangle(center, TEST_RED, true);
    
    /* Draw corner markers */
    hal_lcd_rect_t corners[] = {
        {10, 10, 30, 30},                                          // Top-left
        {LCD_WIDTH-40, 10, 30, 30},                               // Top-right
        {10, LCD_HEIGHT-40, 30, 30},                              // Bottom-left
        {LCD_WIDTH-40, LCD_HEIGHT-40, 30, 30}                     // Bottom-right
    };
    
    for (int i = 0; i < 4; i++) {
        hal_lcd_draw_rectangle(corners[i], TEST_YELLOW, true);
    }
    
    /* Draw cross lines */
    for (uint16_t x = 0; x < LCD_WIDTH; x++) {
        hal_lcd_set_pixel(x, LCD_HEIGHT/2, TEST_GREEN);
    }
    for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
        hal_lcd_set_pixel(LCD_WIDTH/2, y, TEST_GREEN);
    }
}

/* Helper function to blend two colors */
static uint32_t blend_colors(uint32_t color1, uint32_t color2, float ratio)
{
    uint8_t a1 = (color1 >> 24) & 0xFF;
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    
    uint8_t a2 = (color2 >> 24) & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t a = (uint8_t)(a1 + (a2 - a1) * ratio);
    uint8_t r = (uint8_t)(r1 + (r2 - r1) * ratio);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * ratio);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * ratio);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/* Interactive test menu */
static void interactive_test_menu(void)
{
    int choice;
    
    while (1) {
        hal_lcd_clear(TEST_BLACK);
        
        printf("\n=== Interactive LCD Test Menu ===\n");
        printf("1. Basic Colors\n");
        printf("2. Pixel Drawing\n");
        printf("3. Rectangles\n");
        printf("4. Gradients\n");
        printf("5. Patterns\n");
        printf("6. Performance Test\n");
        printf("7. All Tests\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input!\n");
            continue;
        }
        
        switch (choice) {
            case 1:
                test_basic_colors();
                break;
            case 2:
                test_pixel_drawing();
                break;
            case 3:
                test_rectangles();
                break;
            case 4:
                test_gradients();
                break;
            case 5:
                test_patterns();
                break;
            case 6:
                test_performance();
                break;
            case 7:
                test_basic_colors();
                test_pixel_drawing();
                test_rectangles();
                test_gradients();
                test_patterns();
                test_performance();
                break;
            case 0:
                return;
            default:
                printf("Invalid choice!\n");
                break;
        }
        
        printf("Test complete. Press Enter to continue...");
        getchar();
        getchar(); // Consume newline
    }
}

int main(int argc, char *argv[]) {
    printf("=== STM32MP157F-DK2 Comprehensive LCD Test ===\n");
    printf("LCD Resolution: %dx%d pixels\n", LCD_WIDTH, LCD_HEIGHT);
    printf("Color Format: 32-bit ARGB\n\n");

    /* Initialize HAL first */
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

    printf("LCD initialized successfully\n");
    printf("Screen: %dx%d pixels\n", LCD_WIDTH, LCD_HEIGHT);

    /* Check for command line arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "auto") == 0) {
            printf("\nRunning automatic test sequence...\n");
            test_basic_colors();
            test_pixel_drawing();
            test_rectangles();
            test_gradients();
            test_patterns();
            test_performance();
        } else if (strcmp(argv[1], "interactive") == 0) {
            interactive_test_menu();
        } else {
            printf("Usage: %s [auto|interactive]\n", argv[0]);
            printf("  auto       - Run all tests automatically\n");
            printf("  interactive - Interactive test menu\n");
            printf("  (no args)  - Run basic test sequence\n");
        }
    } else {
        /* Default: run basic test sequence */
        printf("\nRunning basic test sequence...\n");
        test_basic_colors();
        test_pixel_drawing();
        test_rectangles();
    }

    /* Final cleanup screen */
    printf("\nTest sequence complete. Clearing screen...\n");
    hal_lcd_clear(TEST_BLACK);
    sleep(1);

    /* Cleanup */
    hal_lcd_deinit();
    
    printf("LCD test completed successfully!\n");
    return EXIT_SUCCESS;
}
#include "../include/hal.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    hal_status_t status;
    
    printf("=== STM32MP157F-DK2 HAL LED Test ===\n");
    printf("HAL Version: %s\n\n", hal_get_version());
    
    /* Initialize HAL */
    status = hal_init();
    if (status != HAL_OK) {
        printf("Failed to initialize HAL: %d\n", status);
        return 1;
    }
    
    printf("Testing individual LED control...\n");
    
    /* Test each LED individually */
    const char* led_names[] = {"Green (LD5)", "Red (LD6)", "Orange (LD7)", "Blue (LD8)"};
    
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        printf("\nTesting %s LED...\n", led_names[i]);
        
        /* Turn on LED */
        status = hal_led_set_state((hal_led_t)i, HAL_LED_ON);
        if (status != HAL_OK) {
            printf("Error turning on LED %d: %d\n", i, status);
            continue;
        }
        sleep(1);
        
        /* Turn off LED */
        status = hal_led_set_state((hal_led_t)i, HAL_LED_OFF);
        if (status != HAL_OK) {
            printf("Error turning off LED %d: %d\n", i, status);
            continue;
        }
        sleep(1);
    }
    
    printf("\nTesting LED patterns...\n");
    
    /* Test different patterns */
    uint8_t patterns[] = {0x0F, 0x05, 0x0A, 0x03, 0x0C, 0x00};
    int pattern_count = sizeof(patterns) / sizeof(patterns[0]);
    
    for (int i = 0; i < pattern_count; i++) {
        printf("Pattern 0x%02X: ", patterns[i]);
        for (int bit = 0; bit < 4; bit++) {
            printf("%c", (patterns[i] & (1 << bit)) ? '1' : '0');
        }
        printf("\n");
        
        status = hal_led_set_pattern(patterns[i]);
        if (status != HAL_OK) {
            printf("Error setting pattern: %d\n", status);
        }
        sleep(2);
    }
    
    printf("\nTesting LED toggle functionality...\n");
    
    /* Turn on all LEDs first */
    hal_led_set_pattern(0x0F);
    sleep(1);
    
    /* Toggle each LED */
    for (int i = 0; i < HAL_LED_COUNT; i++) {
        printf("Toggling LED %d (%s)\n", i, led_names[i]);
        hal_led_toggle((hal_led_t)i);
        sleep(1);
    }
    
    printf("\nTest complete. Cleaning up...\n");
    
    /* Cleanup */
    hal_deinit();
    
    printf("HAL test finished successfully!\n");
    return 0;
}

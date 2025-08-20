#include <string.h>
#include "../include/hal.h"
#include "../include/hal_ui.h"

static int s_enabled = 0;

int hal_ui_init(const char *card) {
    if (hal_lcd_init() < 0) {
        s_enabled = 0;
        return -1;
    }
    s_enabled = 1;
    return 0;
}

void hal_ui_shutdown(void) {
    if (s_enabled) {
        hal_lcd_shutdown();
        s_enabled = 0;
    }
}

int hal_ui_info(hal_ui_info_t *out) {
    if (!s_enabled) return -1;

    hal_ui_info_t lcd_info;
    if (hal_lcd_get_info(&lcd_info) < 0) {
        return -1;
    }

    if (out) {
        out->w = lcd_info.w;
        out->h = lcd_info.h;
        out->bpp = lcd_info.bpp;
        out->pitch = lcd_info.pitch;
    }

    return 0; // success
}

void hal_ui_clear(uint32_t color) {
    if (!s_enabled) return;
    hal_lcd_clear(color);
}

void hal_ui_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!s_enabled) return;
    hal_lcd_rect_t rect = { .x = x, .y = y, .width = w, .height = h };
    hal_lcd_draw_rectangle(rect, color, true);
}

int hal_ui_bar3(int cpu, int mem, int temp) {
    if (!s_enabled) return -1;

    // Normalize values to 0-100 range
    cpu = (cpu < 0) ? 0 : (cpu > 100) ? 100 : cpu;
    mem = (mem < 0) ? 0 : (mem > 100) ? 100 : mem;
    temp = (temp < 0) ? 0 : (temp > 100) ? 100 : temp;

    hal_ui_info_t info;
    if (hal_ui_info(&info) < 0) {
        return -1; // UI not initialized
    }

    int W = info.w;
    int H = info.h;
    int bars = 3;
    int bh = H / (bars + 1);
    int gap = bh / 2;
    hal_ui_clear(0x101010); // Clear with dark gray

    struct {int v; uint32_t c;} B[3] = {
        {cpu, 0xFF0000}, // Red for CPU
        {mem, 0x00FF00}, // Green for Memory
        {temp, 0x0000FF} // Blue for Temperature
    };

    for (int i = 0; i < bars; i++) {
        int bar_height = (B[i].v * bh) / 100;
        int y = H - (i + 1) * bh + gap;
        hal_ui_fill_rect(0, y, W * B[i].v / 100, bar_height, B[i].c);
    }
    hal_lcd_swap();

    return 0; // success
}

// hal_ui.h
#pragma once
#include <stdint.h>

typedef struct {
    int w, h;
    int bpp;
    int pitch;
} hal_ui_info_t;

int  hal_ui_init(const char *card);   // NULL => auto
void hal_ui_shutdown(void);
int  hal_ui_info(hal_ui_info_t *out);         // 0 ok, <0 disabled

void hal_ui_clear(uint32_t color);            // 
void hal_ui_fill_rect(int x,int y,int w,int h,uint32_t color);
int  hal_ui_bar3(int cpu,int mem,int temp);   // 0..100, no-op nếu LCD off

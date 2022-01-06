#ifndef VGA_TEXT
#define VGA_TEXT

#include "common.h"

void update_cursor(void);
void clear_screen(void);
void print_char(char c);
void print_char_color(char c, unsigned char color);
void print(const char *str);
void print_int(int num);
void print_hex(uint64_t num);

enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHTGRAY = 7,
    VGA_DARKGRAY = 8,
    VGA_LIGHTBLUE = 9,
    VGA_LIGHTGREEN = 10,
    VGA_LIGHTCYAN = 11,
    VGA_LIGHTRED = 12,
    VGA_PINK = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15,
};

#define VGA_COLOR(FOREGROUND, BACKGROUND) (((BACKGROUND) << 4) | (FOREGROUND))

#endif

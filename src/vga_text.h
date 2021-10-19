#ifndef VGA_TEXT
#define VGA_TEXT

#include "common.h"

void update_cursor(void);
void clear_screen(void);
void print_char(char c);
void print(const char *str);
void print_int(int num);
void print_hex(uint64_t num);

#endif

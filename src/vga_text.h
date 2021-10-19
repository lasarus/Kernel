#ifndef VGA_TEXT
#define VGA_TEXT

void update_cursor(void);
void clear_screen(void);
void print(const char *str);
void print_int(int num);

#endif

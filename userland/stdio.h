#ifndef STDIO_H
#define STDIO_H

int getchar();
int putchar(int c);
void puts(const char *str);
__attribute__((format(printf, 1, 2))) void printf(const char *fmt, ...);

#endif

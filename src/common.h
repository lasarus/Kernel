#ifndef COMMON_H
#define COMMON_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void hang_kernel(void);
void load_idt(void *ptr);

#define COUNTOF(X) (sizeof (X) / sizeof (*X))

#endif

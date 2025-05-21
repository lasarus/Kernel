#ifndef COMMON_H
#define COMMON_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef unsigned long size_t;
typedef unsigned long uintptr_t;
typedef long ssize_t;

#define UINT64_MAX 0xffffffffffffffff

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void hang_kernel(void);
void sleep_kernel(void);
void load_idt(uint16_t limit, void *base);
void load_gdt(uint16_t limit, void *base);
void load_tss(uint16_t gdt_entry);
uint64_t round_up_4096(uint64_t val);
void load_cr3(uint64_t pml4_ptr);
void reload_cr3(void);
uint64_t get_cr2(void);
uint64_t get_cr3(void);

void print_interrupt(void);

#define COUNTOF(X) (sizeof(X) / sizeof(*(X)))

#define GIBIBYTE 0x40000000
#define MEBIBYTE 0x100000
#define KIBIBYTE 0x400
#define HIGHER_HALF_OFFSET 0xFFFFFFFFC0000000
#define HIGHER_HALF_IDENTITY 0xFFFF800000000000

#define offsetof(type, member) __builtin_offsetof(type, member)

#define NULL ((void *)0)

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);

typedef long time_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

#define ERROR(STR, ...)      \
	do {                     \
		print("\nError: ");  \
		print(__FILE__);     \
		print(":");          \
		print_int(__LINE__); \
		print(" ");          \
		print(STR);          \
		hang_kernel();       \
	} while (0)

#endif

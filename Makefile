.PHONY: all clean run

all: kernel.bin

run: kernel.bin userland_a.elf userland_b.elf
	qemu-system-x86_64 --kernel kernel.bin -initrd userland_a.elf,userland_b.elf

clean:
	rm -rf kernel.bin
	rm -rf userland.bin

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic -Os -Wno-unused-variable -Wno-unused-parameter

userland_a.elf: userland/userland_a.s
	gcc -T userland/userland_elf.ld $^ -o userland_a.elf -no-pie -ffreestanding -nostdlib

userland_b.elf: userland/userland_b.s
	gcc -T userland/userland_elf.ld $^ -o userland_b.elf -no-pie -ffreestanding -nostdlib

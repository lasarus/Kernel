.PHONY: all clean run

all: kernel.bin hello.elf shell.elf

run: kernel.bin hello.elf shell.elf
	qemu-system-x86_64 --kernel kernel.bin -initrd shell.elf,hello.elf -no-reboot -no-shutdown

clean:
	rm -rf kernel.bin
	rm -rf hello.elf
	rm -rf shell.elf

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic -Os -Wno-unused-variable -Wno-unused-parameter

hello.elf: userland/hello.c userland/syscalls.s
	gcc -T userland/userland_elf.ld $^ -o hello.elf -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

shell.elf: userland/shell.c userland/syscalls.s
	gcc -T userland/userland_elf.ld $^ -o shell.elf -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

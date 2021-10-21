.PHONY: all clean run

all: kernel.bin

run: kernel.bin userland_a.bin userland_b.bin
	qemu-system-x86_64 --kernel kernel.bin -initrd userland_a.bin,userland_b.bin

clean:
	rm -rf kernel.bin
	rm -rf userland.bin

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic

userland_a.bin: userland/userland_a.s
	gcc -T userland/userland_link.ld $^ -o userland_a.bin -no-pie -ffreestanding -nostdlib

userland_b.bin: userland/userland_b.s
	gcc -T userland/userland_link.ld $^ -o userland_b.bin -no-pie -ffreestanding -nostdlib

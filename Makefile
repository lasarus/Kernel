.PHONY: all clean run

all: kernel.bin

run: kernel.bin userland.bin
	qemu-system-x86_64 --kernel kernel.bin -initrd userland.bin

clean:
	rm -rf kernel.bin
	rm -rf userland.bin

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic

userland.bin: userland/userland_a.s
	gcc -T userland/userland_link.ld $^ -o userland.bin -no-pie -ffreestanding -nostdlib

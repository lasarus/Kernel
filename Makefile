.PHONY: all clean run

all: kernel.bin

run: kernel.bin
	qemu-system-x86_64 --kernel kernel.bin

clean:
	rm -rf kernel.bin

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie
	objcopy -O elf32-i386 -I elf32-x86-64 kernel.bin kernel.bin

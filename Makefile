.PHONY: all clean run

all: kernel.bin

run: kernel.bin
	qemu-system-x86_64 --kernel kernel.bin -initrd module.txt

clean:
	rm -rf kernel.bin

kernel.bin: src/*.s src/*.c
	gcc -T src/link.ld -o kernel.bin $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os

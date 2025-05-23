USERLAND := shell echo cat ls mkdir
USERLAND_SRCS := $(USERLAND:%=userland/%.c)
USERLAND_OBJS := $(USERLAND:%=build/%)
INITRD := build/initrd.tar

.PHONY: all clean run reformat

all: build/kernel.bin $(USERLAND_OBJS)

run: build/kernel.bin $(USERLAND_OBJS) README.md $(INITRD)
	qemu-system-x86_64 --kernel build/kernel.bin -initrd $(INITRD) -no-reboot -no-shutdown

clean:
	rm -rf build

build/kernel.bin: src/*.s src/*.c src/*.h
	mkdir -p build
	gcc -T src/link.ld -o build/kernel.elf $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -g
	objcopy -O binary build/kernel.elf build/kernel.bin
	objcopy --only-keep-debug build/kernel.elf build/kernel.debug

$(INITRD): $(USERLAND_OBJS) README.md
	mkdir -p build/initrd/{bin,home,dev}
	cp README.md build/initrd/home/
	cp $(USERLAND_OBJS) build/initrd/bin/
	cp build/shell build/initrd/bin/init
	tar -cf $@ -C build/initrd .

build/%: userland/%.c userland/syscalls.s userland/stdio.c
	mkdir -p build
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

reformat:
	clang-format -i src/*.[ch]

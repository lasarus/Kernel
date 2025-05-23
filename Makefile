USERLAND := hello.elf shell.elf echo.elf cat.elf ls.elf mkdir.elf

.PHONY: all clean run

all: kernel.bin $(USERLAND)

run: kernel.bin $(USERLAND) README.md
	qemu-system-x86_64 --kernel kernel.bin -initrd shell.elf,hello.elf,echo.elf,cat.elf,ls.elf,mkdir.elf,README.md -no-reboot -no-shutdown

clean:
	rm -rf kernel.bin
	rm -rf hello.elf
	rm -rf shell.elf
	rm -rf echo.elf

kernel.bin: src/*.s src/*.c src/*.h
	gcc -T src/link.ld -o kernel.elf $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -g
	objcopy -O binary kernel.elf kernel.bin
	objcopy --only-keep-debug kernel.elf kernel.debug

hello.elf: userland/hello.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

echo.elf: userland/echo.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

shell.elf: userland/shell.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

cat.elf: userland/cat.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

ls.elf: userland/ls.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

mkdir.elf: userland/mkdir.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

reformat:
	clang-format -i src/*.[ch]

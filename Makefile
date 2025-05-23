USERLAND := hello shell echo cat ls mkdir
INITRD := initrd.tar

.PHONY: all clean run

all: kernel.bin $(USERLAND)

run: kernel.bin $(USERLAND) README.md $(INITRD)
	qemu-system-x86_64 --kernel kernel.bin -initrd $(INITRD) -no-reboot -no-shutdown

clean:
	rm -rf kernel.bin
	rm -rf $(INITRD) $(USERLAND)
	rm -rd initrd/

kernel.bin: src/*.s src/*.c src/*.h
	gcc -T src/link.ld -o kernel.elf $^ --freestanding -nostdlib -no-pie -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fno-asynchronous-unwind-tables -mcmodel=kernel -fno-pie -fno-stack-protector -Os -Wall -Werror -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -g
	objcopy -O binary kernel.elf kernel.bin
	objcopy --only-keep-debug kernel.elf kernel.debug

$(INITRD): $(USERLAND) README.md
	@mkdir -p initrd
	@mkdir -p initrd/bin
	@mkdir -p initrd/home/
	@mkdir -p initrd/dev/
	cp README.md initrd/home/
	cp $(USERLAND) initrd/bin/
	cp shell initrd/bin/init
	tar -cf $@ -C initrd .

hello: userland/hello.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

echo: userland/echo.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

shell: userland/shell.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

cat: userland/cat.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

ls: userland/ls.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

mkdir: userland/mkdir.c userland/syscalls.s userland/stdio.c
	gcc -T userland/userland_elf.ld $^ -o $@ -no-pie -ffreestanding -nostdlib -mgeneral-regs-only -fno-stack-protector

reformat:
	clang-format -i src/*.[ch]

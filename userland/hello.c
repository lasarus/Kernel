#include "syscalls.h"

char str[] = "Hello World!\n";

int main(int argc, char **argv) {
	write(1, str, sizeof str - 1);
}

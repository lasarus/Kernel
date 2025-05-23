#include "syscalls.h"

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		mkdir(argv[i]);
	}
	return 0;
}

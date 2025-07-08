int _errno;
#define SYS_ERRNO _errno
#include "syscall.hpp"

// armv7a-linux-androideabi21-clang su.c -m32 -static -nostdlib -o su -Os

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

__attribute__((naked)) void _start() {
	__asm__ volatile (
        "ldr r0, [sp]\n\t"
        "add r1, sp, #4\n\t"
        "add r2, r1, r0, lsl #2\n\t"
        "add r2, r2, #4\n\t"

        "bl main\n\t"
        "mov r7, #1\n\t"
        "svc #0\n\t"
        "b .\n\t"
    );
}


int main(int argc, const char **argv, const char **envp) {
	unsigned long result = 0;
	
	if (argc >= 2 && strcmp(argv[1], "--disable-sucompat") == 0) {
		sys_prctl(0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
		return 0;
	}
	
	sys_prctl(0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
	if (result != 0xdeadbeef) {
		const char *error = "Denied\n";
		sys_write(2, error, strlen(error));
		return 1;
	}
	
	const char *args[] = { "/system/bin/su" };
	if (argc < 1) argv = args;
	else argv[0] = "/system/bin/su";
	
	sys_execve("/data/adb/ksud", argv, envp);
	
	const char *error = "Failed to execve /data/adb/ksud\n";
	sys_write(2, error, strlen(error));
	return 1;
}

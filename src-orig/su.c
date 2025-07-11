static int _errno;
#define SYS_ERRNO _errno
#include "syscall.hpp"
#include <asm/termbits.h>
#include <termios.h>

// aarch64-linux-android21-clang su.c -static -nostdlib -o su -Os

__attribute__((noinline)) int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

__attribute__((naked)) void _start() {
    __asm__ volatile (
        "ldr x0, [sp]\n\t"
        "add x1, sp, #8\n\t"

        "lsl x3, x0, #3\n\t"
        "add x2, x1, x3\n\t"
        "add x2, x2, #8\n\t"

        "bl main\n\t"

        "mov x8, #93\n\t"
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
	
	struct termios t;
	if (sys_ioctl(0, TCGETS, &t) == 0) {
		char pts[64];
		long ps = sys_readlink("/proc/self/fd/0", pts, sizeof(pts) - 1);
		if (ps != -1) {
			pts[ps] = '\0';
			const char *ctx = "u:object_r:devpts:s0";
			sys_setxattr(pts, "security.selinux", ctx, strlen(ctx) + 1, 0);
		}
    }
	
	const char *args[] = { "/system/bin/su" };
	if (argc < 1) argv = args;
	else argv[0] = "/system/bin/su";
	
	sys_execve("/data/adb/ksud", argv, envp);
	
	const char *error = "Failed to execve /data/adb/ksud\n";
	sys_write(2, error, strlen(error));
	return 1;
}

int _errno;
#define SYS_ERRNO _errno
#include "syscall.hpp"
#include <asm/termbits.h>
#include <termios.h>

// armv7a-linux-androideabi21-clang su.c -m32 -static -nostdlib -o su -Os

/*
user@7600es:[/tmp/optane/ndk]: $ ./android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang su.c -static -nostdlib -o su -Oz -s
user@7600es:[/tmp/optane/ndk]: $ /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip su
user@7600es:[/tmp/optane/ndk]: $ ls -la su
-rwxrwxr-x 1 user user 852 Jul 11 11:54 su

*/

/*
 * strnmatch, test two strings if they match up to n len
 * just like !!!strncmp
 * 0 = match
 * 1 = not match
 * 
 */
static int strnmatch(const char *a, const char *b, size_t count)
{
	// og condition was like *a && (*a == *b) && count > 0
	do {
		// if they arent equal
		if (*a != *b) 
			return 1;
		a++;
		b++;
		count --;
	} while (count > 0);
	// while count >0, we can omit (*a) check here
	// caller is responsible for bounds
	// *a costs 28 bytes

	// we reach here if they match
	return 0;
}

__attribute__((naked))
void _start() {
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

__attribute__((noreturn))
int main(int argc, const char **argv, const char **envp)
{
	unsigned long result = 0;
	
	int is_on_data = !strnmatch(argv[0], "/data", 5);

	if (is_on_data) {
		sys_prctl(0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
		goto segfault;
	}

	sys_prctl(0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
	if (result != 0xdeadbeef)
		goto segfault;
	
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

	argv[0] = "su";
	sys_execve("/data/adb/ksud", argv, envp);

segfault:	
	*(volatile int *)0 = 0;
}

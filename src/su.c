int _errno;
#define SYS_ERRNO _errno
#include "syscall.hpp"

// armv7a-linux-androideabi21-clang su.c -m32 -static -nostdlib -o su -Os

/*
 * $ ./android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang su.c -static -nostdlib -o su -Oz -s
 * $ ls -la su
 * -rwxrwxr-x 1 user user 1748 Jul  9 16:08 su
 * $ /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip su
 * $ ls -la su
 * -rwxrwxr-x 1 user user 1024 Jul  9 16:08 su

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

static int denied(void)
{
	const char *error = "Denied\n";
	sys_write(2, error, strlen(error));
	return 1;
}

int main(int argc, const char **argv, const char **envp)
{
	unsigned long result = 0;
	
	if (argc >= 2 && !strnmatch(argv[0], "/data/adb", 9)
		&& !strnmatch(argv[1], "--disable-sucompat", 18)) {
		sys_prctl(0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
		return 0;
	}

	// this costs 36 bytes
	// if its called from /data/adb, dont continue!
	// if (!strnmatch(argv[0], "/data/adb", 9))
	// 	return denied();

	sys_prctl(0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
	if (result != 0xdeadbeef)
		return denied();

	
	// const char *args[] = { "/system/bin/su" };
	// if (argc < 1) argv = args;
	// else argv[0] = "/system/bin/su";
	
	// lets just segfault if user passes argc == 0
	// that prolly doesnt happen
	argv[0] = "/system/bin/su";
	
	char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	int fd = sys_open("/dev/kmsg", 1, 0);
	if (fd >= 0) {
		sys_write(fd, debug_msg, strlen(debug_msg));
		sys_close(fd);
	}
	
	// sys_execve("/data/adb/ksud", argv, envp);	
	// const char *error = "Failed to execve /data/adb/ksud\n";
	// sys_write(2, error, strlen(error));
	// return 1;
	
	sys_execve("/data/adb/ksud", argv, envp);
	
	return denied();
}

// dummy for that 1024 meme
__attribute__((section(".text"))) 
const char dummy_padding[3];

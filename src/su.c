#include <sys/syscall.h>
#include <unistd.h>

#define O_WRONLY    1
#define AT_FDCWD   (-100)

// zig cc -target arm-linux -s -Os su.c -static -Wl,--gc-sections
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip a.out

/*
 * strnmatch, test two strings if they match up to n len
 * just like !!!strncmp
 * 0 = match
 * 1 = not match
 * 
 */

// caller is reposnible for sanity! no \0 check!
static int strnmatch(const char *a, const char *b, unsigned short count)
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

	// we reach here if they match
	return 0;
}

static int denied(void)
{
	const char *error = "Denied\n";
	syscall(SYS_write, 2, error, 7);
	return 1;
}

int main(int argc, const char **argv, const char **envp)
{
	unsigned long result = 0;
	
	int is_data_adb = !strnmatch(argv[0], "/data/adb", 9);
	
	if (argc >= 2 && is_data_adb
		&& !strnmatch(argv[1], "--disable-sucompat", 18)) {
		syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
		return 0;
	}

	// if its called from /data/adb, dont continue!
	if (is_data_adb)
	 	return denied();

	syscall(SYS_prctl, 0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
	if (result != 0xdeadbeef)
		return denied();

#if 0
	const char *args[] = { "/system/bin/su" };
	if (argc < 1) argv = args;
	else argv[0] = "/system/bin/su";
#endif
	// lets just segfault if user passes argc == 0
	// since that prolly doesnt happen
	argv[0] = "/system/bin/su";
	
	// https://syscalls.mebeim.net/?table=arm64/64/aarch64/v5.0
	// openat is available on both arm and aarch64, use this instead of open

	char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	int fd = syscall(SYS_openat, AT_FDCWD, "/dev/kmsg", O_WRONLY, 0);
	if (fd >= 0) {
		syscall(SYS_write, fd, debug_msg, 30);
		syscall(SYS_close, fd);
	}

	syscall(SYS_execve, "/data/adb/ksud", argv, envp);
	
	return denied();
}

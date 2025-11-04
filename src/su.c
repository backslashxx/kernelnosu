#include <sys/syscall.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h> 
#include <fcntl.h>

// zig cc -target arm-linux -s -Os su.c -static -Wl,--gc-sections
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip a.out

/*
 * strnmatch, test two strings if they match up to n len
 *
 * caller is reposnible for sanity! no \0 check!
 * returns: 0 = match, 1 = not match
 *
 * Usage examples:
 * for strcmp like behavior, strnmatch(x, y, strlen(y) + 1) (+1 for \0)
 * for strstarts like behavior strnmatch(x, y, strlen(y))
 *  
 */
static int strnmatch(const char *a, const char *b, unsigned int count)
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
	syscall(SYS_write, 2, error, strlen(error));
	return 1;
}

/*
 * https://syscalls.mebeim.net/?table=arm64/64/aarch64/v5.0
 * the choice of syscalls used here is to keep compatibility
 * sys_open* to sys_openat
 * sys_readlink* to sys_readlinkat
 * this way we can have native aarch64 binaries
 * and avoid future potential pointer mismatch
 * we dont want to compat.ptr all the time on ksu driver.
 *
 * * not on aarch64!
 */

// ksu's new supercall
#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)

int main(int argc, const char **argv, const char **envp)
{
	unsigned long result = 0;
	int fd = 0;
	
	int is_data = !strnmatch(argv[0], "/data", 5);
	
	if (argc >= 2 && is_data) {
		if (!strnmatch(argv[1], "--disable-sucompat", 19)) { 
			syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
			return 0;
		}

		// since theres massive feature fragmentation on ksu forks
		// we can't just rely on version checking for shit
		// we need to actually test for this feature instead
		if (!strnmatch(argv[1], "--test-15", 10)) { 
			syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
			if (result == 0xdeadbeef) {
				// enable it again, check is arg3 !=0
				syscall(SYS_prctl, 0xdeadbeef, 15L, 1L, 0L, (unsigned long) &result);
				//syscall(SYS_write, 2, "ok\n", 3);
				return 0;
			} else
				return denied();
		}
	}

	// if its called from /data/adb, dont continue!
	if (is_data)
	 	return denied();

	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (void *)&fd);
	if (fd == 0) {
		// so its likely on old interface, try escalating via prctl
		syscall(SYS_prctl, 0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
		if (result != 0xdeadbeef)
			return denied();
	} else {
		int ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0);
		syscall(SYS_close, fd); // close it here regardless
		if (ret < 0)
			return denied();
	}

	struct termios t;
	if (syscall(SYS_ioctl, 0, TCGETS, &t) == 0) {
		char pts[64];
		long ps = syscall(SYS_readlinkat, AT_FDCWD, "/proc/self/fd/0", pts, sizeof(pts) - 1);
		if (ps != -1) {
			pts[ps] = '\0';
			const char *ctx = "u:object_r:devpts:s0";
			syscall(SYS_setxattr, pts, "security.selinux", ctx, strlen(ctx) + 1, 0);
		}
	}

	argv[0] = "su";

	char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	fd = syscall(SYS_openat, AT_FDCWD, "/dev/kmsg", O_WRONLY, 0);
	if (fd >= 0) {
		syscall(SYS_write, fd, debug_msg, strlen(debug_msg));
		syscall(SYS_close, fd);
	}

	syscall(SYS_execve, "/data/adb/ksud", argv, envp);
	
	return denied();
}

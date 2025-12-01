#include <sys/syscall.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// zig cc -target arm-linux -s -Os su.c -static -Wl,--gc-sections
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip a.out

// https://gcc.gnu.org/onlinedocs/gcc/Library-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-functions
#define memcmp __builtin_memcmp
#define strlen __builtin_strlen

// ksu's new supercall
#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)

int main(int argc, const char **argv, const char **envp)
{
	const char *error = "Denied\n";
	unsigned long result = 0;
	int fd = 0;
	
	int is_data = !memcmp(argv[0], "/data", strlen("/data"));
	
	if (is_data) {
		if (!memcmp(argv[1], "--disable-sucompat", strlen("--disable-sucompat") + 1 )) { 
			syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
			return 0;
		}

		// since theres massive feature fragmentation on ksu forks
		// we can't just rely on version checking for shit
		// we need to actually test for this feature instead
		if (!memcmp(argv[1], "--test-15", strlen("--test-15") + 1)) { 
			syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (unsigned long) &result);
			if (result == 0xdeadbeef) {
				// enable it again, check is arg3 !=0
				syscall(SYS_prctl, 0xdeadbeef, 15L, 1L, 0L, (unsigned long) &result);
				//syscall(SYS_write, 2, "ok\n", 3);
				return 0;
			} else
				goto denied;
		}

		// if its called from /data/adb, dont continue!
		goto denied;
	}

	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (void *)&fd);
	if (fd == 0) {
		// so its likely on old interface, try escalating via prctl
		syscall(SYS_prctl, 0xdeadbeef, 0L, 0L, 0L, (unsigned long) &result);
		if (result != 0xdeadbeef)
			goto denied;
	} else {
		int ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0);
		if (ret < 0)
			goto denied;
	}

	argv[0] = "su";

	char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	fd = syscall(SYS_openat, AT_FDCWD, "/dev/kmsg", O_WRONLY, 0);
	if (fd >= 0) {
		syscall(SYS_write, fd, debug_msg, strlen(debug_msg));
		syscall(SYS_close, fd);
	}

	syscall(SYS_execve, "/data/adb/ksud", argv, envp);

denied:	
	syscall(SYS_write, 2, error, strlen(error));
	return 1;
}

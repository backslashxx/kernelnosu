#include <sys/ioctl.h>
#include <fcntl.h>
#include "small_rt.h"

// zig cc -target aarch64-linux su.c -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -o su
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip a.out

#define memcmp __builtin_memcmp
#define strlen __builtin_strlen

// ksu's new supercall
#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)

static int c_main(int argc, char **argv, char **envp)
{
	const char *error = "Denied\n";
	unsigned long result = 0;
	int fd = 0;
	
	int is_data = !memcmp(argv[0], "/data", strlen("/data"));
	
	if (is_data) {
		if (!memcmp(argv[1], "--disable-sucompat", strlen("--disable-sucompat") + 1 )) { 
			__syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (long)&result, NONE);
			return 0;
		}

		// since theres massive feature fragmentation on ksu forks
		// we can't just rely on version checking for shit
		// we need to actually test for this feature instead
		if (!memcmp(argv[1], "--test-15", strlen("--test-15") + 1)) { 
			__syscall(SYS_prctl, 0xdeadbeef, 15L, 0L, 0L, (long)&result, NONE);
			if (result == 0xdeadbeef) {
				// enable it again, check is arg3 !=0
				__syscall(SYS_prctl, 0xdeadbeef, 15L, 1L, 0L, (long)&result, NONE);
				return 0;
			} else
				goto denied;
		}

		// if its called from /data/adb, dont continue!
		goto denied;
	}

	__syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (long)&fd, NONE, NONE);
	if (fd == 0) {
		// so its likely on old interface, try escalating via prctl
		__syscall(SYS_prctl, 0xdeadbeef, 0L, 0L, 0L, (long)&result, NONE);
		if (result != 0xdeadbeef)
			goto denied;
	} else {
		int ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0, NONE, NONE, NONE);
		if (ret < 0)
			goto denied;
	}

	argv[0] = "su";

	const char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	const char *kmsg = "/dev/kmsg";
	fd = __syscall(SYS_openat, AT_FDCWD, (long)kmsg, O_WRONLY, 0, NONE, NONE);
	if (fd >= 0) {
		__syscall(SYS_write, fd, (long)debug_msg, strlen(debug_msg), NONE, NONE, NONE);
		__syscall(SYS_close, fd, NONE, NONE, NONE, NONE, NONE);
	}

	const char *ksud = "/data/adb/ksud";

	__syscall(SYS_execve, (long)ksud, (long)argv, (long)envp, NONE, NONE, NONE);

denied:	
	__syscall(SYS_write, 2, (long)error, strlen(error), NONE, NONE, NONE);
	return 1;
}

void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = argv + argc + 1; // we need to offset it by the number of argc's!

	long exit_code = c_main(argc, argv, envp);
	__syscall(SYS_exit, exit_code, NONE, NONE, NONE, NONE, NONE);
}


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
	int fd = 0;
	
	if (!memcmp(argv[0], "/data", strlen("/data")))
		goto denied;

	__syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (long)&fd, NONE, NONE);
	if (!fd)
		goto denied;

	int ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0, NONE, NONE, NONE);
	if (ret < 0)
		goto denied;

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


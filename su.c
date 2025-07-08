#include <linux/prctl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#define AT_EMPTY_PATH 0x1000

int main(int argc, char **argv, char **envp) {
	unsigned long result = 0;
	if (argc >= 2 && strcmp(argv[1], "--disable-sucompat") == 0) {
		argv[1] = "--disable-sucompat2";
		int fd = open("/system/bin/su", O_PATH | O_CLOEXEC);
		syscall(__NR_execveat, fd, "", argv, envp, AT_EMPTY_PATH);
		perror("execveat");
		return 1;
	}
	
	if (argc >= 2 && strcmp(argv[1], "--disable-sucompat2") == 0) {
		prctl(0xdeadbeef, 15L, 0L, 0L, &result);
		return 0;
	}
	
	prctl(0xdeadbeef, 0L, 0L, 0L, &result);
	if (result != 0xdeadbeef) {
		fprintf(stderr, "Denied\n");
		return 1;
	}
	
	char *args[] = { "/system/bin/su" };
	if (argc < 1) argv = args;
	else argv[0] = "/system/bin/su";
	
	execv("/data/adb/ksud", argv);
	perror("execv");
	return 1;
}

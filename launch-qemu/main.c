#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>

int main(int argc, char** argv, char** envp)
{
	if (execlp("qemu-system-i386", "qemu-system-i386", 
		"-drive", "file=release/obos.iso,format=raw",
		"-s",
		"-m", "1G",
		"-mem-prealloc",
		"-serial", "file:./log.txt", 
		"-serial", "tcp:0.0.0.0:1534,server",
		"-no-reboot",
		"-d", "int",
		"-soundhw", "pcspk",
		NULL) == -1)
	{
		perror("execvp: ");
		return errno; 
	}
	return 0;
}
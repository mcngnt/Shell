#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

int main (int argc, char **argv)
{
	execlp("ls", "ls","*.c", NULL);
	return 0;
}

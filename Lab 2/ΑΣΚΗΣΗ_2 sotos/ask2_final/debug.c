#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>


int main(int argc, char *argv[])
{
int pfd[2];
int answer=2,readnum;
if (pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}
pid_t pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		write(pfd[1],&answer, sizeof(int));
		exit(1);
	}
	read(pfd[0],&readnum,sizeof(int));
printf("%d\n",readnum);
exit(5);
}

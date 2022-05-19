#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"


void fork_procs(struct tree_node *root, int *pfd)
{
	
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	change_pname(root->name);		//dinw onoma sti diergasia
    int num = atoi (root->name);

    if (root->nr_children > 0)	//an exei pedia ksekina na kanei fork
    {
        pid_t pid[2];
        int status[2],ch_pfd[2],i,num1,num2,ans;;

    if (pipe(ch_pfd) < 0) {
		perror("pipe");
		exit(1);
	}
    
     for (i=0; i < 2; i++)
     {
         pid[i] = fork();
	      if (pid[i] < 0) {
		        perror("main: fork");
		        exit(1);
	        }
	        if (pid[i] == 0) {
		        fork_procs(root->children + i,ch_pfd);
		       exit(1);
	        }
    wait_for_ready_children(1);
    }
      raise(SIGSTOP);

      for (i=0; i < 2; i++){
        kill(pid[i],SIGCONT);
        pid[i] = wait(&status[i]);
	    explain_wait_status(pid[i], status[i]);
      }

    if (read(ch_pfd[0], &num1, sizeof(int) ) != sizeof(int) ) {
		perror("read from pipe");
		exit(1);
		}

	if (read(ch_pfd[0], &num2, sizeof(int) ) != sizeof(int) ) {
		perror("read from pipe");
		exit(1);
		}   
    int x = root->name[0];
    if (x ==  42) ans = (num1)*(num2);
	else if (x ==  43) ans = num1+num2;
	else exit(1);

    printf("%d %d %d\n\n",ans,num1,num2);

    if (write(pfd[1],&ans, sizeof(int)) != sizeof(int) ) {	
				perror("write to pipe");
				exit(1);
			}
         

    }
    else {
        raise(SIGSTOP);
        if (write(pfd[1],&num, sizeof(int)) != sizeof(int) ) {		//writes to the pipe
				perror("write to pipe");
				exit(1);
			}
        }


	exit(0);
}



int main(int argc, char *argv[])
{
	pid_t pid;
	int status,ans,pfd[2];
	struct tree_node *root;

	//elexos oti iparxei arxio
	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);	//an iparxe, dimiourgoume to structure

    
    if (root==NULL)						//an einai adeio exit
		exit(1);

    if (pipe(pfd) < 0) {	
		perror("pipe");
		exit(1);
	}

	pid = fork();			//dimiourgia prwtou komvou
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root,pfd);	//arxizoume ti dimiourgia diergasiwn
		exit(1);
	}

	wait_for_ready_children(1);

	show_pstree(pid);

	kill(pid, SIGCONT);

    wait(&status);
	explain_wait_status(pid, status);
    
    
    if (read(pfd[0], &ans, sizeof(ans)) != sizeof(ans)) {
		perror("read from pipe");
		exit(1);
		}

	printf("The answer is: %d\n\n",ans);

	return 0;
}

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"


void fork_procs(struct tree_node *root, int *pfd) {
	
	printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
	change_pname(root->name);		
    int num = atoi (root->name);

    if (root->nr_children> 0) {
        pid_tpid[2];
        int status[2],ch_pfd[2],i,num1,num2,ans;

        if (pipe(ch_pfd) < 0) {
		    perror("pipe");
		    exit(1);
	    }

        for (i=0; i< 2; i++) {
            pid[i] = fork();
	        if (pid[i] < 0) {
		        perror("main: fork");
		        exit(1);
	        }
	        if (pid[i] == 0) {
		        fork_procs(root->children + i,ch_pfd);
		        exit(1);
	        }
        }

        for (i=0; i< 2; i++){
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
        if (x ==  42) {ans = (num1)*(num2);}
        else if {(x ==  43) ans = num1+num2;}
        else {exit(1);}

        printf("%d %d %d\n\n",ans,num1,num2);

        if (write(pfd[1],&ans, sizeof(int)) != sizeof(int) ) {	
                perror("write to pipe");
                exit(1);
        }
    }
    else {
        if (write(pfd[1],&num, sizeof(int)) != sizeof(int) ) {		//writes to the pipe
				perror("write to pipe");
				exit(1);
			}
        }

    exit(0);
}


int main(intargc, char *argv[]) {
	pid_tpid;
	intstatus,ans,pfd[2];
	struct tree_node *root;

	//check it exists
	if (argc<2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);	
    print_tree(root);

    if (root==NULL)	{exit(1);} //if null exit

    if (pipe(pfd) < 0) {	
		perror("pipe");
		exit(1);
	}

	pid = fork();   //create first
	if (pid< 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
	    fork_procs(root,pfd);	//start fork
		exit(1);
	}

	wait(&status);
	explain_wait_status(pid, status);

	if (read(pfd[0], &ans, sizeof(ans)) != sizeof(ans)) {
		perror("read from pipe");
		exit(1);
	}

	printf("The answer is: %d\n\n",ans);

	return 0;
}

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root) {
	int i;
	printf("PID = %ld, name %s, starting...\n", (long)getpid(), root->name); change_pname(root->name);
    
    if (root->nr_children> 0) {
        pid_tpid[root->nr_children];
        int status[root->nr_children];

        for (i=0; i< root->nr_children; i++) {
            pid[i] = fork();
            if (pid[i] < 0) {
                perror("main: fork");
                exit(1);
            }
            if (pid[i] == 0) {
                fork_procs(root->children + i);
                exit(1);
            }
            printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(),(long)pid[i]);
            wait_for_ready_children(1);
        }
        raise(SIGSTOP);
        printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);

        for (i=0; i< root->nr_children; i++) {
            kill(pid[i],SIGCONT);
            pid[i] = wait(&status[i]);
            explain_wait_status(pid[i], status[i]);
        }

        }
    else { raise(SIGSTOP);  printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name); }

    exit(0);
}

int main(intargc, char *argv[]) {
	pid_tpid;
	int status;
	struct tree_node *root;

	//check that exists
	if (argc<2) {
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	pid = fork();	//create first
	if (pid< 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root);	//start fork
		exit(1);
	}

	wait_for_ready_children(1);		//wait until child state changes
    show_pstree(pid);				
    kill(pid, SIGCONT);				//ignal child
    wait(&status);					//wait first
	explain_wait_status(pid, status);

	return 0;
}

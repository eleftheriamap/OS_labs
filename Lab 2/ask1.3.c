#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{
	int i;
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	change_pname(root->name);

    if (root->nr_children > 0)
    {
        pid_t pid[root->nr_children];
        int status[root->nr_children];
    
     for (i=0; i < root->nr_children; i++)
        {
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
	    
    printf("PID = %ld, name = %s is awake\n",
		(long)getpid(), root->name);

    for (i=0; i < root->nr_children; i++)
    {
        kill(pid[i],SIGCONT);
        pid[i] = wait(&status[i]);
	    explain_wait_status(pid[i], status[i]);
    }
    
    }
    else {raise(SIGSTOP);  printf("PID = %ld, name = %s is awake\n",
		(long)getpid(), root->name); }

	exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	//elexos oti iparxei arxio
	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();	//dimiourgia prwtou komvou
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root);	//arxizoume ti dimiourgia diergasiwn
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);		//perimenei na alla3ei katastasi to paidi tis (diladi na kanei stop i root)

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);				//etsi exoun dimiourgithei ola ta paidia kai tipwnw to dentro

	/* for ask2-signals */
	kill(pid, SIGCONT);				//stelnw shma sto paidi gia na sinexistoun oi diergasies 

	/* Wait for the root of the process tree to terminate */
	wait(&status);					//perimeno ton prwto komvo na pethanei	
	explain_wait_status(pid, status);

	return 0;
}

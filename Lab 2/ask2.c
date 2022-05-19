#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC 10
#define SLEEP_TREE_SEC  4

void fork_procs(struct tree_node *root) {
	change_pname(root->name);	//name process
    if (root->nr_children> 0) {
        pid_tpid[root->nr_children];
        int status[root->nr_children],i;
        
        for (i=0; i< root->nr_children; i++) {
            pid[i] = fork();
	        if (pid[i] < 0) {
		        perror("main: fork");
		        exit(1);
	        }
	        if (pid[i] == 0) {
		        fork_procs(root->children + i);	//start fork
		        exit(1);
	        }

	    }
        
        for (i=0; i< root->nr_children; i++) {
            pid[i] = wait(&status[i]);	//wait for first 
	        explain_wait_status(pid[i], status[i]);
        }
    }
    else sleep(SLEEP_PROC_SEC);	//sleep and exit
    exit(10);
}

int main(intargc, char *argv[]) {
	struct tree_node *root;

	//check if exists
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);	//if exists get root 

    pid_tpid;
	int status;

	pid = fork();   //create first
	if (pid< 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);	//start fork
		exit(1);
	}

    sleep(SLEEP_TREE_SEC);	
	show_pstree(pid);	//show tree

	pid = wait(&status);	//wait for first 
	explain_wait_status(pid, status);

	return 0;
}

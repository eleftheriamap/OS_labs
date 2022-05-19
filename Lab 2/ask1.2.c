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



void fork_procs(struct tree_node *root)
{
	change_pname(root->name);	//dinoume onoma stin diergasia
    if (root->nr_children > 0)	//an exei pedia ksekina na kanei fork 
    {
        pid_t pid[root->nr_children];
        int status[root->nr_children],i;
    
     for (i=0; i < root->nr_children; i++)
        {
         pid[i] = fork();
	      if (pid[i] < 0) {
		        perror("main: fork");
		        exit(1);
	        }
	        if (pid[i] == 0) {
		        fork_procs(root->children + i);	//arxizoume ti dimiourgia diergasiwn
		       exit(1);
	        }

	    }
        for (i=0; i < root->nr_children; i++)
        {
          pid[i] = wait(&status[i]);	//perimeno ton prwto komvo
	      explain_wait_status(pid[i], status[i]);
        }
    }
    else sleep(SLEEP_PROC_SEC);	//i dergasia kanei sleep k termatizei
exit(10);
}

int main(int argc, char *argv[])
{
	struct tree_node *root;

	//elexos oti iparxei arxio
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);	//an iparxe, dimiourgoume to structure k pernoume ti riza

    pid_t pid;
	int status;

	pid = fork();		//dimiourgia prwtou komvou
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);	//arxizoume ti dimiourgia diergasiwn
		exit(1);
	}

    sleep(SLEEP_TREE_SEC);	//parousiasi dentrou diergasiwn
	show_pstree(pid);	

	pid = wait(&status);	//perimeno ton prwto komvo
	explain_wait_status(pid, status);

	return 0;
}

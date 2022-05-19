#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3


void fork_procs(struct tree_node *root)
{	
	printf("PROCESS %s CREATED WITH PID = %ld\n",root->name,(long)getpid());
	change_pname(root->name); //Ονομασία κόμβου
	int i;
	for (i=0; i < root->nr_children; i++){ //Επανέλαβε για όλα τα παιδιά του κόμβου
		pid_t child = fork();// Δημιουργία i παιδιού
		if (child < 0) {
			perror("main: fork");
			exit(1);
		}
		if (child == 0) {//Αναδρομή για κάθε παιδί του κόμβου
			fork_procs(root->children+i);
			exit(1);
		}
		printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)child);
	}
	sleep(SLEEP_PROC_SEC);
	wait_for_dead_children(root->nr_children);//Αναμονή για τα τον τερματισμό όλων των παιδιών του κόμβου
	
	exit(1);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 */

int main(int argc, char *argv[])
{

	struct tree_node *root;
	int status;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);	
	print_tree(root);
	
	if (root==NULL)
		exit(1);
	
	pid_t pid = fork();	//Δημιουργία της ρίζας του δέντρου
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);
		exit(1);
	}


	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "tree.h"
#include "proc-common.h"



void fork_procs(struct tree_node *root)
{

	printf("PROCESS %s CREATED WITH PID = %ld\n",root->name,(long)getpid());
	change_pname(root->name);	//Ονομασία διεργασίας
	pid_t* pidOfChilds = malloc(sizeof(pid_t)*(root->nr_children));
	int i;
	for (i=0; i < root->nr_children; i++){
		pid_t child = fork();	//Δημιουργία παιδιού
		if (child < 0) {
			perror("main: fork");
			exit(1);
		}
		if (child == 0) {
			fork_procs(root->children+i);	//Αναδρομή για κάθε παιδί
			exit(1);
		}
		printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)child);
		pidOfChilds[i]=child;	//Αποθήκευση του PID κάθε παιδιού 
	}

	wait_for_ready_children(root->nr_children); //Αναμονή μέχρι όλα τα παιδιά να αναστείλουν τη λειτουργία τους
	raise(SIGSTOP);	//Αναστολή λειτουργίας
	printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
	
	
	int status;
	pid_t pid;
	for (i=0; i < root->nr_children; i++){
		kill(pidOfChilds[i],SIGCONT);	//Ξύπνημα παιδιού
		pid=wait(&status);		//Αναμονή μέρχι να τερματιστεί το παιδί
		explain_wait_status(pid, status);
		}
	
	exit(0);
}

/*
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
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
	
	pid_t pid = fork();	//Δημιουργία διεργασίας που είναι η κορυφή του δεντρού
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		printf("ROOT_PROCESS CREATED WITH PID = %ld\n",(long)getpid());
		fork_procs(root);
		exit(1);
	}


	//Αναμονή μέρχι να αναστείλει τη λειτουργία η διεργασία
	wait_for_ready_children(1);


	/* Print the process tree root at pid */
	show_pstree(pid);

	//Ξύπνημα της διεργασίας
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}

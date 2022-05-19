#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */
	int status;

	change_pname("A");					//Ονομασία διεργασίας Α
	printf("A CREATED WITH PID = %ld\n",(long)getpid());
	
	pid_t childB = fork();				//Δημιουργία παιδιού Β. Η συνάρτηση fork επιστρέφει 0 στη διεγασία του παιδιού, το PID του παιδιού στη διεργασία του πατέρα και αρνητικό αριθμό σε περίπτωση σφάλματος. 
	

	if (childB < 0) {					//Λάθος στη δημιουργία του Β
		perror("main: fork");
		exit(1);
	}
	if (childB == 0) { //Το κομμάτι αυτό θα εκτελεστεί από τη διεργασία Β.
		change_pname("B");	//Ονομασία διεργασίας Β	
		printf("B CREATED WITH PID = %ld\n",(long)getpid()); //Δημιουργία παιδιού D
		pid_t childD = fork ();
		if (childD < 0) {	//Σφάλμα στη  fork
			perror("main: fork");
			exit(1);
			}
		if (childD == 0) {//Το κομμάτι αυτό θα εκτελεστεί από τη διεργασία D.
			change_pname("D");
			printf("D CREATED WITH PID = %ld\n",(long)getpid());
			printf("D: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
			exit(13);
		}
		printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)childD);

		printf("B: Sleeping...\n");
		sleep(SLEEP_PROC_SEC);
		childD = wait(&status); //Αναμονή για τερματισμό του D
		explain_wait_status(childD, status);
		printf("B: Exiting...\n");
		exit(19);
	}
	//To κομμάτι αυτό θα εκτελεστεί από τον Α
	printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)childB);
	
	pid_t childC = fork(); //Δημιουργία του C
	if (childC < 0) { //Σφάλμα στη fork
		perror("main: fork");
		exit(1);
	}
	if (childC == 0) {
		change_pname("C"); //Ονομασία του C
		printf("C CREATED WITH PID = %ld\n",(long)getpid());
		printf("C: Sleeping...\n");
		sleep(SLEEP_PROC_SEC);
		printf("C: Exiting...\n");
		exit(17);
		}
	printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)childC);
	
	printf("A: Sleeping...\n");
	sleep(SLEEP_PROC_SEC);
	
	//Aναμονή για τερματισμό των παιδιών
	childC = wait(&status); 
	explain_wait_status(childC, status); 
	childB = wait(&status);	
	explain_wait_status(childB, status);

	printf("A: Exiting...\n");
	exit(16);
}


int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork(); //Δημιουργία του Α
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) { //Αυτό θα εκτελεστεί από την Α
		/* Child */
		fork_procs();
		exit(1);
	}


	/* waits for the process tree to be completely created*/
	sleep(SLEEP_TREE_SEC);

	/* takes a photo of the tree*/
	show_pstree(pid);

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}

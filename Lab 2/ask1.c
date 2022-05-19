#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/wait.h>
#include "proc-common.h"

#define SLEEP_PROC_SEC 5
#define SLEEP_TREE_SEC  2

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */

//process C
void fork_procsC(void) {
    change_pname("C");
    sleep(SLEEP_PROC_SEC);
    printf("C: Exiting...\n");
    exit(17);
}

//process D
void fork_procsD(void) {
    change_pname("D");
    sleep(SLEEP_PROC_SEC);
    printf("D: Exiting...\n");
    exit(13);
}

//process B
void fork_procsB(void) {
    change_pname("B");
    pid_tpid;
    int status;

    pid = fork();
    if (pid< 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid == 0) {
        fork_procsD();
        exit(1);
    }
    
    pid = wait(&status);
    explain_wait_status(pid, status);
    printf("B: Exiting...\n");
    exit(19);
}

void fork_procs(void) {
    change_pname("A");
    pid_t pid1,pid2;
    int status1,status2;

    pid1 = fork();
    if (pid1 < 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid1 == 0) {
        fork_procsB();
        exit(1);
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid2 == 0) {
        fork_procsC();
        exit(1);
    }

    pid1 = wait(&status1);
    explain_wait_status(pid1, status1);

    pid2 = wait(&status2);
    explain_wait_status(pid2, status2);

    printf("A: Exiting...\n");
    exit(16);
}

int main(void) {
    pid_tpid;
    int status;
    
    pid = fork();
    if (pid< 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid == 0) {
        /* Child */
        fork_procs();
        exit(1);
    }

    sleep(SLEEP_TREE_SEC);

    /* Print the process tree root at pid */
    show_pstree(pid);

    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    return 0;
}

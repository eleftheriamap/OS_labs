#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "proc-common.h"
#include "request.h"
/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

typedef struct PCBstruct {            //structure of the processes
  int id;
  int pid;
  char name[TASK_NAME_SZ];
  struct PCBstruct *next;
} pcb_t;

int process;                            //how many processes i have
pcb_t *current,*previous;               //pointers to the list

/*
 * SIGALRM handler
*/
static void sigalrm_handler(int signum)
{
  if (kill(current->pid, SIGSTOP) < 0) {  //kill current process with     sigstop
    perror("SIGSTOP"); exit(1);     //check for errors
  }
  printf("Stopping procress with ID = %d \n",current->id);
}

/*
* SIGCHLD handler
*/
static void sigchld_handler(int signum)
{
  for (;;) {
    int p,status;
    p = waitpid(-1, &status, WUNTRACED | WNOHANG);
    //whohang => return immediately if no child has exited
    //wuntraced =>return if a child has stopped
    //waitpid returns the process id of the child whose state has changed
    if (p < 0) {
      perror("waitpid");
      exit(1);
     }
    if (p == 0) {break}; //has not changed state yet
    // explain_wait_status(p, status);
    if (WIFEXITED(status) || WIFSIGNALED(status)) {   /* A child has died     */
      alarm(0); // restart alarm to 0
      printf("Died process with ID = %d.\n",current->id);
      previous->next=current->next; // delete current node
      free(current);
      process --;     //one less process
      if (process != 0){          //i still have processes
        current = previous -> next;     //get next one
        kill(current->pid,SIGCONT);     //start process
        printf("Starting process with ID = %d.\n",current->id);
        if(process !=1) {alarm(SCHED_TQ_SEC);}    // unless a single process is left initiate new alarm
      }
      else{       // case : no more processes left to execute
        printf("No more processes left.\n");
        exit(0);
      }
    }

    if (WIFSTOPPED(status)) {    /* A child has stopped due to SIGSTOP/SIGTSTP(alarm went off), etc */
      printf("Stopped procress with ID = %d \n",current->id);
      previous = current;         // the next one
      current = current->next;
      kill(current->pid,SIGCONT); //start it
      printf("Starting process with ID = %d.\n",current->id);
      alarm(SCHED_TQ_SEC); //restart alarm 
    }
  }
}

/* Install two signal handlers.
* One for SIGCHLD, one for SIGALRM.
* Make sure both signals are masked when one of them is running.
*/
static void install_signal_handlers(void)
{
  sigset_t sigset;
  struct sigaction sa;
  
  sa.sa_handler = sigchld_handler;        //sa_handler -> pointer to a signal-catching function
  sa.sa_flags = SA_RESTART;               //flags affect the signals behavior. if set the function can be interrupted by the signal and the function restarts.
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGCHLD);
  sigaddset(&sigset, SIGALRM);
  sa.sa_mask = sigset;                    //additional set of signals are blocked during execution of signal-catching function
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction: sigchld");
    exit(1);
  }

  sa.sa_handler = sigalrm_handler;
  if (sigaction(SIGALRM, &sa, NULL) < 0) {
    perror("sigaction: sigalrm");
    exit(1);
  }

  /*
  * Ignore SIGPIPE, so that write()s to pipes
  * with no reader do not result in us being killed,
  * and write() returns EPIPE instead.
  */
  if (signal(SIGPIPE, SIG_IGN) < 0) {
    perror("signal: sigpipe");
    exit(1);
  }
}

void child(char *process_name)  {
  char *newargv[] = {process_name, NULL, NULL, NULL };
  char *newenviron[] = { NULL };
  raise(SIGSTOP);         // stop all

  // replace myself with the process_name, when receives SIGCONT it continues as a "prog"
  execve(process_name, newargv, newenviron);
  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

int main(int argc, char *argv[])
{
  int nproc,i; //nproc=number of given processes
  /*
  * For each of argv[1] to argv[argc - 1],
   * create a new child process, add it to the process list.
  */
  nproc = argc -1; /* number of proccesses goes here */
  pcb_t *pcb = NULL;          //pointer to the list initially is NULL
  
  for(i=0; i < nproc; i++) {
    pid_t pidc;
    pidc = fork();
    if (pidc < 0) { // error
      perror("parent: fork");
      exit(1);
    }
    if (pidc == 0) { // child
      child(argv[i+1]);
      exit(1);
    }
    if (pcb == NULL){
      pcb = malloc(sizeof(pcb_t));
      if (pcb == NULL) {return(1);}
      pcb->id=i;  //give id
      pcb->pid=pidc;      //give pid
      trcpy(pcb->name,argv[i+1]);        //give name
      pcb->next=NULL;                     //next NULL
      current = pcb;
    }
    else {
      current->next = malloc(sizeof(pcb_t));      //make list
      current = current->next;
      current->id=i;
      current->pid=pidc;
      strcpy(current->name,argv[i+1]);
      if(i == (nproc-1))  {current->next = pcb;}
    }
  }
  /* Wait for all children to raise SIGSTOP before exec()ing. */
  wait_for_ready_children(nproc);

  /* Install SIGALRM and SIGCHLD handlers. */
  install_signal_handlers();

  if (nproc == 0) {
    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    exit(1);
  }
  previous = current;
  current = pcb;      //dinxw sti kefali tis listas
  process = nproc;    //poses exw pou perimenoun
  kill(current->pid,SIGCONT); //initiate
  alarm(SCHED_TQ_SEC); //initate alarm

  /* loop forever  until we exit from inside a signal handler. */
  while (pause());
  /* Unreachable */
  fprintf(stderr, "Internal error: Reached unreachable point\n");
  return 1;
}

                                                                          
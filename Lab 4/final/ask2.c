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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

typedef struct PCBstruct {
  int id;
  int pid;
  char name[TASK_NAME_SZ];
  struct PCBstruct *next;
  } pcb_t;

int help=0;
int process,shellpid,idc;
pcb_t *current,*previous,*head;

void child(char *process_name)  {
  char *newargv[] = {process_name, NULL, NULL, NULL };
  char *newenviron[] = { NULL };
  raise(SIGSTOP);         // stop all
  // replace myself with the process_name
  execve(process_name, newargv, newenviron);
  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

/* Print a list of all tasks currently being scheduled.  */
static void sched_print_tasks(void)
{
  pcb_t *i = current;
  printf("\nRUNNING PROCESS : name: %s ,pid: %d ,id: %d\n", i->name, i->pid, i->id);
  while (i->next != current) {
    i = i-> next;
    printf("name: %s ,pid: %d ,id: %d\n", i->name, i->pid, i->id);
  }
  printf("\n");
}

/* Send SIGKILL to a task determined by the value of its
* scheduler-specific id.
*/
static void sched_kill_task_by_id(int id)
{
  pcb_t *i = head;
  if (head->id == id) {
    printf("Killing process with id : %d\n",i->id);
    kill( i->pid, SIGTERM);
  }
  else {
    while (i->next != head){
      i = i-> next;
      if (i->id == id){
        printf("Killing process with id : %d\n",i->id);
        kill( i->pid, SIGTERM);
        return;
      }
    }
  printf("Not found process with id : %d\n", id);
  }
}
 
/* Create a new task.  */
static void sched_create_task(char *executable)
{
  pid_t p;
  pcb_t *i;
 
  p = fork();
  if (p < 0) { // error
    perror("parent: fork");
    exit(1);
  }
  if (p == 0) { // child
    child(executable);
    exit(1);
  }
  help++;
  i = malloc(sizeof(pcb_t));
  i->id=++idc;
  i->pid=p;
  strcpy(i->name,executable);
  process++;
  previous->next = i;
  i -> next = current;
  
  printf("Added %s to the processes list.\n\n", executable);
}

/* Process requests by the shell.  */
static int process_request(struct request_struct *rq)
{
  switch (rq->request_no) {
    case REQ_PRINT_TASKS:
    sched_print_tasks();
    return 0;
    case REQ_KILL_TASK:
      return 0;
    case REQ_EXEC_TASK:
      sched_create_task(rq->exec_task_arg);
      return 0;
    default:
      return -ENOSYS;
  }
}

/*
 * SIGALRM handler
 */
static void sigalrm_handler(int signum)
{
  if (kill(current->pid, SIGSTOP) < 0) {
    perror("SIGSTOP"); exit(1);
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
    if (p < 0) {
      perror("waitpid");
      exit(1);
    }
    if (p == 0) break;
    if(help > 0) {help--; return;}
    // explain_wait_status(p, status);

    if (WIFEXITED(status) || WIFSIGNALED(status)){   /* A child has died */
      alarm(0); // zero alarm
      printf("Died process with ID = %d.\n",current->id);
      if(current==head) {head = current->next;}
      free(current);
      process --;
      if (process != 0){
        current = previous -> next;
        kill(current->pid,SIGCONT);
        printf("Starting process with ID = %d.\n",current->id);
        alarm(SCHED_TQ_SEC); //start alarm
      }
      else{       // case : no more processes left to execute
        printf("No more processes left.\n");
        exit(0);
      }
    }

    if (WIFSTOPPED(status)) {    /* A child has stopped due to SIGSTOP/SIGTSTP, etc */
      printf("Stopped procress with ID = %d \n",current->id);
      previous = current;         // the next one
      current = current->next;
      kill(current->pid,SIGCONT);
      printf("Starting process with ID = %d.\n",current->id);
      alarm(SCHED_TQ_SEC);
    }
  }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void)
{
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
    perror("signals_disable: sigprocmask");
    exit(1);
  }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void signals_enable(void)
{
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
    perror("signals_enable: sigprocmask");
    exit(1);
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
  
  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGCHLD);
  sigaddset(&sigset, SIGALRM);
  sa.sa_mask = sigset;
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

static void do_shell(char *executable, int wfd, int rfd)
{
  char arg1[10], arg2[10];
  char *newargv[] = { executable, NULL, NULL, NULL };
  char *newenviron[] = { NULL };

  sprintf(arg1, "%05d", wfd);
  sprintf(arg2, "%05d", rfd);
  newargv[1] = arg1;
  newargv[2] = arg2;

  raise(SIGSTOP);
  execve(executable, newargv, newenviron);
  
  /* execve() only returns on error */
  perror("scheduler: child: execve");
  exit(1);
}

/* Create a new shell task.
*
* The shell gets special treatment:
* two pipes are created for communication and passed
* as command-line arguments to the executable.
*/
static void sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
  pid_t p;
  int pfds_rq[2], pfds_ret[2];
  if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
    perror("pipe");
    exit(1);
  }
  
  p = fork();
  if (p < 0) {
    perror("scheduler: fork");
    exit(1);
  }
  if (p == 0) {
    /* Child */
    close(pfds_rq[0]);
    close(pfds_ret[1]);
    do_shell(executable, pfds_rq[1], pfds_ret[0]);
    assert(0);
  }
  /* Parent */
  shellpid = p;
  close(pfds_rq[1]);
  close(pfds_ret[0]);
  *request_fd = pfds_rq[0];
  *return_fd = pfds_ret[1];
}

static void shell_request_loop(int request_fd, int return_fd)
{
  int ret;
  struct request_struct rq;
  /*
  * Keep receiving requests from the shell.
   */
   for (;;) {
    if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
      perror("scheduler: read from shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }

    signals_disable();
    ret = process_request(&rq);
    signals_enable();
    
    if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
      perror("scheduler: write to shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }
  }
}

int main(int argc, char *argv[])
{
  int nproc;
  /* Two file descriptors for communication with the shell */
  static int request_fd, return_fd;

  /* Create the shell. */
  sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
  /* TODO: add the shell to the scheduler's tasks */

  /*
  * For each of argv[1] to argv[argc - 1],
  * create a new child process, add it to the process list.
   */
  head = NULL;
  nproc = argc-1; /* number of proccesses goes here */
  int i;
  for( i=0; i < nproc; i++) {
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
    
    if (head == NULL){
      head = malloc(sizeof(pcb_t));
      if (head == NULL) {return(1);}
      head->id=i;
      head->pid=pidc;
      strcpy(head->name,argv[i+1]);
      head->next=NULL;
      current = head;
    }
    else {
      current->next = malloc(sizeof(pcb_t));
      current = current->next;
      current->id=i;
      current->pid=pidc;
      strcpy(current->name,argv[i+1]);
    }
  }
  
  idc = nproc;
  current->next = malloc(sizeof(pcb_t));
  current = current->next;
  current->id=idc;
  current->pid=shellpid;
  strcpy(current->name,SHELL_EXECUTABLE_NAME);
  current->next = head;

  /* Wait for all children to raise SIGSTOP before exec()ing. */
  wait_for_ready_children(nproc+1);
  /* Install SIGALRM and SIGCHLD handlers. */
  install_signal_handlers();
  if (nproc == 0) {
    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    exit(1);
  }

  previous = current;
  current = head;
  process = nproc + 1;
  kill(current->pid,SIGCONT);
  alarm(SCHED_TQ_SEC);

  shell_request_loop(request_fd, return_fd);
  /* Now that the shell is gone, just loop forever
  * until we exit from inside a signal handler.
  */
  while (pause());

  /* Unreachable */
  fprintf(stderr, "Internal error: Reached unreachable point\n");
  return 1;
}

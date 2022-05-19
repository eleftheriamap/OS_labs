  1 #include <errno.h>
  2 #include <unistd.h>
  3 #include <stdlib.h>
  4 #include <stdio.h>
  5 #include <signal.h>
  6 #include <string.h>
  7 #include <assert.h>
  8
  9 #include <sys/wait.h>
 10 #include <sys/types.h>
 11
 12 #include "proc-common.h"
 13 #include "request.h"
 14
 15 /* Compile-time parameters. */
 16 #define SCHED_TQ_SEC 2                /* time quantum */
 17 #define TASK_NAME_SZ 60               /* maximum size for a task's name */
 18
 19 typedef struct PCBstruct {                      //structure of the processes
 20     int id;
 21     int pid;
 22     char name[TASK_NAME_SZ];
 23     struct PCBstruct *next;
 24     } pcb_t;
 25
 26 int process;                            //how many processes i have
 27 pcb_t *current,*previous;               //pointers to the list
 28
 29 /*
 30  * SIGALRM handler
 31  */
 32 static void
 33 sigalrm_handler(int signum)
 34 {
 35         if (kill(current->pid, SIGSTOP) < 0) {  //kill current process with     sigstop
 36                 perror("SIGSTOP"); exit(1);     //check for errors
 37         }
 38     printf("Stopping procress with ID = %d \n",current->id);
 39 }
 40
 41 /*
 42  * SIGCHLD handler
 43  */
 44 static void
 45 sigchld_handler(int signum)
 46 {
 47     for (;;) {
 48         int p,status;
 49             p = waitpid(-1, &status, WUNTRACED | WNOHANG);
 50 //whohang => return immediately if no child has exited
 51 //wuntraced =>return if a child has stopped
 52 //waitpid returns the process id of the child whose state has changed
 53         if (p < 0) {
 54             perror("waitpid");
 55             exit(1);
 56         }
 57         if (p == 0) break; //has not changed state yet
 58
 59        // explain_wait_status(p, status);
 60
 61        if (WIFEXITED(status) || WIFSIGNALED(status)){   /* A child has died     */
 62             alarm(0); // midenizoume ton xrono tou alarm
 63             printf("Died process with ID = %d.\n",current->id);
 64             previous->next=current->next; // delete current node xwris na sp    azw ti lista
 65             free(current);
 66             process --;         //meiwnw arithmo diergasiwn
 67             if (process != 0){          //exw kai alles diergasies
 68                 current = previous -> next;     //dixnw stin epomeni
 69                 kill(current->pid,SIGCONT);     //tin 3ekinw
 70                 printf("Starting process with ID = %d.\n",current->id);
 71                 if(process !=1) alarm(SCHED_TQ_SEC);    // unless a single p    rocess is left initiate new alarm
 72                     }
 73             else{       // case : no more processes left to execute
 74                                 printf("No more processes left.\n");
 75                                 exit(0);
 76                         }
 77         }
 78
 79        if (WIFSTOPPED(status)) {    /* A child has stopped due to SIGSTOP/SI    GTSTP(alarm went off), etc */
 80             printf("Stopped procress with ID = %d \n",current->id);
 81             previous = current;         // the next one
 82                         current = current->next;
 83             kill(current->pid,SIGCONT); //sinexizei i epomeni
 84             printf("Starting process with ID = %d.\n",current->id);
 85             alarm(SCHED_TQ_SEC); //3anarxizw to alarm
 86         }
 87
 88     }
 89 }
 90
 91 /* Install two signal handlers.
 92  * One for SIGCHLD, one for SIGALRM.
 93  * Make sure both signals are masked when one of them is running.
 94  */
 95 static void
 96 install_signal_handlers(void)
 97 {
 98         sigset_t sigset;
 99         struct sigaction sa;
100
101         sa.sa_handler = sigchld_handler;        //sa_handler -> pointer to a     signal-catching function
102         sa.sa_flags = SA_RESTART;               //flags affect the signals b    ehavior. if set the function can be interrupted by the signal and the functi    on restarts.
103         sigemptyset(&sigset);
104         sigaddset(&sigset, SIGCHLD);
105         sigaddset(&sigset, SIGALRM);
106         sa.sa_mask = sigset;                    //additional set of signals     are blocked during execution of signal-catching function
107         if (sigaction(SIGCHLD, &sa, NULL) < 0) {
108                 perror("sigaction: sigchld");
109                 exit(1);
110         }
111
112         sa.sa_handler = sigalrm_handler;
113         if (sigaction(SIGALRM, &sa, NULL) < 0) {
114                 perror("sigaction: sigalrm");
115                 exit(1);
116         }
117
118         /*
119          * Ignore SIGPIPE, so that write()s to pipes
120          * with no reader do not result in us being killed,
121          * and write() returns EPIPE instead.
122          */
123         if (signal(SIGPIPE, SIG_IGN) < 0) {
124                 perror("signal: sigpipe");
125                 exit(1);
126         }
127 }
128
129 void child(char *process_name)  {
130         char *newargv[] = {process_name, NULL, NULL, NULL };
131         char *newenviron[] = { NULL };
132
133         raise(SIGSTOP);         // arxika tis stamatame oles
134
135
136     // replace myself with the process_name, when receives SIGCONT it continues as a "prog"
137         execve(process_name, newargv, newenviron);
138
139         /* execve() only returns on error */
140         perror("execve");
141     exit(1);
142 }
143
144
145
146
147 int main(int argc, char *argv[])
148 {
149         int nproc,i; //nproc=number of given processes
150         /*
151          * For each of argv[1] to argv[argc - 1],
152          * create a new child process, add it to the process list.
153          */
154
155         nproc = argc -1; /* number of proccesses goes here */
156     pcb_t *pcb = NULL;          //pointer to the list arxika NULL
157
158     for(i=0; i < nproc; i++)
159     {
160         pid_t pidc;
161
162
163             pidc = fork();
164             if (pidc < 0) { // error
165                     perror("parent: fork");
166                     exit(1);
167             }
168         if (pidc == 0) { // child
169                     child(argv[i+1]);
170                     exit(1);
171             }
172
173         if (pcb == NULL){
174             pcb = malloc(sizeof(pcb_t));
175             if (pcb == NULL) return(1);
176
177             pcb->id=i;  //dinw id
178             pcb->pid=pidc;      //dinw pid
179             strcpy(pcb->name,argv[i+1]);        //dinw onoma
180             pcb->next=NULL;                     //kanw to next NULL
181             current = pcb;
182         }
183         else {
184             current->next = malloc(sizeof(pcb_t));      //dimiourgw ti lista
185             current = current->next;
186             current->id=i;
187             current->pid=pidc;
188             strcpy(current->name,argv[i+1]);
189             if(i == (nproc-1))  current->next = pcb;
190         }
191     }
192
193         /* Wait for all children to raise SIGSTOP before exec()ing. */
194         wait_for_ready_children(nproc);
195
196         /* Install SIGALRM and SIGCHLD handlers. */
197         install_signal_handlers();
198
199         if (nproc == 0) {
200                 fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
201                 exit(1);
202         }
203     previous = current;
204     current = pcb;      //dinxw sti kefali tis listas
205     process = nproc;    //poses exw pou perimenoun
206     kill(current->pid,SIGCONT); //initiate
207     alarm(SCHED_TQ_SEC); //initate alarm
208
209         /* loop forever  until we exit from inside a signal handler. */
210         while (pause());
211
212         /* Unreachable */
213         fprintf(stderr, "Internal error: Reached unreachable point\n");
214         return 1;
215 }

                                                                          
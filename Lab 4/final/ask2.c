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
 18 #define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */
 19
 20 typedef struct PCBstruct {
 21     int id;
 22     int pid;
 23     char name[TASK_NAME_SZ];
 24     struct PCBstruct *next;
 25     } pcb_t;
 26
 27 int help=0;
 28 int process,shellpid,idc;
 29 pcb_t *current,*previous,*head;
 30
 31
 32
 33 void child(char *process_name)  {
 34         char *newargv[] = {process_name, NULL, NULL, NULL };
 35         char *newenviron[] = { NULL };
 36
 37         raise(SIGSTOP);         // arxika tis stamatame oles
 38
 39
 40     // replace myself with the process_name
 41         execve(process_name, newargv, newenviron);
 42
 43         /* execve() only returns on error */
 44         perror("execve");
 45     exit(1);
 46 }
 47
 48
 49
 50 /* Print a list of all tasks currently being scheduled.  */
 51 static void
 52 sched_print_tasks(void)
 53 {
 54     pcb_t *i = current;
 55     printf("\nRUNNING PROCESS : name: %s ,pid: %d ,id: %d\n", i->name, i->pid, i->id);
 56     while (i->next != current) {
 57         i = i-> next;
 58             printf("name: %s ,pid: %d ,id: %d\n", i->name, i->pid, i->id);
 59     }
 60     printf("\n");
 61 }
 62
 63 /* Send SIGKILL to a task determined by the value of its
 64  * scheduler-specific id.
 65  */
 66 static void
 67 sched_kill_task_by_id(int id)
 68 {
 69      pcb_t *i = head;
 70     if (head->id == id)
 71     {
 72           printf("Killing process with id : %d\n",i->id);
 73           kill( i->pid, SIGTERM);
 74     }
 75     else {
 76         while (i->next != head){
 77             i = i-> next;
 78             if (i->id == id){
 79                 printf("Killing process with id : %d\n",i->id);
 80                 kill( i->pid, SIGTERM);
 81                 return;
 82             }
 83
 84         }
 85
 86      printf("Not found process with id : %d\n", id);
 87     }
 88 }
 89
 90
 91 /* Create a new task.  */
 92 static void
 93 sched_create_task(char *executable)
 94 {
 95         pid_t p;
 96     pcb_t *i;
 97
 98     p = fork();
 99
100     if (p < 0) { // error
101                 perror("parent: fork");
102                 exit(1);
103         }
104     if (p == 0) { // child
105             child(executable);
106             exit(1);
107     }
108      help++;
109      i = malloc(sizeof(pcb_t));
110      i->id=++idc;
111      i->pid=p;
112      strcpy(i->name,executable);
113      process++;
114      previous->next = i;
115      i -> next = current;
116
117     printf("Added %s to the processes list.\n\n", executable);
118 }
119
120 /* Process requests by the shell.  */
121 static int
122 process_request(struct request_struct *rq)
123 {
124         switch (rq->request_no) {
125                 case REQ_PRINT_TASKS:
126                         sched_print_tasks();
127                         return 0;
128
129                 case REQ_KILL_TASK:
131                         return 0;
132
133                 case REQ_EXEC_TASK:
134                         sched_create_task(rq->exec_task_arg);
135                         return 0;
136
137                 default:
138                         return -ENOSYS;
139         }
140 }
141
142 /*
143  * SIGALRM handler
144  */
145 static void
146 sigalrm_handler(int signum)
147 {
148         if (kill(current->pid, SIGSTOP) < 0) {
149                 perror("SIGSTOP"); exit(1);
150         }
151     printf("Stopping procress with ID = %d \n",current->id);
152 }
153
154 /*
155  * SIGCHLD handler
156  */
157 static void
158 sigchld_handler(int signum)
159 {
160     for (;;) {
161         int p,status;
162             p = waitpid(-1, &status, WUNTRACED | WNOHANG);
163         if (p < 0) {
164             perror("waitpid");
165             exit(1);
166         }
167         if (p == 0) break;
168     if(help > 0) {help--; return;}
169        // explain_wait_status(p, status);
170
171        if (WIFEXITED(status) || WIFSIGNALED(status)){   /* A child has died */
172             alarm(0); // midenizoume ton xrono tou alarm
173             printf("Died process with ID = %d.\n",current->id);
175             if(current==head) head = current->next;
176             free(current);
177             process --;
178             if (process != 0){
179                 current = previous -> next;
180                 kill(current->pid,SIGCONT);
181                 printf("Starting process with ID = %d.\n",current->id);
182                 alarm(SCHED_TQ_SEC);
183                 //if(process !=1) alarm(SCHED_TQ_SEC);  // unless a single process is left
184                     }
185             else{       // case : no more processes left to execute
186                                 printf("No more processes left.\n");
187                                 exit(0);
188                         }
189         }
190
191        if (WIFSTOPPED(status)) {    /* A child has stopped due to SIGSTOP/SIGTSTP, etc */
192             printf("Stopped procress with ID = %d \n",current->id);
193             previous = current;         // the next one
194                         current = current->next;
195             kill(current->pid,SIGCONT);
196             printf("Starting process with ID = %d.\n",current->id);
197             alarm(SCHED_TQ_SEC);
198         }
199
200     }
201 }
202
203 /* Disable delivery of SIGALRM and SIGCHLD. */
204 static void
205 signals_disable(void)
206 {
207         sigset_t sigset;
208
209         sigemptyset(&sigset);
210         sigaddset(&sigset, SIGALRM);
211         sigaddset(&sigset, SIGCHLD);
212         if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
213                 perror("signals_disable: sigprocmask");
214                 exit(1);
215         }
216 }
217
218 /* Enable delivery of SIGALRM and SIGCHLD.  */
219 static void
220 signals_enable(void)
221 {
222         sigset_t sigset;
223
224         sigemptyset(&sigset);
225         sigaddset(&sigset, SIGALRM);
226         sigaddset(&sigset, SIGCHLD);
227         if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
228                 perror("signals_enable: sigprocmask");
229                 exit(1);
230         }
231 }
232
233
234 /* Install two signal handlers.
235  * One for SIGCHLD, one for SIGALRM.
236  * Make sure both signals are masked when one of them is running.
237  */
238 static void
239 install_signal_handlers(void)
240 {
241         sigset_t sigset;
242         struct sigaction sa;
243
244         sa.sa_handler = sigchld_handler;
245         sa.sa_flags = SA_RESTART;
246         sigemptyset(&sigset);
247         sigaddset(&sigset, SIGCHLD);
248         sigaddset(&sigset, SIGALRM);
249         sa.sa_mask = sigset;
250         if (sigaction(SIGCHLD, &sa, NULL) < 0) {
251                 perror("sigaction: sigchld");
252                 exit(1);
253         }
254
255         sa.sa_handler = sigalrm_handler;
256         if (sigaction(SIGALRM, &sa, NULL) < 0) {
257                 perror("sigaction: sigalrm");
258                 exit(1);
259         }
260
261         /*
262          * Ignore SIGPIPE, so that write()s to pipes
263          * with no reader do not result in us being killed,
264          * and write() returns EPIPE instead.
265          */
266         if (signal(SIGPIPE, SIG_IGN) < 0) {
267                 perror("signal: sigpipe");
268                 exit(1);
269         }
270 }
271
272 static void
273 do_shell(char *executable, int wfd, int rfd)
274 {
275         char arg1[10], arg2[10];
276         char *newargv[] = { executable, NULL, NULL, NULL };
277         char *newenviron[] = { NULL };
278
279         sprintf(arg1, "%05d", wfd);
280         sprintf(arg2, "%05d", rfd);
281         newargv[1] = arg1;
282         newargv[2] = arg2;
283
284         raise(SIGSTOP);
285         execve(executable, newargv, newenviron);
286
287         /* execve() only returns on error */
288         perror("scheduler: child: execve");
289         exit(1);
290 }
291
292 /* Create a new shell task.
293  *
294  * The shell gets special treatment:
295  * two pipes are created for communication and passed
296  * as command-line arguments to the executable.
297  */
298 static void
299 sched_create_shell(char *executable, int *request_fd, int *return_fd)
300 {
301         pid_t p;
302         int pfds_rq[2], pfds_ret[2];
303
304         if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
305                 perror("pipe");
306                 exit(1);
307         }
308
309         p = fork();
310         if (p < 0) {
311                 perror("scheduler: fork");
312                 exit(1);
313         }
314
315         if (p == 0) {
316                 /* Child */
317                 close(pfds_rq[0]);
318                 close(pfds_ret[1]);
319                 do_shell(executable, pfds_rq[1], pfds_ret[0]);
320                 assert(0);
321         }
322         /* Parent */
323     shellpid = p;
324         close(pfds_rq[1]);
325         close(pfds_ret[0]);
326         *request_fd = pfds_rq[0];
327         *return_fd = pfds_ret[1];
328 }
329
330 static void
331 shell_request_loop(int request_fd, int return_fd)
332 {
333         int ret;
334         struct request_struct rq;
335
336         /*
337          * Keep receiving requests from the shell.
338          */
339         for (;;) {
340                 if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
341                         perror("scheduler: read from shell");
342                         fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
343                         break;
344                 }
345
346                 signals_disable();
347                 ret = process_request(&rq);
348                 signals_enable();
349
350                 if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
351                         perror("scheduler: write to shell");
352                         fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
353                         break;
354                 }
355         }
356 }
357
358
359
360
361
362 int main(int argc, char *argv[])
363 {
364         int nproc;
365         /* Two file descriptors for communication with the shell */
366         static int request_fd, return_fd;
367
368         /* Create the shell. */
369         sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
370         /* TODO: add the shell to the scheduler's tasks */
371
372         /*
373          * For each of argv[1] to argv[argc - 1],
374          * create a new child process, add it to the process list.
375          */
376     head = NULL;
377         nproc = argc-1; /* number of proccesses goes here */
378
379         int i;
380      for( i=0; i < nproc; i++)
381      {
382         pid_t pidc;
383
384
385             pidc = fork();
386             if (pidc < 0) { // error
387                     perror("parent: fork");
388                     exit(1);
389             }
390         if (pidc == 0) { // child
391                     child(argv[i+1]);
392                     exit(1);
393             }
394
395         if (head == NULL){
396             head = malloc(sizeof(pcb_t));
397             if (head == NULL) return(1);
398
399             head->id=i;
400             head->pid=pidc;
401             strcpy(head->name,argv[i+1]);
402             head->next=NULL;
403             current = head;
404         }
405         else {
406             current->next = malloc(sizeof(pcb_t));
407             current = current->next;
408             current->id=i;
409             current->pid=pidc;
410             strcpy(current->name,argv[i+1]);
411         }
412     }
413     idc = nproc;
414     current->next = malloc(sizeof(pcb_t));
415     current = current->next;
416     current->id=idc;
417     current->pid=shellpid;
418     strcpy(current->name,SHELL_EXECUTABLE_NAME);
419     current->next = head;
420
421
422         /* Wait for all children to raise SIGSTOP before exec()ing. */
423         wait_for_ready_children(nproc+1);
424
425         /* Install SIGALRM and SIGCHLD handlers. */
426         install_signal_handlers();
427
428         if (nproc == 0) {
429                 fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
430                 exit(1);
431         }
432
433     previous = current;
434     current = head;
435     process = nproc + 1;
436     kill(current->pid,SIGCONT);
437     alarm(SCHED_TQ_SEC);
438
439         shell_request_loop(request_fd, return_fd);
440
441         /* Now that the shell is gone, just loop forever
442          * until we exit from inside a signal handler.
443          */
444         while (pause());
445
446         /* Unreachable */
447         fprintf(stderr, "Internal error: Reached unreachable point\n");
448         return 1;
449 }

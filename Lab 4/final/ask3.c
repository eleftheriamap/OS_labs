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
 24     int prio;
 25     struct PCBstruct *next;
 26     } pcb_t;
 27 int help =0;
 28 int processl,shellpid,idc,processh;
 29 pcb_t *current,*previous,*headh,*start,headl,*head;
 30
 31
 32
 33
 34 void child(char *process_name)  {
 35         char *newargv[] = {process_name, NULL, NULL, NULL };
 36         char *newenviron[] = { NULL };
 37
 38         raise(SIGSTOP);         // arxika tis stamatame oles
 39
 40
 41     // replace myself with the process_name
 42         execve(process_name, newargv, newenviron);
 43
 44         /* execve() only returns on error */
 45         perror("execve");
 46     exit(1);
 47 }
 48
 49
 50 /* Print a list of all tasks currently being scheduled.  */
 51
 52 static void
 53 sched_print_tasks(void)
 54 {
 55     pcb_t *i = current;
 56     printf("\nRUNNING PROCESS : name: %s ,pid: %d ,id: %d, prio: %d\n", i->name, i->pid, i->id,i->prio);
 57     while (i->next != current) {
 58         i = i-> next;
 59             printf("name: %s ,pid: %d ,id: %d, prio: %d\n", i->name, i->pid, i->id,i->prio);
 60     }
 61     printf("\n");
 62 }
 63
 64 /* Send SIGKILL to a task determined by the value of its
 65  * scheduler-specific id.
 66  */
 67 static void
 68 sched_kill_task_by_id(int id)
 69 {
 70     pcb_t *i = head;
 71     if (head->id == id)
 72     {
 73          printf("Killing process with id : %d\n",i->id);
 74          kill( i->pid, SIGTERM);
 75     }
 76     else {
 77         while (i->next != head){
 78             i = i-> next;
 79             if (i->id == id){
 80                 printf("Killing process with id : %d\n",i->id);
 81                 kill( i->pid, SIGTERM);
 82                 return;
 83             }
 84         }
 85       printf("Not found process with id: %d\n",id);
 86
 87     }
 88 }
 89
 90 /* Create a new task.  */
 91 void print (){
 92     pcb_t *i = head;
 93     printf("%d -> %d",i->id,i->next->id);
 94     i = i->next;
 95     while (i != head) {printf("%d -> %d ",i->id,i->next->id); i = i->next;}
 96     printf("\n");
 97 }
 98
 99 static void
100 sched_create_task(char *executable)
101 {
102         pid_t p;
103     pcb_t *i;
104
105     p = fork();
106
107     if (p < 0) { // error
108                 perror("parent: fork");
109                 exit(1);
110         }
111     help ++;
112     if (p == 0) { // child
113             child(executable);
114             exit(1);
115     }
116
117      i = malloc(sizeof(pcb_t));
118      i->id=++idc;
119      i->pid=p;
120      i->prio =0;
121      strcpy(i->name,executable);
122
123      if(processh == 0){
124         pcb_t *q = head;
125         while(q->next != head) q = q->next;
126         i->next = head;
127         head = i;
128         q->next = head;
129
130     }
131     else if(processl == 0){
132         pcb_t *q = head->next;
133         while (q->next != head) q = q->next;
134         q->next = i;
135         i-> next = head;
136     }
137     else{
138          pcb_t *q = head->next,*j= head;
139          while (q->prio == 1) {j = q; q = q->next;}
140          j->next = i;
141          i->next = q;
142          processl++;
143     }
144
145     printf("Added %s to the processes list.\n\n", executable);
146 }
147
148 static void
149 sched_low(int id)
150 {
151    pcb_t *curr = head,*prev = NULL;
152
153     if(head == NULL)
154         return;
155
156     while (curr->id != id)
157     {
158         if (curr->next == head)
159         {
160             printf("\nGiven node is not found"
161                    " in the list!!!");
162             break;
163         }
164         prev = curr;
165         curr = curr->next;
166     }
167     if(curr->prio == 0){ printf("\nGiven node is already LOW prio\n"); return; }
169     if (curr == head) {
170             if (processh == 1){
171                 curr->prio = 0;
172                 processh--;
173                 processl++;
174                 printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
175             }
176             else if (processl != 0){
177                 pcb_t *q = head->next,*i= head,*z=head;
178                 while (q->prio == 1) {i = q; q = q->next;}
179                 while(z->next != head)z = z->next;
180                 i-> next = curr;
181                 head = curr->next;
182                 curr->next = q;
183                 curr->prio = 0;
184                 z->next = head;
185
186                 processh--;
187                 processl++;
188                 printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
189             }
190             else{
191                 pcb_t *q = head;
192                 while (q->next != head) q = q->next;
193                 q->next = curr;
194                 head = curr->next;
195                 curr->next = head;
196                 curr->prio = 0;
197                 processh--;
198                 processl++;
199                 printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
200             }
201     }
202     else if (curr->next == head)
203     {
204         curr->prio = 0;
205         processh--;
206         processl++;
207         printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
208     }
209     else{
210         if(processl != 0){
211             pcb_t *q = head->next,*i= head;
212             while (q->prio == 1) {i = q; q = q->next;}
213             prev->next = curr->next;
214             i-> next = curr;
215             curr->next = q;
216             curr->prio = 0;
217             processh--;
218             processl++;
219             printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
220         }
221         else{
222              pcb_t *q = head;
223              while (q->next != head) q = q->next;
224              q->next = curr;
225              prev->next = curr->next;
226              curr->next = head;
227              curr->prio = 0;
228              processh--;
229              processl++;
230              printf("Process with ID  %d  to LOW priority.\n\n", curr->id);
231             }
232
233     }
234     //  print();
235 }
236
237 static void
238 sched_high(int id)
239 {
240     pcb_t *curr = head, *prev = NULL;
241
242     if(head == NULL)
243         return;
244
245     while (curr->id != id)
246     {
247         if (curr->next == head)
248         {
249             printf("\nGiven node is not found"
250                    " in the list!!!");
251             return;
252         }
253         prev = curr;
254         curr = curr->next;
255     }
256     if(curr->prio == 1) { printf("\nGiven node is already HIGH prio\n"); return; }
257
258     if (curr == head) {
259         curr->prio =1;
260         processh++;
261         processl--;
262         printf("Process with ID  %d  to HIGH priority.\n\n", curr->id);
263     }
264     else if (curr->next == head)
265     {
266         curr->prio =1;
267         processh++;
268         processl--;
269         printf("Process with ID  %d  to HIGH priority.\n\n", curr->id);
270         head = curr;
271         prev->next = head;
272     }
273     else{
274         printf("Process with ID  %d  to HIGH priority.\n\n", curr->id);
275         curr->prio =1;
276         processh++;
277         processl--;
278         prev->next = curr->next;
279         prev = head;
280         while (prev->next != head) prev = prev->next;
281         curr->next = head;
282         prev->next = curr;
283         head = curr;
284
285     }
286     // print();
287 }
288
289
290
291
292 /* Process requests by the shell.  */
293 static int
294 process_request(struct request_struct *rq)
295 {
296         switch (rq->request_no) {
297                 case REQ_PRINT_TASKS:
298                         sched_print_tasks();
299                         return 0;
300
301                 case REQ_KILL_TASK:
302             sched_kill_task_by_id(rq->task_arg);
303                         return 0;
304
305                 case REQ_EXEC_TASK:
306                         sched_create_task(rq->exec_task_arg);
307                         return 0;
308
309         case REQ_HIGH_TASK:
310                         sched_high(rq->task_arg);
311                         return 0;
312
313                 case REQ_LOW_TASK:
314                         sched_low(rq->task_arg);
315                         return 0;
316
317                 default:
318                         return -ENOSYS;
319         }
320 }
321
322 /*
323  * SIGALRM handler
324  */
325 static void
326 sigalrm_handler(int signum)
327 {
328         if (kill(current->pid, SIGSTOP) < 0) {
329                 perror("SIGSTOP"); exit(1);
330         }
331     printf("Stopping procress with ID = %d \n",current->id);
332 }
333
334 /*
335  * SIGCHLD handler
336  */
337 static void
338 sigchld_handler(int signum)
339 {
340     for (;;) {
341         int p,status;
342             p = waitpid(-1, &status, WUNTRACED | WNOHANG);
343         if (p < 0) {
344             perror("waitpid");
345             exit(1);
346         }
347         if (p == 0) break;
348
349        // explain_wait_status(p, status);
350     if(help > 0) {help--; return;}
351        if (WIFEXITED(status) || WIFSIGNALED(status)){   /* A child has died */
352             alarm(0); // midenizoume ton xrono tou alarm
353             printf("Died process with ID = %d.\n",current->id);
354             if(current == head){
355                 pcb_t *q = head;
356                 while(q->next != head) q= q->next;
357                 head = current->next;
358                 q->next = head;
359             }
360             else previous->next=current->next; // delete current node
361             if(current->prio == 1) processh --;
362             else    processl --;
363
364
365             if (current->next->prio == 1){
366                 free(current);
367                 current = current -> next;
368                 kill(current->pid,SIGCONT);
369                 printf("Starting process with ID = %d.\n",current->id);
370                 alarm(SCHED_TQ_SEC);
371                // if(processh !=1) alarm(SCHED_TQ_SEC); // unless a single process is left
372                     }
373             else if ((processh != 0) && (previous->next->prio == 0)){
374                 free(current);
375                 current = head;
376                 kill(current->pid,SIGCONT);
377                 printf("Starting process with ID = %d.\n",current->id);
378                 alarm(SCHED_TQ_SEC);
379                // if(processh !=1) alarm(SCHED_TQ_SEC);
380             }
381             else if (processl != 0){
382                 free(current);
383                 current = current -> next;
384                 kill(current->pid,SIGCONT);
385                 printf("Starting process with ID = %d.\n",current->id);
386                 alarm(SCHED_TQ_SEC);
387                // if(processl !=1) alarm(SCHED_TQ_SEC); // unless a single process is left
388                     }
389
390             else{       // case : no more processes left to execute
391                                 printf("No more processes left.\n");
392                                 exit(0);
393                         }
394         }
395
396        if (WIFSTOPPED(status)) {    /* A child has stopped due to SIGSTOP/SIGTSTP, etc */
397         if(processh == 0){
398             printf("Stopped procress with ID = %d \n",current->id);
399             previous = current;         // the next one
400                         current = current->next;
401             kill(current->pid,SIGCONT);
402             printf("Starting process with ID = %d.\n",current->id);
403             alarm(SCHED_TQ_SEC);
404         }
405         else {
406             printf("Stopped procress with ID = %d \n",current->id);
407             if (current->next->prio == 1){
408                 printf("Stopped procress with ID = %d \n",current->id);
409                 previous = current;             // the next one
410                             current = current->next;
411                 kill(current->pid,SIGCONT);
412                 printf("Starting process with ID = %d.\n",current->id);
413                 alarm(SCHED_TQ_SEC);
414             }
415             else{
416                 printf("Stopped procress with ID = %d \n",current->id);
417                 previous = current;             // the next one
418                             current = head;
419                 kill(current->pid,SIGCONT);
420                 printf("Starting process with ID = %d.\n",current->id);
421                 alarm(SCHED_TQ_SEC);
422             }
423         }
424
425      }
426     }
427 }
428
429
430
431
432 /* Disable delivery of SIGALRM and SIGCHLD. */
433 static void
434 signals_disable(void)
435 {
436         sigset_t sigset;
437
438         sigemptyset(&sigset);
439         sigaddset(&sigset, SIGALRM);
440         sigaddset(&sigset, SIGCHLD);
441         if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
442                 perror("signals_disable: sigprocmask");
443                 exit(1);
444         }
445 }
446
447 /* Enable delivery of SIGALRM and SIGCHLD.  */
448 static void
449 signals_enable(void)
450 {
451         sigset_t sigset;
452
453         sigemptyset(&sigset);
454         sigaddset(&sigset, SIGALRM);
455         sigaddset(&sigset, SIGCHLD);
456         if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
457                 perror("signals_enable: sigprocmask");
458                 exit(1);
459         }
460 }
461
462
463 /* Install two signal handlers.
464  * One for SIGCHLD, one for SIGALRM.
465  * Make sure both signals are masked when one of them is running.
466  */
467 static void
468 install_signal_handlers(void)
469 {
470         sigset_t sigset;
471         struct sigaction sa;
472
473         sa.sa_handler = sigchld_handler;
474         sa.sa_flags = SA_RESTART;
475         sigemptyset(&sigset);
476         sigaddset(&sigset, SIGCHLD);
477         sigaddset(&sigset, SIGALRM);
478         sa.sa_mask = sigset;
479         if (sigaction(SIGCHLD, &sa, NULL) < 0) {
480                 perror("sigaction: sigchld");
481                 exit(1);
482         }
483
484         sa.sa_handler = sigalrm_handler;
485         if (sigaction(SIGALRM, &sa, NULL) < 0) {
486                 perror("sigaction: sigalrm");
487                 exit(1);
488         }
489
490         /*
491          * Ignore SIGPIPE, so that write()s to pipes
492          * with no reader do not result in us being killed,
493          * and write() returns EPIPE instead.
494          */
495         if (signal(SIGPIPE, SIG_IGN) < 0) {
496                 perror("signal: sigpipe");
497                 exit(1);
499 }
500
501
502 static void
503 do_shell(char *executable, int wfd, int rfd)
504 {
505         char arg1[10], arg2[10];
506         char *newargv[] = { executable, NULL, NULL, NULL };
507         char *newenviron[] = { NULL };
508
509         sprintf(arg1, "%05d", wfd);
510         sprintf(arg2, "%05d", rfd);
511         newargv[1] = arg1;
512         newargv[2] = arg2;
513
514         raise(SIGSTOP);
515         execve(executable, newargv, newenviron);
516
517         /* execve() only returns on error */
518         perror("scheduler: child: execve");
519         exit(1);
520 }
521
522 /* Create a new shell task.
523  *
524  * The shell gets special treatment:
525  * two pipes are created for communication and passed
526  * as command-line arguments to the executable.
527  */
528 static void
529 sched_create_shell(char *executable, int *request_fd, int *return_fd)
530 {
531         pid_t p;
532         int pfds_rq[2], pfds_ret[2];
533
534         if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
535                 perror("pipe");
536                 exit(1);
537         }
538
539         p = fork();
540         if (p < 0) {
541                 perror("scheduler: fork");
542                 exit(1);
543         }
544
545         if (p == 0) {
546                 /* Child */
547                 close(pfds_rq[0]);
548                 close(pfds_ret[1]);
549                 do_shell(executable, pfds_rq[1], pfds_ret[0]);
550                 assert(0);
551         }
552         /* Parent */
553     shellpid = p;
554         close(pfds_rq[1]);
555         close(pfds_ret[0]);
556         *request_fd = pfds_rq[0];
557         *return_fd = pfds_ret[1];
558 }
559
560 static void
561 shell_request_loop(int request_fd, int return_fd)
562 {
563         int ret;
564         struct request_struct rq;
565
566         /*
567          * Keep receiving requests from the shell.
568          */
569         for (;;) {
570                 if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
571                         perror("scheduler: read from shell");
572                         fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
573                         break;
574                 }
575
576                 signals_disable();
577                 ret = process_request(&rq);
578                 signals_enable();
579
580                 if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
581                         perror("scheduler: write to shell");
582                         fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
583                         break;
584                 }
585         }
586 }
587
588
589
590
591
592 int main(int argc, char *argv[])
593 {
594         int nproc;
595         /* Two file descriptors for communication with the shell */
596         static int request_fd, return_fd;
597
598         /* Create the shell. */
599         sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
600         /* TODO: add the shell to the scheduler's tasks */
601
602         /*
603          * For each of argv[1] to argv[argc - 1],
604          * create a new child process, add it to the process list.
605          */
606     head = NULL;
607         nproc = argc-1; /* number of proccesses goes here */
608
609         int i;
610      for( i=0; i < nproc; i++)
611      {
612         pid_t pidc;
613
614
615             pidc = fork();
616             if (pidc < 0) { // error
617                     perror("parent: fork");
618                     exit(1);
619             }
620         if (pidc == 0) { // child
621                     child(argv[i+1]);
622                     exit(1);
623             }
624
625         if (head == NULL){
626             head = malloc(sizeof(pcb_t));
627             if (head == NULL) return(1);
628
629             head->id=i;
630             head->pid=pidc;
631             head->prio=0;
632             strcpy(head->name,argv[i+1]);
633             head->next=NULL;
634             current = head;
635         }
636         else {
637             current->next = malloc(sizeof(pcb_t));
638             current = current->next;
639             current->id=i;
640             current->pid=pidc;
641             current->prio = 0;
642             strcpy(current->name,argv[i+1]);
643         }
644     }
645     idc = nproc;
646     current->next = malloc(sizeof(pcb_t));
647     current = current->next;
648     current->id=idc;
649     current->prio = 0;
650     current->pid=shellpid;
651     strcpy(current->name,SHELL_EXECUTABLE_NAME);
652     current->next = head;
653
654
655         /* Wait for all children to raise SIGSTOP before exec()ing. */
656         wait_for_ready_children(nproc+1);
657
658         /* Install SIGALRM and SIGCHLD handlers. */
659         install_signal_handlers();
660
661         if (nproc == 0) {
662                 fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
663                 exit(1);
664         }
665
666     previous = current;
667     current = head;
668     start = head;
669     processl = nproc + 1;
670     kill(current->pid,SIGCONT);
671     alarm(SCHED_TQ_SEC);
672
673         shell_request_loop(request_fd, return_fd);
674
675         /* Now that the shell is gone, just loop forever
676          * until we exit from inside a signal handler.
677          */
678         while (pause());
679
680         /* Unreachable */
681         fprintf(stderr, "Internal error: Reached unreachable point\n");
682         return 1;
683 }
                                                                                                                                                                                                                                                                                                                                                                                                                                                    
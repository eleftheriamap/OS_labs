/*
* mandel.c
*
* A program to draw the Mandelbrot Set on a 256-color xterm.
*
*/
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include "mandel-lib.h"
#define MANDEL_MAX_ITERATION 100000
#define perror_pthread(ret, msg) 

do { errno = ret; perror(msg); } while (0)
/***************************
* Compile-time parameters *
***************************/
/*
* Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;
sem_t *sem;
int nthreads;

/*
* The part of the complex plane to be drawn:
* upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
* Every character in the final output is
* xstep x ystep units wide on the complex plane.
*/
double xstep;
double ystep;

/*
* This function computes a line of output
* as an array of x_char color values.
*/
int safe_atoi(char *s, int *val) {
    long l;
    char *endp;
    l = strtol(s, &endp, 10);
    if (s != endp && *endp == '\0') {
        *val = l;
        return 0;
    } 
    return -1;
}

void compute_mandel_line(int line, int *color_val[]) {
    /*
    * x and y traverse the complex plane.
    */
    double x, y;
    int n;
    int val;
    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;
    /* and iterate for all points on this line */
    for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {
        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255)
        val = 255;
        /* And store it in the color_val[] array */
        val = xterm_color(val);
        color_val[line][n] = val;
    }
}

/*
* This function outputs an array of x_char color values
* to a 256-color xterm.
*/
void output_mandel_line(int fd, int *color_val[],int line)
{
    int i;
    char point ='@';
    char newline='\n';
    for (i = 0; i < x_chars; i++) {
        /* Set the current color, then output the point */
        set_xterm_color(fd, color_val[line][i]);
        if (write(fd, &point, 1) != 1) {
            perror("compute_and_output_mandel_line: write point");
            exit(1);
        }
    }
    /* Now that the line is done, output a newline character */
    if (write(fd, &newline, 1) != 1) {
        perror("compute_and_output_mandel_line: write newline");
    exit(1);
    }
}

void *thread_start_fn(void *arg)
{
    int line,*x = arg;
    int *color_val[y_chars];
    for (line =0; line < y_chars; line++)
        color_val[line] = (int *)malloc(x_chars*sizeof(int));
    for (line = *x; line < y_chars; line +=nthreads) {
        compute_mandel_line(line, color_val);
    }
    /** If the semaphore's value is greater than zero, then the decrement proceeds,
    * and the function returns, immediately. If the semaphore currently has the value
    * zero, then the call blocks until either it becomes possible to perform the decrement*/
    for (line = *x; line < y_chars; line +=nthreads) {
        sem_wait(&sem[*x]);
        output_mandel_line(1, color_val,line);
        /* increments (unlocks) the semaphore pointed to by sem. */
        if (*x == nthreads-1) sem_post(&sem[0]);
        else sem_post(&sem[*x + 1]);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret,i;
    /*
    * Parse the command line
    */
    Ο
    if (argc != 2){
        fprintf(stderr, "Usage: %s NTHREADS \n\n" "Exactly one argument required:\n" " NTHREADS: The number of threads.\n\n", argv[0]);
        exit(1);
    }
    if (safe_atoi(argv[1], &nthreads) < 0 || nthreads <= 0) {
        fprintf(stderr, "`%s' is not valid for `nthreads'\n", argv[1]);
        exit(1);
    }

    /** Create threads*/
    pthread_t *tid;
    tid = malloc(nthreads*sizeof(pthread_t));
    int *id;
    id = malloc(nthreads*sizeof(int));
    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    /** Create semaphores*/
    sem = malloc(nthreads*sizeof(sem_t));

    /** initializes the unnamed semaphore at the address pointed to by sem.
    * int sem_init(sem_t *sem, int pshared, unsigned int value);
    * pshared = 0, then the semaphore is shared between the threads of a process
    * The value argument specifies the initial value for the semaphore.*/
    sem_init(&sem[0],0,1); // semaphore 0 : allows 1 thread inside
    
    for(i = 1; i<nthreads; i++ )
        sem_init(&sem[i],0,0); // other semaphores allow 0 threads inside
    /* Spawn new thread */
    for (i = 0; i < nthreads; i++) {
        id[i] = i ;
        ret = pthread_create(&tid[i], NULL,thread_start_fn,&id[i]);
        if (ret) {
            perror_pthread(ret, "pthread_create");
            exit(1);
        }
    }
   
    /** Wait for all threads to terminate*/

    for (i = 0; i < nthreads; i++) {
        ret = pthread_join(tid[i], NULL);
        if (ret) {
        perror_pthread(ret, "pthread_join");
        exit(1);
        }
    }
    /*
    * Destroy all semaphores
    */
    for(i = 0; i<nthreads; i++ )
        sem_destroy(&sem[i]);
    return 0;
}
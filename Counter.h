//
// Created by thanos on 22/1/21.
//

#ifndef IT219113_COUNTER_H
#define IT219113_COUNTER_H

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define NTHREADS 12 /*32,64 */ //after some experimenting at three of my systems i came dow to the conclusion that this is the number of threads that is making more sense to create.
#define THREAD_THREASHOLD 3000 //this is when we will not make use of multi thread operations because it is not efficient to do so.

typedef struct t_data data;
struct t_data{
    int tnum,ffd,dfd;
    off_t foff;
    struct dirent *entry;
    pthread_t threads[NTHREADS];
//    long NTHREADS;
} ;

void catcher(int sig);

int isASCII(int ffd, off_t foff);

void * thread_func (void *th);

#endif //IT219113_COUNTER_H

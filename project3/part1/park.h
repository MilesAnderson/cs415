#ifndef PARK_H
#define PARK_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include <semaphore.h>


#define NUM_WORKERS 1

pthread_t *attendants;
pthread_t *cars;

static struct timespec start_time;

static atomic_ulong next_ticket = 0;
static atomic_ulong now_serving = 0;

int n = NUM_WORKERS;
int c = 1;
int p = 1;
int w = 3;
int r = 6;

static sem_t sem_boardQueue;
static sem_t sem_unboardQueue;
static pthread_mutex_t ride_mutex = PTHREAD_MUTEX_INITIALIZER;
static int boardedCount = 0;
static int unboardedCount = 0;


static int elapsed_seconds(void);

void enterTicketLine(void);

void exitTicketLine(void);

void* attendant(void* arg);

void* car(void* arg);


#endif
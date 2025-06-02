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

static int elapsed_seconds(void);

void* ticketBooth(void);

void* attendant(void* arg);

#endif
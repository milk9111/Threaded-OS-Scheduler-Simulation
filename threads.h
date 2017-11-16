
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "fifo_queue.h"
#include <stdio.h>
#include <stdlib.h>


typedef struct MUTEX {
	int isLocked;
	PCB pcb1;
	PCB pcb2;
	PCB hasLock;
	ReadyQueue blocked;
} mutex_s;

typedef mutex_s * Mutex;


void mutex_init (Mutex mutex);

void toStringMutex (Mutex mutex);

#endif
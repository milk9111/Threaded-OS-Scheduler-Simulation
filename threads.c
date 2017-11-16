
#include "threads.h"


void mutex_init (Mutex mutex) {
	mutex = (Mutex) malloc (sizeof (Mutex));
	mutex->isLocked = 0;
	mutex->blocked = q_create();
}


void toStringMutex (Mutex mutex) {
	printf ("Mutex:\r\n");
	printf("isLocked: %d\r\n", mutex->isLocked);
	
	printf("pcb1: ");
	toStringPCB(mutex->pcb1);
	printf("pcb2: ");
	toStringPCB(mutex->pcb2);
	
	printf("blocked: ");
	toStringReadyQueue(mutex->blocked);
}





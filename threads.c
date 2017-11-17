
#include "pcb.h"


Mutex mutex_init () {
	Mutex mutex = (Mutex) malloc (sizeof (Mutex));
	mutex->isLocked = 0;
	
	return mutex;
}


void toStringMutex (Mutex mutex) {
	printf ("Mutex:\r\n");
	printf("isLocked: %d\r\n", mutex->isLocked);
	
	printf("pcb1: ");
	toStringPCB(mutex->pcb1, 0);
	printf("lock pc: %d, unlock pc: %d\r\n\r\n", mutex->pcb1->lock_pc, mutex->pcb1->unlock_pc);
	
	printf("pcb2: ");
	toStringPCB(mutex->pcb2, 0);
	printf("lock pc: %d, unlock pc: %d\r\n\r\n", mutex->pcb2->lock_pc, mutex->pcb2->unlock_pc);
	
	/*printf("blocked: ");
	toStringPCB(mutex->blocked, 0);*/
}

void mutex_destroy(Mutex mutex) {
	free (mutex);
}




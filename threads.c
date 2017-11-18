
#include "pcb.h"


unsigned int global_largest_MID;

/*
	Creates and initializes the value of the mutex.
*/
Mutex mutex_init () {
	Mutex mutex = (Mutex) malloc (sizeof (Mutex));
	mutex->isLocked = 0;
	mutex->hasLock = NULL;
	mutex->blocked = NULL;
	mutex->mid = global_largest_MID;
	global_largest_MID++;
	return mutex;
}


/*
	Locks the given mutex. Sets the hasLocked PCB value to be the PCB given.
	If the mutex is already locked and the calling pcb was the one that locks it, 
	a big printf is displayed to alert the user.
*/
void mutex_lock (Mutex mutex, PCB pcb) {
	if (mutex) {
		if (mutex->isLocked && mutex->hasLock == pcb) { 
			printf("\r\n\r\n\t\tMUTEX IS ALREADY LOCKED!!!!!!!!!!\r\n\r\n");
			//exit(0);
		}
		mutex->isLocked = 1;
		mutex->hasLock = pcb;
	}  else {
		printf("\r\n\r\n\t\tMUTEX IS NULL. LOCK FAILED\r\n\r\n");
	}
}


/*
	Attempts to lock the given mutex with the given PCB. If the lock has already 
	been taken, it will return false. Otherwise it will lock the mutex and return
	true.
*/
int mutex_trylock (Mutex mutex, PCB pcb) {
	int wasLocked = 0;
	
	if (mutex) {
		if (!mutex->isLocked) {
			mutex->isLocked = 1;
			mutex->hasLock = pcb;
		}
	} else {
		printf("\r\n\r\n\t\tMUTEX IS NULL. TRYLOCK FAILED\r\n\r\n");
	}
	
	return wasLocked;
}


/*
	Unlocks the given mutex. Sets the hasLocked PCB value back to NULL.
	If the mutex is already unlocked a big printf is displayed to alert the user.
*/
void mutex_unlock (Mutex mutex, PCB pcb) {
	if (mutex) {
		if (!mutex->isLocked) { 
			printf("\r\n\r\n\t\tMUTEX IS ALREADY UNLOCKED\r\n\r\n");
			//exit(0);
		}
		mutex->isLocked = 0;
		mutex->hasLock = NULL;
	} else {
		printf("\r\n\r\n\t\tMUTEX IS NULL. UNLOCK FAILED\r\n\r\n");
	}
}


/*
	Prints the contents of the mutex.
*/
void toStringMutex (Mutex mutex) {
	printf ("Mutex:\r\n");
	printf("mid: %d, isLocked: %d\r\n", mutex->mid, mutex->isLocked);
	
	printf("pcb1: ");
	toStringPCB(mutex->pcb1, 0);
	printf("lock pc: %d, unlock pc: %d\r\n\r\n", mutex->pcb1->lock_pc, mutex->pcb1->unlock_pc);
	
	printf("pcb2: ");
	toStringPCB(mutex->pcb2, 0);
	printf("lock pc: %d, unlock pc: %d\r\n\r\n", mutex->pcb2->lock_pc, mutex->pcb2->unlock_pc);
	
	/*if (mutex->blocked) {
		printf("blocked: ");
		toStringPCB(mutex->blocked, 0);
	}*/
}


/*
	Destroys the given mutex.
*/
void mutex_destroy(Mutex mutex) {
	if (mutex != NULL) {
		printf("mutex is locked: %d\n", mutex->isLocked);
		printf("trying to free mutex\n");
		if (mutex->pcb1) {
			printf("freeing pcb1\n");
			free(mutex->pcb1);
		}
		
		if (mutex->pcb2) {
			printf("freeing pcb2\n");
			free(mutex->pcb2);
		}
		
		/*if (mutex->hasLock) {
			printf("freeing hasLock\n");
			free(mutex->hasLock);
			printf("freed hasLock\n");
		}*/
		
		if (mutex->blocked) {
			printf("freeing blocked\n");
			free(mutex->blocked);
		}
		free (mutex);
		mutex = NULL;
		printf("freed mutex\n");
	} else {
		printf("mutex was null\n");
	}
}








#include "pcb.h"


unsigned int global_largest_MID;

void printNull2 (Mutex mutex) {
	printf("pcb1 is null: %d\n", (mutex->pcb1 == NULL));
	if (mutex->pcb1 != NULL) {
		printf("pcb1 values\n");
		toStringPCB(mutex->pcb1, 0);
	}
	
	printf("pcb2 is null: %d\n", (mutex->pcb2 == NULL));
	if (mutex->pcb2 != NULL) {
		printf("pcb2 values\n");
		toStringPCB(mutex->pcb2, 0);
	}
	
	printf("hasLock is null: %d\n", (mutex->hasLock == NULL));
	if (mutex->hasLock != NULL) {
		printf("hasLock values\n");
		toStringPCB(mutex->hasLock, 0);
	}
	
	printf("blocked is null: %d\n", (mutex->blocked == NULL));
	if (mutex->blocked != NULL) {
		printf("blocked values\n");
		toStringPCB(mutex->blocked, 0);
	}
}

/*
	Creates and initializes the value of the mutex.
*/
Mutex mutex_create () {
	Mutex mutex = (Mutex) malloc (sizeof (struct MUTEX));
	mutex->isLocked = 0;
	mutex->hasLock = NULL;
	mutex->blocked = NULL;
	mutex->pcb1 = NULL;
	mutex->pcb2 = NULL;
	mutex->mid = global_largest_MID;
	global_largest_MID++;
	return mutex;
}


void mutex_init (Mutex mutex) {
	mutex->isLocked = 0;
	mutex->hasLock = NULL;
	mutex->blocked = NULL;
	mutex->pcb1 = NULL;
	mutex->pcb2 = NULL;
	mutex->mid = global_largest_MID;
	global_largest_MID++;
}



/*
	Locks the given mutex. Sets the hasLocked PCB value to be the PCB given.
	If the mutex is already locked and the calling pcb was the one that locks it, 
	a big printf is displayed to alert the user.
*/
int mutex_lock (Mutex mutex, PCB pcb) {
	if (mutex) {
		if (mutex->isLocked && mutex->hasLock == pcb) { 
			printf("\r\n\r\n\t\tMUTEX IS ALREADY LOCKED!!!!!!!!!!\r\n\r\n");
			//exit(0);
			return 0;
		} else {
			mutex->isLocked = 1;
			mutex->hasLock = pcb;
			return 1;
		}

	}  else {
		printf("\r\n\r\n\t\tMUTEX IS NULL. LOCK FAILED\r\n\r\n");
		return 0;
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
			wasLocked = 1;
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
int mutex_unlock (Mutex mutex, PCB pcb) {
	if (mutex) {
		if (!mutex->isLocked) { 
			printf("\r\n\r\n\t\tMUTEX IS ALREADY UNLOCKED\r\n\r\n");
			//exit(0);
			return 0;
		} else {
			mutex->isLocked = 0;
			mutex->hasLock = NULL;
			return 1;
		}
		
	} else {
		printf("\r\n\r\n\t\tMUTEX IS NULL. UNLOCK FAILED\r\n\r\n");
		return 0;
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
		free (mutex);
		mutex = NULL;
		//printf("freed mutex\n");
	} else {
		printf("mutex was null\n");
	}
}


ConditionVariable cond_var_create () {
	ConditionVariable condVar = (ConditionVariable) malloc (sizeof(struct COND_VAR));
	return condVar;
}


void cond_var_init (ConditionVariable condVar) {
	condVar->signal = 0;
}


void toStringConditionVariable (ConditionVariable condVar) {
	printf("signal: %d\r\n", condVar->signal);
}


void cond_var_destroy (ConditionVariable condVar) {
	free(condVar);
}


void cond_var_signal (ConditionVariable condVar) {
	condVar->signal = 1;
}


int cond_var_wait (ConditionVariable condVar) {
	return condVar->signal;
}






// simulator threads testing file
// For testing purposes only

#include <stdio.h>
#include "pcb.h"

void initialize_pcb_type_test (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2);

void main()
{
	PCB testPCB1 = PCB_create();
	PCB testPCB2 = PCB_create();
	
	Mutex testMutex1 = mutex_create();
	Mutex testMutex2 = mutex_create();
	
	testPCB1->role = SHARED;
	testPCB2->role = SHARED;
	
	initialize_pcb_type_test(testPCB1, 1, testMutex1, testMutex2);
	initialize_pcb_type_test(testPCB2, 0, testMutex1, testMutex2);
	
	int lock_result = mutex_trylock (testMutex1, testPCB1);
	if(lock_result == 1)
	{
		printf("Success: Lock result was: %d, expected: 1\n", lock_result);
	}
	else if (lock_result == 0)
	{
		printf("Fail: Lock result was: %d, expected: 1\n", lock_result);
	}
	else
	{
		printf("try_lock test failed unexpectedly.");
	}
	
	lock_result = mutex_trylock(testMutex1, testPCB2);
	if(lock_result == 1)
	{
		printf("Failure, lock has been erroniously grabbed. Result: %d, expected: 0\n", lock_result);
	}
	else if(lock_result == 0)
	{
		printf("Test passed. Result: %d, expected: 0\n", lock_result);
	}
	else
	{
		printf("try_lock test failed unexpectedly.");
	}
	
	printf("\n=============\n");
	mutex_unlock(testMutex1, testPCB1);
	toStringMutex(testMutex1);
	
	
	printf("\n=============\n");
	mutex_lock(testMutex1, testPCB1);
	toStringMutex(testMutex1);
	
	
	printf("\n=============\n");
	ConditionVariable testCV = cond_var_create();
	
	cond_var_init(testCV);
	printf("testCV initial state: ");
	toStringConditionVariable(testCV);
	
	cond_var_signal(testCV);
	printf("testCV new state: ");
	toStringConditionVariable(testCV);
	
	
}

void initialize_pcb_type_test (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2) {
	int lock = 0, unlock = 0, signal = 0, wait = 0;
  
	
	switch(pcb->role) {
		case COMP:
			if (isFirst) {
				sharedMutexR1->pcb1 = pcb;
				sharedMutexR2->pcb1 = pcb;
			} else {
				sharedMutexR1->pcb2 = pcb;
				sharedMutexR2->pcb2 = pcb;
			}
			break;
		case IO:
			populateIOTraps (pcb, 0); // populates io_1_traps
			populateIOTraps (pcb, 1); // populates io_2_traps
			if (isFirst) {
				sharedMutexR1->pcb1 = pcb;
				sharedMutexR2->pcb1 = pcb;
			} else {
				sharedMutexR1->pcb2 = pcb;
				sharedMutexR2->pcb2 = pcb;
			}
			break;
		case PAIR:
			if (isFirst) {
				if ((rand() % 100) > 49) { //this decides if it's producer or consumer
					pcb->isProducer = 1;
				} else {
					pcb->isConsumer = 1;
				}
				sharedMutexR1->pcb1 = pcb;
				sharedMutexR2->pcb1 = pcb;
			} else {
				if (sharedMutexR1->pcb1->isProducer) { //if the first PCB is producer, the second will be 
					pcb->isConsumer = 1;			   //Consumer, or vice versa
				} else {
					pcb->isProducer = 1;
				}
				sharedMutexR1->pcb2 = pcb;
				sharedMutexR2->pcb2 = pcb;
			}
			pcb->mutex_R1_id = sharedMutexR1->mid;
			pcb->mutex_R2_id = sharedMutexR2->mid;
			break;
		case SHARED:
			if (isFirst) {
				sharedMutexR1->pcb1 = pcb;
				sharedMutexR2->pcb1 = pcb;
			} else {
				sharedMutexR1->pcb2 = pcb;
				sharedMutexR2->pcb2 = pcb;
			}
			pcb->mutex_R1_id = sharedMutexR1->mid;
			pcb->mutex_R2_id = sharedMutexR2->mid;
			break;
	}
}
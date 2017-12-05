// deadlock function test
// NOTE: this will require commenting out the main function
// 		 in scheduler_pthreads.c to compile

#include "scheduler_pthreads.h"
#include <time.h>


void TEST_initialize_pcb_type (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2);
int TEST_makePCBList (Scheduler theScheduler, int deadlock);

void main()
{
	Scheduler testScheduler = schedulerConstructor();
	TEST_makePCBList(testScheduler, 0);
	
	printf("\n=======BEGIN TESTING=======\n");
	printf("Deadlock control test - fresh PCBs, no locked mutexes.\n");
	deadlockMonitor(testScheduler);
	//Mutex curr_test_mutx = 
	testScheduler->running->context->pc = testScheduler->running->lockR1[0];
	
	int result = useMutex(testScheduler);
	printf("Result: %d\n", result);
	
	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);
	printf("\n=================\nBasic locking test - can PCB2 acquire the lock when PCB1 has already locked it?\n");
	
	testScheduler->running->context->pc = testScheduler->running->lockR1[0];
	result = useMutex(testScheduler);
	printf("Result: %d\n", result);
	printSchedulerState(testScheduler);

	testScheduler->running->context->pc = testScheduler->running->unlockR1[0];
	useMutex(testScheduler);

	printf("\n=================\nDeadlock test - PCB1 takes both mutexes\n");
	testScheduler->running->context->pc = testScheduler->running->lockR1[0];
	useMutex(testScheduler);
	testScheduler->running->context->pc = testScheduler->running->lockR2[0];
	useMutex(testScheduler);
	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);

	//testScheduler->running->context->pc = testScheduler->running->lockR1[0];
//	useMutex(testScheduler);
	deadlockMonitor(testScheduler);
	printSchedulerState(testScheduler);
	
	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);
	testScheduler->running->context->pc = testScheduler->running->unlockR1[0];
	useMutex(testScheduler);
	printf("\n=================\nDeadlock test - PBC1 takes Mutex2, PCB2 takes Mutex1\n");
	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);
	//printSchedulerState(testScheduler);
	testScheduler->running->context->pc = testScheduler->running->lockR1[0];
	useMutex(testScheduler);
	deadlockMonitor(testScheduler);

	testScheduler->running->context->pc = testScheduler->running->unlockR1[0];
	useMutex(testScheduler);
	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);

	testScheduler->running->context->pc = testScheduler->running->unlockR2[0];
	useMutex(testScheduler);
	
	printf("\n=================\nDeadlock test - PCB1 takes Mutex1, PCB2 takes Mutex2\n");
	
	testScheduler->running->context->pc = testScheduler->running->lockR1[0];
	useMutex(testScheduler);

	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);
	testScheduler->running->context->pc = testScheduler->running->lockR2[0];
	useMutex(testScheduler);

	pq_enqueue(testScheduler->ready, testScheduler->running);
	dispatcher(testScheduler);
	deadlockMonitor(testScheduler);
	
	
}

void TEST_initialize_pcb_type (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2) {
	int lock = 0, unlock = 0, signal = 0, wait = 0;  
	
	pcb->role = SHARED;
	
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

int TEST_makePCBList (Scheduler theScheduler, int deadlock) {
	printf("inside making new PCBs\n");
	int newPCBCount = 2;
		
	Mutex sharedMutexR1 = mutex_create();
	Mutex sharedMutexR2 = mutex_create();
	printf("made both mutexes\n");
	
	PCB newPCB1 = PCB_create();
	printf("made the first pcb P%d\n", newPCB1->pid);
	PCB newPCB2 = PCB_create();
	printf("made second pcb P%d\n", newPCB2->pid);
	newPCB2->parent = newPCB1->pid;
	
	newPCB1->role = COMP;
	newPCB2->role = COMP;
	TEST_initialize_pcb_type (newPCB1, 1, sharedMutexR1, sharedMutexR2); 
	TEST_initialize_pcb_type (newPCB2, 0, sharedMutexR1, sharedMutexR2); 
	
	incrementRoleCount(newPCB1->role);
	incrementRoleCount(newPCB2->role);
	
	if (newPCB1->role == COMP || newPCB1->role == IO) { //if the role isn't one that uses a mutex, then destroy it.
		printf("Made COMP or IO pair\n");
		free(sharedMutexR1);
		free(sharedMutexR2);
	} else {
		if (newPCB1->role == SHARED) {
			printf("Made Shared Resource pair\n");
			
			if (deadlock) {
				populateMutexTraps1221(newPCB1, newPCB1->max_pc / MAX_DIVIDER);
				populateMutexTraps2112(newPCB2, newPCB2->max_pc / MAX_DIVIDER);
			} else {
				populateMutexTraps1221(newPCB1, newPCB1->max_pc / MAX_DIVIDER);
				populateMutexTraps1221(newPCB2, newPCB2->max_pc / MAX_DIVIDER);
			}
	
			add_to_mutx_map(theScheduler->mutexes, sharedMutexR1, sharedMutexR1->mid);
			add_to_mutx_map(theScheduler->mutexes, sharedMutexR2, sharedMutexR2->mid);
		} else {
			printf("Made Producer/Consumer\n");
			populateProducerConsumerTraps(newPCB1, newPCB1->max_pc / MAX_DIVIDER, newPCB1->isProducer);
			populateProducerConsumerTraps(newPCB2, newPCB2->max_pc / MAX_DIVIDER, newPCB2->isProducer);
			
			int result = add_to_mutx_map(theScheduler->mutexes, sharedMutexR1, sharedMutexR1->mid);
			free(sharedMutexR2);
		}
		
	}
	
		
	/***************************************/
	
	newPCB1->state = STATE_NEW;
	newPCB2->state = STATE_NEW;
	
	q_enqueue(theScheduler->created, newPCB1);
	q_enqueue(theScheduler->created, newPCB2);

	if (newPCBCount) {
		printf("q_is_empty: %d\r\n", q_is_empty(theScheduler->created));
		while (!q_is_empty(theScheduler->created)) {
			PCB nextPCB = q_dequeue(theScheduler->created);
			printf("created queue:\n");
			toStringReadyQueue(theScheduler->created);
			//toStringPCB(nextPCB, 0);
			nextPCB->state = STATE_READY;
			//printf("\r\n");
			printf("enqueuing P%d into MLFQ from makePCBList\n", nextPCB->pid);
			pq_enqueue(theScheduler->ready, nextPCB);
			printf("printing scheduler state from makePCBList\n");
			for (int i = 0; i < NUM_PRIORITIES; i++) {
				printf("Q[%d]: ", i);
				ReadyQueueNode tmp = theScheduler->ready->queues[i]->first_node;
				while (tmp) {
					printf("P%d->", tmp->pcb->pid);
					tmp = tmp->next;
				}
				printf("*\n");
			}
			/*pthread_mutex_lock(&printMutex);
			toStringPriorityQueue(theScheduler->ready);
			pthread_mutex_unlock(&printMutex);*/
			printf("end printing in makePCBList\n");
		}
		//printf("\r\n");
		
		//toStringPriorityQueue(theScheduler->ready);
		if (theScheduler->isNew) {
			printf("Scheduler is empty!\n");
			theScheduler->running = pq_dequeue(theScheduler->ready);
			printf("Dequeuing to run\n");
			toStringPCB(theScheduler->running, 0);
			if (theScheduler->running) {
				theScheduler->running->state = STATE_RUNNING;
			}
			theScheduler->isNew = 0;
		}
	}
	
	return newPCBCount;
}

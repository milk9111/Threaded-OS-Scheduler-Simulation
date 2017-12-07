/*
	10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman, Jasmine Dacones
	
	In this project we will be making an MLFQ scheduling algorithm
	that will take a PriorityQueue of PCBs and run them through our scheduler.
	It will simulate the "running" of the process by incrementing the running PCB's PCB 
	by one on each loop through. It will also incorporate various interrupts to show 
	the effects it has on the scheduling simulator.
	
	This file holds the defined functions declared in the scheduler.h header file.
*/

#include "scheduler_pthreads.h"


unsigned int sysstack;
int switchCalls;

Scheduler thisScheduler;

PCB privileged[4];
int privilege_counter = 0;
int ran_term_num = 0;
int terminated = 0;
int currQuantumSize;
int quantum_tick = 0; // Use for quantum length tracking
int io_timer = 0;
int totalProcesses = 0;
int iteration = 0;
int isIOTrapPos = 0;
int hasBlockedPCB = 0;
int deadlockDetected = 0;
int isFirstRun = 0;


time_t t;

// The global counts of each PCB type, the final count at end of program run 
// should be roughly 50%, 25%, 12.5%, 12.5% respectively.
int compCount;
int ioCount;
int pairCount;
int sharedCount;
int deadlockCount;
int contextSwitchCount;

int incrementPair;

pthread_mutex_t schedulerMutex;
pthread_mutex_t iterationMutex;
pthread_mutex_t randMutex;
pthread_mutex_t printMutex;
pthread_mutex_t totalProcessesMutex;
pthread_mutex_t trapMutex;
pthread_mutex_t interruptMutex;

pthread_cond_t trapCondVar;
pthread_cond_t interruptCondVar;



/*
	Will look through the lockR1 and lockR2 arrays in the given pcb and see if 
	they match the given PC value. If so, then return the array number, 0 otherwise.
	
	(1 for lockR1, 2 for lockR2)
*/
int isLockPC (unsigned int pc, PCB pcb) {
	int isLock = 0;
	for (int i = 0; i < TRAP_COUNT; i++) {
		if (pc == pcb->lockR1[i]) {
			isLock = 1;
			break;
		}
	}
	
	if (pcb->role == SHARED && !isLock) {
		for (int i = 0; i < TRAP_COUNT; i++) {
			if (pc == pcb->lockR2[i]) {
				isLock = 2;
				break;
			}
		}
	}
	
	return isLock;
}


/*
	Will look through the unlockR1 and unlockR2 arrays in the given pcb and see if 
	they match the given PC value. If so, then return the array number, 0 otherwise.
	
	(1 for unlockR1, 2 for unlockR2)
*/
int isUnlockPC (unsigned int pc, PCB pcb) {
	int isUnlock = 0;
	for (int i = 0; i < TRAP_COUNT; i++) {
		if (pc == pcb->unlockR1[i]) {
			isUnlock = 1;
			break;
		}
	}
	
	if (pcb->role == SHARED && !isUnlock) {
		for (int i = 0; i < TRAP_COUNT; i++) {
			if (pc == pcb->unlockR2[i]) {
				isUnlock = 2;
				break;
			}
		}
	}
	
	return isUnlock;
}


/*
	Will look through the signal_cond array in the given pcb and see if 
	they match the given PC value. If so, then return 1, 0 otherwise.
*/
int isSignalPC (unsigned int pc, PCB pcb) {
	int isSignal = 0;
	for (int i = 0; i < TRAP_COUNT; i++) {
		if (pc == pcb->signal_cond[i]) {
			isSignal = 1;
			break;
		}
	}
	
	return isSignal;
}


/*
	Will look through the signal_cond array in the given pcb and see if 
	they match the given PC value. If so, then return 1, 0 otherwise.
*/
int isWaitPC (unsigned int pc, PCB pcb) {
	int isWait = 0;
	for (int i = 0; i < TRAP_COUNT; i++) {
		if (pc == pcb->wait_cond[i]) {
			isWait = 1;
			break;
		}
	}
	
	return isWait;
}


/*
	Will look through the I/O trap arrays in the given pcb and see if 
	they match the given PC value. If so, then return 1, 0 otherwise.
*/
int isTrapPC (unsigned int pc, PCB pcb) {
	int isTrap = 0;
	if (pcb) {
		for (int i = 0; i < TRAP_COUNT; i++) {
			if (pc == pcb->io_1_traps[i]) {
				isTrap = 1;
				break;
			}
		}
		
		if (isTrap) {
			for (int i = 0; i < TRAP_COUNT; i++) {
				if (pc == pcb->io_2_traps[i]) {
					isTrap = 1;
					break;
				}
			}
		}
	}
	
	return isTrap;
}


/*
	This creates the list of new PCBs for the current loop through. It simulates
	the creation of each PCB, the changing of state to new, enqueueing into the
	list of created PCBs, and moving each of those PCBs into the ready queue.
*/
int makePCBList (Scheduler theScheduler) {
	//printf("inside making new PCBs\n");
	int newPCBCount = 2;
		
	Mutex sharedMutexR1 = mutex_create();
	Mutex sharedMutexR2 = mutex_create();
	
	PCB newPCB1 = PCB_create();
	PCB newPCB2 = PCB_create();
	newPCB2->parent = newPCB1->pid;
	
	if (isFirstRun) {
			newPCB1->role = SHARED;
			newPCB2->role = SHARED;
		
			sharedMutexR1->pcb1 = newPCB1;
			sharedMutexR2->pcb1 = newPCB1;
				
			sharedMutexR1->pcb2 = newPCB2;
			sharedMutexR2->pcb2 = newPCB2;

			newPCB1->mutex_R1_id = sharedMutexR1->mid;
			newPCB1->mutex_R2_id = sharedMutexR2->mid;
			
			newPCB2->mutex_R1_id = sharedMutexR1->mid;
			newPCB2->mutex_R2_id = sharedMutexR2->mid;
			isFirstRun = 0;
	} else {
		initialize_pcb_type (newPCB1, 1, sharedMutexR1, sharedMutexR2); 
		initialize_pcb_type (newPCB2, 0, sharedMutexR1, sharedMutexR2); 
	}
	
	incrementRoleCount(newPCB1->role);
	incrementRoleCount(newPCB2->role);
	
	if (newPCB1->role == COMP || newPCB1->role == IO) { //if the role isn't one that uses a mutex, then destroy it.
		printf("Made COMP or IO pair\r\n");
		free(sharedMutexR1);
		free(sharedMutexR2);
	} else {
		if (newPCB1->role == SHARED) {
			printf("Made Shared Resource pair\r\n");
			if (DEADLOCK) {
				pthread_mutex_lock(&randMutex);
					int temp = rand() % DEADLOCK_CHANCE_DOMAIN;
				pthread_mutex_unlock(&randMutex);
				if (temp <= DEADLOCK_CHANCE_PERCENTAGE) {
					populateMutexTraps2112(newPCB1, newPCB1->max_pc / MAX_DIVIDER);
					populateMutexTraps1221(newPCB2, newPCB2->max_pc / MAX_DIVIDER);
				} else {
					populateMutexTraps1221(newPCB1, newPCB1->max_pc / MAX_DIVIDER);
					populateMutexTraps1221(newPCB2, newPCB2->max_pc / MAX_DIVIDER);
				}
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
	
	newPCB1->state = STATE_NEW;
	newPCB2->state = STATE_NEW;
	
	q_enqueue(theScheduler->created, newPCB1);
	q_enqueue(theScheduler->created, newPCB2);

	if (newPCBCount) {
		while (!q_is_empty(theScheduler->created)) {
			PCB nextPCB = q_dequeue(theScheduler->created);
			nextPCB->state = STATE_READY;
			printf("Enqueuing newly created P%d into MLFQ\n", nextPCB->pid);
			pq_enqueue(theScheduler->ready, nextPCB);
		}
		
		if (theScheduler->isNew) {
			//printf("Scheduler is empty!\n");
			theScheduler->running = pq_dequeue(theScheduler->ready);
			
			pthread_mutex_lock(&printMutex);
			printf("Dequeuing to run\n");
			toStringPCB(theScheduler->running, 0);
			pthread_mutex_unlock(&printMutex);
			if (theScheduler->running) {
				theScheduler->running->state = STATE_RUNNING;
			}
			theScheduler->isNew = 0;
		}
	}
	
	return newPCBCount;
}


/*
	Given the roleType of the newly created PCBs, the corresponding global role 
	type count variable will be incremented. Results will be printed at the end of 
	program execution. See documentation in pcb.c chooseRole() function for 
	more details on percentage for each role type count.
*/
void incrementRoleCount (enum pcb_type roleType) {
	switch (roleType) {
		case COMP:
			compCount++;
			break;
		case IO:
			ioCount++;
			break;
		case PAIR:
			pairCount++;
			break;
		case SHARED:
			sharedCount++;
			break;
	}
}


/*
	Marks the running PCB as terminated. This means it will increment its term_count, if the
	term_count is then over its maximum terminate amount, then it will be enqueued into the
	Killed queue which will empty when it reaches its TOTAL_TERMINATED size.
*/
void terminate(Scheduler theScheduler) {
	if(theScheduler->running != NULL && theScheduler->running->terminate > 0 
		&& theScheduler->running->terminate == theScheduler->running->term_count)
	{
		printf("\nMarking P%d for termination...\r\n", theScheduler->running->pid);
		theScheduler->running->state = STATE_HALT;
		theScheduler->interrupted = theScheduler->running;
		printf("...\r\n");
		scheduling(IS_TERMINATING, theScheduler);	
	}
	
}


/*
	This acts as an Interrupt Service Routine, but only for the Timer interrupt.
	It handles changing the running PCB state to Interrupted, moving the running
	PCB to interrupted, saving the PC to the SysStack and calling the scheduler.
*/
void pseudoISR (Scheduler theScheduler, int interruptType) {
	if (theScheduler->running && theScheduler->running->state != STATE_HALT) {
		theScheduler->running->state = STATE_INT;
		theScheduler->interrupted = theScheduler->running;
		theScheduler->running = NULL;
	} else {
		theScheduler->interrupted = NULL; //explicitly set this to NULL so we can be sure
										  //we're handling it correctly
	}
	scheduling(interruptType, theScheduler);
	printf("Exiting ISR\n");
}


/*
	Prints the state of the Scheduler. Mostly this consists of the MLFQ, the next
	highest priority PCB in line, the one that will be run on next iteration, and
	the current list of "privileged PCBs" that will not be terminated.
*/
void printSchedulerState (Scheduler theScheduler) {
	
	printf("\r\nMLFQ State\r\n");
	//pthread_mutex_lock(&iterationMutex);
	printf("iteration: %d\r\n", iteration);
	//pthread_mutex_unlock(&iterationMutex);
	toStringPriorityQueue(theScheduler->ready);
	printf("\r\n");
	
	int index = 0;

	/*printf("blocked size: %d\r\n", theScheduler->blocked->size);
	printf("killed size: %d\r\n", theScheduler->killed->size);
	printf("killedMutexes size: %d\r\n", theScheduler->killedMutexes->size);*/
	printf("blocked: ");
	toStringReadyQueue(theScheduler->blocked);
	printf("killed: ");
	toStringReadyQueue(theScheduler->killed);
	printf("killedMutexes: ");
	toStringReadyQueueMutexes(theScheduler->killedMutexes);
	printf("\r\n");
	
	if (pq_peek(theScheduler->ready) != NULL) {
		printf("Going to be running ");
		if (theScheduler->running) {
			toStringPCB(theScheduler->running, 0);
		} else {
			printf("\r\n\r\n");
		}
		printf("Next highest priority PCB ");
		toStringPCB(pq_peek(theScheduler->ready), 0);
		printf("\r\n\r\n\r\n");
	} else {
		
		if (theScheduler->running != NULL) {
			printf("Going to be running ");
			toStringPCB(theScheduler->running, 0);
		} else {
			printf("\r\n");
		}

		printf("Next highest priority PCB contents: The MLFQ is empty!\r\n");
		printf("\r\n\r\n\r\n");
	}
}


/*
	Used to move every value in the MLFQ back to the highest priority
	ReadyQueue after a predetermined time. It does this by taking the first value
	of each ReadyQueue (after the 0 *highest priority* queue) and setting it to
	be the new last value of the 0 queue.
*/
void resetMLFQ (Scheduler theScheduler) {
	int allEmpty = 1;
	
	printf("\r\n\r\nRESETTING MLFQ\r\n\r\n");
	
	if (!pq_is_empty(theScheduler->ready)) { //if the MLFQ isn't empty, then reset it
		allEmpty = 0;
		for (int i = 1; i < NUM_PRIORITIES; i++) {
			ReadyQueue curr = theScheduler->ready->queues[i];
			if (!q_is_empty(curr)) {
				if (!q_is_empty(theScheduler->ready->queues[0])) {
					theScheduler->ready->queues[0]->last_node->next = curr->first_node;
					theScheduler->ready->queues[0]->last_node = curr->last_node;
					theScheduler->ready->queues[0]->size += curr->size;
					allEmpty = 0;
				} else {
					theScheduler->ready->queues[0]->first_node = curr->first_node;
					theScheduler->ready->queues[0]->last_node = curr->last_node;
					theScheduler->ready->queues[0]->size = curr->size;
				}
				resetReadyQueue(curr);
			}
		}
	}
	
	if (allEmpty) {
		theScheduler->isNew = 1;
	}
}


/*
	Resets the given ReadyQueue to be empty.
*/
void resetReadyQueue (ReadyQueue queue) {
	ReadyQueueNode ptr = queue->first_node;
	while (ptr) {
		ptr->pcb->priority = 0;
		ptr = ptr->next;
	}
	queue->first_node = NULL;
	queue->last_node = NULL;
	queue->size = 0;
}


/*
	If the interrupt that occurs was a Timer interrupt, it will simply set the 
	interrupted PCBs state to Ready and enqueue it into the Ready queue. If it is
	an IO Trap, then it will put the running PCB into the Blocked queue. If it is
	an IO Interrupt, then it will take the top of the Blocked queue and enqueue it
	back into the MLFQ. If it is a termination, then the running PCB will be marked 
	as such and, if its term_count is greater than its maximum terminate amount, will 
	be enqueued into the Killed queue. Then, if the Killed queue is at or above its own 
	TOTAL_TERMINATED size, it will be emptied. It then calls the dispatcher to get the 
	next PCB in the queue.
*/
void scheduling (int interrupt_code, Scheduler theScheduler) {
	//Mutex currMutex;
	int temp = 0, wentIn = 0;
	PCB tmp = NULL;
	if (interrupt_code == IS_TIMER) {
		printf("Entering Timer Interrupt\r\n");
		/*if (!(theScheduler->interrupted)) {
			printf("interrupted is NULL\n");
		} else {
			printf("interrupted is not NULL\n");
		}*/
		
		if (theScheduler->interrupted) {
			wentIn = 1;
			printf("\r\nEnqueueing into priority %d of MLFQ\r\n", (theScheduler->interrupted->priority+1)%NUM_PRIORITIES);
			toStringPCB(theScheduler->interrupted, 0);
			
			theScheduler->interrupted->state = STATE_READY;
			theScheduler->interrupted->priority = (theScheduler->interrupted->priority + 1) % NUM_PRIORITIES;
			tmp = theScheduler->interrupted;
			//printf("setting tmp to P%d that was the interrupted P%d\n", tmp->pid, theScheduler->interrupted->pid);
			pq_enqueue(theScheduler->ready, theScheduler->interrupted);
			if (tmp == NULL) {
				printf("tmp NULL after pq_enqueue!\n");
				exit(0);
			}
			theScheduler->interrupted = NULL;
		} else {
			printf("\r\nEnqueueing into MLFQ\r\n");
			printf("IDLE\r\n");
		}
		printf("Exiting Timer Interrupt\r\n");
	}
	else if (interrupt_code == IS_IO_TRAP)
	{
		// Do I/O trap handling
		printf("Entering IO Trap\r\n");
		/*if (!(theScheduler->interrupted)) {
			printf("interrupted was NULL in IS_IO_TRAP\n");
		}*/
		theScheduler->interrupted->state = STATE_WAIT;
		
		pthread_mutex_lock(&printMutex);
			printf("\r\nEnqueueing into Blocked queue\r\n");
			toStringPCB(theScheduler->interrupted, 0);
		pthread_mutex_unlock(&printMutex);
		
		q_enqueue(theScheduler->blocked, theScheduler->interrupted);
		theScheduler->interrupted = NULL;
		pthread_mutex_lock(&printMutex);
			printSchedulerState(theScheduler);
		pthread_mutex_unlock(&printMutex);
		pthread_mutex_lock(&interruptMutex);
			hasBlockedPCB = 1;
			printf("\nSending signal to ioInterrupt\n\n");
			pthread_cond_signal(&interruptCondVar);
		pthread_mutex_unlock(&interruptMutex);
		printf("Exiting IO Trap\r\n");
	}
	else if (interrupt_code == IS_IO_INTERRUPT)
	{
		printf("Entering IO Interrupt\r\n");
		// Do I/O interrupt handling
		printf("\r\nEnqueueing into MLFQ from Blocked queue\r\n");
		toStringPCB(q_peek(theScheduler->blocked), 0);
		PCB theBlocked = q_dequeue(theScheduler->blocked);
		theBlocked->state = STATE_READY;
		pq_enqueue(theScheduler->ready, theBlocked);
		if (theScheduler->interrupted != NULL)
		{
			theScheduler->running = theScheduler->interrupted;
			theScheduler->running->state = STATE_RUNNING;
		
			sysstack = theScheduler->running->context->pc;
		}
		theScheduler->interrupted = NULL;
		pthread_mutex_lock(&printMutex);
			printSchedulerState(theScheduler);
		pthread_mutex_unlock(&printMutex);
		printf("Exiting IO Interrupt\r\n");
	}
	
	if (theScheduler->interrupted != NULL && theScheduler->interrupted->state == STATE_HALT) {
		//printf("entering handleKilledQueueInsertion\n");
		printf("\nInserting P%d into the Killed queue\n\n", theScheduler->interrupted->pid);
		handleKilledQueueInsertion(theScheduler);
		//printf("finished handleKilledQueueInsertion\n");
	}
	
	//printf("size of killed queue before checking to empty %d\n", theScheduler->killed->size); 
	if (theScheduler->killed->size >= TOTAL_TERMINATED) {
		//printf("entering handleKilledQueueEmptying with killed size of %d\n", theScheduler->killed->size);
		handleKilledQueueEmptying(theScheduler);
	}
	// I/O interrupt does not require putting a new process
	// into the running state, so we ignore this.
	if(interrupt_code != IS_IO_INTERRUPT) 
	{
		//printf("above dispatcher\n");
		dispatcher(theScheduler);
		//printf("finished dispatcher\n");
	}
}


/*
	This simply gets the next ready PCB from the Ready queue and moves it into the
	running state of the Scheduler.
*/
void dispatcher (Scheduler theScheduler) {
	if (pq_peek(theScheduler->ready) != NULL && pq_peek(theScheduler->ready)->state != STATE_HALT) {
		//toStringPriorityQueue(theScheduler->ready);
		theScheduler->running = pq_dequeue(theScheduler->ready);
		
		pthread_mutex_lock(&printMutex);
			printf("\r\nDequeueing to run\r\n");
			toStringPCB(theScheduler->running, 0);
		pthread_mutex_unlock(&printMutex);
		theScheduler->running->state = STATE_RUNNING;
	} else if (theScheduler->running && theScheduler->running->state == STATE_HALT) { 
		printf("\r\nNothing to dequeue for running, MLFQ is empty.\r\n");
		theScheduler->running = NULL; //do this so it doesn't continue to enqueue into the killed list an already enqueued PCB
		theScheduler->interrupted = NULL;
	} else {
		//printf("\n\t\t\tHit new case in dispatcher I wasn't expecting\n");
		theScheduler->running = NULL; //do this so it doesn't continue to enqueue into the killed list an already enqueued PCB
		theScheduler->interrupted = NULL;
	}
}


/*
	This simply sets the running PCB's PC to the value in the SysStack;
*/
void pseudoIRET (Scheduler theScheduler) {
	if (theScheduler->running != NULL) {
		theScheduler->running->context->pc = sysstack;
	}
}


/*
	This will construct the Scheduler, along with its numerous ReadyQueues and
	important PCBs.
*/
Scheduler schedulerConstructor () {
	Scheduler newScheduler = (Scheduler) malloc (sizeof(struct scheduler));
	newScheduler->created = q_create();
	newScheduler->killed = q_create();
	newScheduler->blocked = q_create();
	newScheduler->killedMutexes = q_create();
	newScheduler->mutexes = create_mutx_map();
	newScheduler->ready = pq_create();
	newScheduler->running = NULL;
	newScheduler->interrupted = NULL;
	newScheduler->isNew = 1;
	
	return newScheduler;
}


/*
	This will do the opposite of the constructor with the exception of 
	the interrupted PCB which checks for equivalancy of it and the running
	PCB to see if they are pointing to the same freed process (so the program
	doesn't crash).
*/
void schedulerDeconstructor (Scheduler theScheduler) {
	int remainingProcesses = 0;
	int remainingInCreated = 0;
	int remainingInKilled = 0;
	int remainingMutexesInKilled = 0;
	int remainingInBlocked = 0;
	//printf("\r\n");
	if (theScheduler) {
		
		if (theScheduler->ready) {
			remainingProcesses = countRemainingProcesses(theScheduler->ready);
			//printf("destroying ready\n");
			//toStringPriorityQueue(theScheduler->ready);
			pq_destroy(theScheduler->ready);
		}
		
		if (theScheduler->created) {
			remainingInCreated = countRemainingProcessesInQueue(theScheduler->created);
			//printf("destroying created\n");
			q_destroy(theScheduler->created);
		}		
		
		if (theScheduler->killed) {
			remainingInKilled = countRemainingProcessesInQueue(theScheduler->killed);
			//printf("destroying killed\n");
			q_destroy(theScheduler->killed);
		}
		
		if (theScheduler->blocked) {
			remainingInBlocked = countRemainingProcessesInQueue(theScheduler->blocked);
			//printf("destroying blocked\n");
			q_destroy(theScheduler->blocked);
		}
		
		if (theScheduler->killedMutexes) {
			remainingMutexesInKilled = countRemainingProcessesInMutexQueue(theScheduler->killedMutexes);
			//printf("destroying killedMutexes\n");
			q_destroy_m(theScheduler->killedMutexes);
			//printf("destroyed killedMutexes\n");
		}
		
		if (theScheduler->mutexes) {
			mutex_map_destroy(theScheduler->mutexes);
		}
		
		if (theScheduler->running) {
			//printf("destroying running\n");
			PCB_destroy(theScheduler->running);
		}
		
		if (theScheduler->interrupted && theScheduler->running && theScheduler->interrupted == theScheduler->running) {
			//printf("\ndestroying interrupted\n");	
			
			//don't want to free interrupted because it was previously set to running so when that 
			//PCB is destroyed this one doesn't need to be. 
			theScheduler->interrupted = NULL;  
			//printf("destroyed interrupted\n");
		}
		
		//printf("destroying scheduler\n");
		free (theScheduler);
	}
	
	displayRoleCountResults();
	printf("Number of total iterations in osLoop: %d\r\n", iteration);
	printf("Number of remaining PCBs in MLFQ: %d\r\n", remainingProcesses);
	printf("Number of remaining PCBS in created: %d\r\n", remainingInCreated);
	printf("Number of remaining PCBS in blocked: %d\r\n", remainingInBlocked);
	printf("Number of remaining PCBS in killed: %d\r\n", remainingInKilled);
	printf("Number of remaining Mutexes in killedMutexes: %d\r\n", remainingMutexesInKilled);
	if (deadlockDetected) {
		printf("Deadlock detected in this run!\r\n");
		printf("Deadlock occurence: %d\r\n", deadlockCount);
	} else {
		printf("No deadlock detected in this run!\r\n");
	}
}

/*
	Displays the number of PCBs created for each type.
*/
void displayRoleCountResults() {
	printf("\r\nTOTAL ROLE TYPES: %d\r\n\r\n", (compCount + ioCount + pairCount + sharedCount));
	
	printf("COMP: \t%d\r\n", compCount);
	printf("IO: \t%d\r\n", ioCount);
	printf("PAIR: \t%d\r\n", pairCount);
	printf("SHARED: %d\r\n", sharedCount);
}

int isPrivileged(PCB pcb) {
	if (pcb != NULL) {
		for (int i = 0; i < 4; i++) {
			if (privileged[i] == pcb) {
				return i;
			}	
		}
	}
	
	return 0;	
}


void main () {
	
	// FILE *f;
    // f = freopen("scheduleTrace.txt", "w", stdout);
	
	setvbuf(stdout, NULL, _IONBF, 0);
	srand((unsigned) time(&t));
	
	compCount = 0;
	ioCount = 0;
	pairCount = 0;
	sharedCount = 0;
	contextSwitchCount = 0;
	incrementPair = 0;
	int i = 0;
	
	osLoop();
	
	//printf("finished with osLoop\n");
	pthread_exit(NULL);
}


/*
	This function is our main loop. It creates a Scheduler object and follows the
	steps a normal MLFQ Priority Scheduler would to "run" for a certain length of time,
	check for all interrupt types, then call the ISR, scheduler,
	dispatcher, and eventually an IRET to return to the top of the loop and start
	with the new process.
*/
void osLoop () {
	void *status, *status2, *status3;
	int temp = 0, makeMorePCBs = 0;
	
	totalProcesses = 0;
	Scheduler scheduler = schedulerConstructor ();
	currQuantumSize = 100;
	
	totalProcesses += makePCBList(scheduler);
	printSchedulerState(scheduler);
	
	scheduler->isNew = 0;
	
	pthread_attr_t attr;
	pthread_mutex_init(&schedulerMutex, NULL);
	pthread_mutex_init(&iterationMutex, NULL);
	pthread_mutex_init(&randMutex, NULL);
	pthread_mutex_init(&printMutex, NULL);
	pthread_mutex_init(&totalProcessesMutex, NULL);
	pthread_mutex_init(&trapMutex, NULL);
	pthread_mutex_init(&interruptMutex, NULL);
	
	pthread_cond_init(&trapCondVar, NULL);
	pthread_cond_init(&interruptCondVar, NULL);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	pthread_t threads[3];
	
	int curr = 3;
	
	//pthread_create(&timer, &attr, timerInterrupt, (void *) scheduler);
	for (int i = 0; i < curr; i++) {
		if (i == 0) {
			pthread_create(&threads[i], &attr, timerInterrupt, (void *) scheduler);
		} else if (i == 1) {
			pthread_create(&threads[i], &attr, ioTrap, (void *) scheduler);
		} else {
			pthread_create(&threads[i], &attr, ioInterrupt, (void *) scheduler);
		}
	}
	printf("done making threads\n");
	pthread_attr_destroy(&attr);
	//pthread_create(&iotrap, &attr, ioTrap, (void *) scheduler);
	//pthread_create(&iointerrupt, &attr, ioInterrupt, (void *) scheduler);
	
	int isRunning = 0;
	int isSwitched = 0;
	int tempHolder = 0;
	int trapPos = 0;

	for(;;)
	{		
		pthread_mutex_lock(&schedulerMutex);
			if (scheduler && scheduler->running) {
				isRunning = 1;
			} else {
				isRunning = 0;
			}
		pthread_mutex_unlock(&schedulerMutex);
		
		if (isRunning) {
			pthread_mutex_lock(&schedulerMutex);
				if (scheduler && scheduler->running && (scheduler->running->role == PAIR || scheduler->running->role == SHARED)) {
					//printf("Going to useMutex\n");
					isSwitched = useMutex(scheduler);
					
					
					if (isSwitched) {
						contextSwitchCount++;
						printf("CONTEXT SWITCH COUNT: %d\r\n", contextSwitchCount);
					}
				
					
					if (contextSwitchCount == 2) { // change to 10 later
						//printf("Going into deadlockMonitor\n");
						tempHolder = deadlockMonitor(scheduler);
						if (!deadlockDetected) {
							deadlockDetected = tempHolder;
						}
						contextSwitchCount = 0;				
					}
				}
			pthread_mutex_unlock(&schedulerMutex);
			
			if (!isSwitched) { //if a context switch happened inside of useMutex, then we want to start over
				//printf("above pc increment\n");
				
				pthread_mutex_lock(&schedulerMutex);
				//printf("called lock before pc\n");
					/*pthread_mutex_lock(&trapMutex);
						trapPos = isIOTrapPos;
					pthread_mutex_unlock(&trapMutex);*/
					if (scheduler && scheduler->running && !isIOTrapPos) {
						scheduler->running->context->pc++;
						if (scheduler->running->role == IO 
							&& isTrapPC(scheduler->running->context->pc, scheduler->running)) {
							pthread_mutex_lock(&trapMutex);
								isIOTrapPos = 1;
								pthread_cond_signal(&trapCondVar);
							pthread_mutex_unlock(&trapMutex);
						}
					}
				pthread_mutex_unlock(&schedulerMutex);
				
				//printf("above term_count increment\n");
				pthread_mutex_lock(&schedulerMutex);
					//printf("called lock before term_count\n");
					if (scheduler && scheduler->running != NULL 
							&& scheduler->running->term_count != scheduler->running->terminate)
					{
						if (scheduler->running->context->pc >= scheduler->running->max_pc) {
							scheduler->running->context->pc = 0;
							scheduler->running->term_count++;
						}
					}
				pthread_mutex_unlock(&schedulerMutex);
				
				//printf("above terminate\n");
				pthread_mutex_lock(&schedulerMutex);
					terminate(scheduler);
				pthread_mutex_unlock(&schedulerMutex);
			}
		}
		
		//printf("above iteration increment\n");
		pthread_mutex_lock(&iterationMutex);
			iteration++;			
		pthread_mutex_unlock(&iterationMutex);
		
		
		pthread_mutex_lock(&iterationMutex);
			if(!(iteration % RESET_COUNT)) {
				pthread_mutex_lock(&schedulerMutex); //resets the MLFQ
					resetMLFQ(scheduler);
				pthread_mutex_unlock(&schedulerMutex);
			}
		pthread_mutex_unlock(&iterationMutex);
		
		
		/*pthread_mutex_lock(&iterationMutex);
			itrHolder = iteration;
		pthread_mutex_unlock(&iterationMutex);*/
		
		
		pthread_mutex_lock(&randMutex);
			temp = rand();
		pthread_mutex_unlock(&randMutex);
		
		pthread_mutex_lock(&schedulerMutex);
			if (temp % MAKE_PCB_CHANCE_DOMAIN <= MAKE_PCB_CHANCE_PERCENTAGE) {
				printf("\nMAKING NEW PCBS\r\n");
				totalProcesses += makePCBList (scheduler); //makes new processes
				/*pthread_mutex_lock(&printMutex);
					printSchedulerState(scheduler);
				pthread_mutex_unlock(&printMutex);*/
			}
		pthread_mutex_unlock(&schedulerMutex);
		

		pthread_mutex_lock(&iterationMutex);
			if (iteration >= MAX_ITERATION_TOTAL) {
				printf("\n");
				/*pthread_mutex_lock(&printMutex);
					printSchedulerState(scheduler);
				pthread_mutex_unlock(&printMutex);*/
				//printf("\n");
				/*pthread_mutex_lock(&printMutex);
					toStringMutexMap(scheduler->mutexes);
				pthread_mutex_unlock(&printMutex);*/
				printf("MAX_ITERATION_TOTAL reached in main\r\n");
				pthread_mutex_unlock(&iterationMutex);
				break;
			}
		pthread_mutex_unlock(&iterationMutex);
	}
	pthread_cancel(threads[0]);
	pthread_mutex_lock(&trapMutex);
	if (!isIOTrapPos) {
		pthread_cancel(threads[1]);
	}
	pthread_mutex_unlock(&trapMutex);
	pthread_mutex_lock(&interruptMutex);
	if (!hasBlockedPCB) {
		pthread_cancel(threads[2]);
	}
	pthread_mutex_unlock(&interruptMutex);
	
	for (int i = 0; i < curr; i++) {
		pthread_join(threads[i], &status);
	}
	//pthread_join(timer, &status);
	//pthread_join(iotrap, &status2);
	//pthread_join(iointerrupt, &status3);
	
	//printf("done joining\n");
	pthread_mutex_destroy(&schedulerMutex);
	pthread_mutex_destroy(&iterationMutex);
	pthread_mutex_destroy(&randMutex);
	pthread_mutex_destroy(&printMutex);	
	pthread_mutex_destroy(&totalProcessesMutex);
	pthread_mutex_destroy(&trapMutex);
	pthread_mutex_destroy(&interruptMutex);
	
	pthread_cond_destroy(&trapCondVar);
	pthread_cond_destroy(&interruptCondVar);
	
	
	printSchedulerState(scheduler);
	schedulerDeconstructor(scheduler);
	
	//printf("Completed main, exiting\n");
}


void * ioInterrupt (void * theScheduler) {
	Scheduler scheduler = (Scheduler) theScheduler;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	int isRunning = 0, temp = 0;
	printf("Starting ioInterrupt thread\r\n\n");
	for (;;) {
		if (!isRunning) {
			pthread_mutex_lock(&interruptMutex);
				while (!hasBlockedPCB) {
					printf("Waiting on condition variable in ioInterrupt\r\n");
					pthread_cond_wait(&interruptCondVar, &interruptMutex);
					printf("\nValue found in the blocked queue, waiting for I/O Interrupt!\r\n");
				}
				hasBlockedPCB = 0;
				isRunning = 1;
			pthread_mutex_unlock(&interruptMutex);
		}
		
		pthread_mutex_lock(&randMutex);
			temp = rand() % IO_INT_CHANCE_DOMAIN;
		pthread_mutex_unlock(&randMutex);
		
		if (temp <= IO_INT_CHANCE_PERCENTAGE) {
			pthread_mutex_lock(&schedulerMutex);
				printf("Received I/O\n");
				printf("Starting ISR in ioInterrupt\r\n");
				pseudoISR(scheduler, IS_IO_INTERRUPT);
				printf("Finished ISR in ioInterrupt\r\n");
				if (q_is_empty(scheduler->blocked)) {
					pthread_mutex_lock(&interruptMutex);
						hasBlockedPCB = 0;
						isRunning = 0;
					pthread_mutex_unlock(&interruptMutex);
				}
			pthread_mutex_unlock(&schedulerMutex);
		}
		
		
		pthread_mutex_lock(&iterationMutex);
			if (iteration >= MAX_ITERATION_TOTAL) { 			//this is how we will break out of the loop, same as in main.
				printf("MAX_ITERATION_TOTAL reached in ioTrap\r\n"); //may think about a check here instead
				pthread_mutex_unlock(&iterationMutex);
				break;
			}
		pthread_mutex_unlock(&iterationMutex);
	}
	
	printf("Finished ioInterrupt, exiting\r\n");
	pthread_exit(NULL);
}


void * ioTrap (void * theScheduler) {
	Scheduler scheduler = (Scheduler) theScheduler;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	printf("\nStarting ioTrap thread\r\n\n");
	for (;;) {
		
		pthread_mutex_lock(&trapMutex);
			while (!isIOTrapPos) {
				printf("Waiting on condition variable in ioTrap\r\n");
				pthread_cond_wait(&trapCondVar, &trapMutex);
				printf("Trap position reached, starting I/O Trap\r\n");
			}
		pthread_mutex_unlock(&trapMutex);
		
		pthread_mutex_lock(&schedulerMutex);
			printf("Starting ISR in ioTrap\r\n");
			pseudoISR(scheduler, IS_IO_TRAP);
			printf("Finished ISR in ioTrap\r\n");
		pthread_mutex_unlock(&schedulerMutex);
		
		pthread_mutex_lock(&trapMutex);
			isIOTrapPos = 0;
		pthread_mutex_unlock(&trapMutex);
		
		pthread_mutex_lock(&iterationMutex);
			if (iteration >= MAX_ITERATION_TOTAL) { 			//this is how we will break out of the loop, same as in main.
				printf("MAX_ITERATION_TOTAL reached in ioTrap\r\n"); //may think about a check here instead
				pthread_mutex_unlock(&iterationMutex);
				break;
			}
		pthread_mutex_unlock(&iterationMutex);
	}
	
	printf("Finished ioTrap, exiting\r\n");
	pthread_exit(NULL);
}


/*
	Checks if the global quantum tick is greater than or equal to
	the current quantum size for the running PCB. If so, then reset
	the quantum tick to 0 and return 1 so the pseudoISR can occur.
	If not, increase quantum tick by 1.
*/
void * timerInterrupt(void * theScheduler)
{	
	Scheduler scheduler = (Scheduler) theScheduler;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	unsigned int temp = 0;
	unsigned int *seed;
	
	struct timespec quantum;
	quantum.tv_sec = 0;
	printf("\nStarting timer interrupt\r\n\n");
	for(;;)
	{
		printf("top of timer\n");
		quantum.tv_nsec = currQuantumSize; //this WAS locked by schedulerMutex
		
		nanosleep(&quantum, NULL); //puts the thread to sleep
		//for (int i = 0; i < 100; i++){}
		
		pthread_mutex_lock(&schedulerMutex); //performs context switching as soon as it wakes
			printf("\nTimer waking up\r\n");
			printf("Starting ISR in timerInterrupt\r\n");
			pseudoISR(scheduler, IS_TIMER);
			printf("Finished ISR in timerInterrupt\r\n");
			
			pthread_mutex_lock(&printMutex);
				printSchedulerState(scheduler);
			pthread_mutex_unlock(&printMutex);
			currQuantumSize = getNextQuantumSize(scheduler->ready); //sets the quantum for the sleep amount
		pthread_mutex_unlock(&schedulerMutex);
		
		pthread_mutex_lock(&iterationMutex);
			if (iteration >= MAX_ITERATION_TOTAL) { 			//this is how we will break out of the loop, same as in main.
				printf("MAX_ITERATION_TOTAL reached in timer\r\n"); //may think about a check here instead
				pthread_mutex_unlock(&iterationMutex);
				break;
			}
		pthread_mutex_unlock(&iterationMutex);
		
		printf("bottom of timer\n");
	}
	
	printf("Finished timer, exiting\n");
	pthread_exit(NULL);
}


int countRemainingProcesses(PriorityQueue pq) {
	int remaining = 0;
	while (!pq_is_empty(pq)) {
		PCB getting = pq_dequeue(pq);
		if (!getting) {
			/*toStringPriorityQueue(pq);
			printf("Finished counting by dequeue\n");*/
			break;
		} else {
			PCB_destroy(getting);
			remaining++;
		}
	}
	return remaining;
}


int countRemainingProcessesInQueue(ReadyQueue queue) {
	int remaining = 0;
	while (!q_is_empty(queue)) {
		PCB getting = q_dequeue(queue);
		if (!getting) {
			/*toStringReadyQueue(queue);
			printf("Finished counting by dequeue\n");*/
			break;
		} else {
			PCB_destroy(getting);
			remaining++;
		}
	}
	return remaining;
}


int countRemainingProcessesInMutexQueue(ReadyQueue queue) {
	int remaining = 0;
	while (!q_is_empty(queue)) {
		Mutex getting = q_dequeue_m(queue);
		if (!getting) {
			/*toStringReadyQueueMutexes(queue);
			printf("Finished counting by dequeue\n");*/
			break;
		} else {
			mutex_destroy(getting);
			remaining++;
		}
	}
	return remaining;
}



/*
	If the Running PCB is of role PAIR or SHARED, then this function is called. What this
	does is handle the locking, unlocking, signalling, and waiting for the Mutexes and 
	their ConditionVariables. If it's determined to be a lock or unlock operation, it will 
	return a 1 or 2 from the respective isLockPC or isUnlockPC saying which of the two SHARED 
	resource the PC is found within, so we know which sharedMutex to look for in the MutexMap.
	
	If a Mutex tries to lock, but the Mutex is already locked, it will simply stall the Mutex.
	Meaning, it will simply perform a simple context switch on the running PCB and put it back 
	into the MLFQ and set the running to the MLFQ dequeue. This happens for lock and wait.
	
	Returns 1 if a context switched happen so the osLoop knows to start over, otherwise 0.
*/
int useMutex (Scheduler thisScheduler) {	
	int wait = 0, signal = 0; 
	int lock = isLockPC(thisScheduler->running->context->pc, thisScheduler->running);
	int unlock = isUnlockPC(thisScheduler->running->context->pc, thisScheduler->running);
	
	
	if (thisScheduler->running->role == PAIR) {
		if (thisScheduler->running->isProducer) {
			signal = isSignalPC(thisScheduler->running->context->pc, thisScheduler->running);
		} else {
			wait = isWaitPC(thisScheduler->running->context->pc, thisScheduler->running);
		}
	}
	
	Mutex currMutex = NULL;
	
	//printf("lock = %d\nunlock = %d\nsignal = %d\nwait = %d\n", lock, unlock, signal, wait);
	
	if (lock) {
		printf("lock values for R1 of P%d:\n", thisScheduler->running->pid);
		printPCLocations(thisScheduler->running->lockR1);
		if (thisScheduler->running->role == SHARED) {
			printf("lock values for R2 of P%d:\n", thisScheduler->running->pid);
			printPCLocations(thisScheduler->running->lockR2);
		}
		
		if (lock == 1) { //is mutex_R1_id
			printf("Getting the Mutex for R1\n");
			currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R1_id);
		} else { //is mutex_R2_id
			printf("Getting the Mutex for R2\n");
			currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R2_id);
		}
		
		if (currMutex) {
			int isLocked = mutex_lock (currMutex, thisScheduler->running);
			if (currMutex->hasLock != thisScheduler->running) {
				// printf("M%d already locked, going to wait for unlock\r\n\r\n", currMutex->mid);
				printf("PID%d: requested lock on mutex M%d - blocked by PID%d\r\n", 
					thisScheduler->running->pid, currMutex->mid, currMutex->hasLock->pid);
				pq_enqueue(thisScheduler->ready, thisScheduler->running);
				thisScheduler->running = pq_dequeue(thisScheduler->ready);
				return 1;
			} else {
				printf("PID%d: requested lock on mutex M%d - succeeded\r\n", 
					thisScheduler->running->pid, currMutex->mid);
				// printf("M%d locked at PC %d\n", currMutex->mid, thisScheduler->running->context->pc);
			}
		} else {
			toStringMutexMap(thisScheduler->mutexes);
			printf("\r\n\t\t\tcurrMutex was null!!!\r\n\r\n");
			exit(0);
		}
		printf("\n");
	} else if (unlock) {
		printf("unlock values for R1 of P%d:\n", thisScheduler->running->pid);
		printPCLocations(thisScheduler->running->unlockR1);
		if (thisScheduler->running->role == SHARED) {
			printf("unlock values for R2 of P%d:\n", thisScheduler->running->pid);
			printPCLocations(thisScheduler->running->unlockR2);	
		}
		
		if (unlock == 1) { //is mutex_R1_id
			printf("Getting the Mutex for R1\n");
			currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R1_id);
		} else { //is mutex_R2_id
			printf("Getting the Mutex for R2\n");
			currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R2_id);
		}
		
		if (currMutex) {
			printf("Trying to use Mutex\n");
			int result = mutex_unlock (currMutex, thisScheduler->running);
			if(result == 1)
			{
				printf("M%d unlocked at PC %d\n", currMutex->mid, thisScheduler->running->context->pc);
			} 
			else if (result == 2)
			{
				printf("Unlock failed, M%d is already owned and locked by P%d!\n", currMutex->mid, currMutex->hasLock->pid);
			}
		} else {
			printf("\r\n\t\t\tcurrMutex was null!!!\r\n\r\n");
			exit(0);
		}
		printf("\n");
	} else if (signal) {
		printf("signal values for R1 of P%d:\n", thisScheduler->running->pid);
		printPCLocations(thisScheduler->running->signal_cond);
		
		currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R1_id);
		
		if (currMutex) {
			cond_var_signal (currMutex->condVar);
			printf("M%d condition variable signalled at PC %d\n", currMutex->mid, thisScheduler->running->context->pc);
			
			//mutex_lock(currMutex, thisScheduler->running);
			incrementPair++;
			printf("Producer %d incremented incrementPair: %d\r\n", thisScheduler->running->pid, incrementPair);
			//mutex_unlock(currMutex, thisScheduler->running);
			
		} else {
			printf("\r\n\t\t\tcurrMutex was null!!!\r\n\r\n");
			exit(0);
		}
		printf("\n");
	} else if (wait) {
		printf("wait values for R1 of P%d:\n", thisScheduler->running->pid);
		printPCLocations(thisScheduler->running->wait_cond);
		
		currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R1_id);
		
		if (currMutex) {
			int isWaiting = cond_var_wait (currMutex->condVar);
			if (isWaiting) {
				pq_enqueue(thisScheduler->ready, thisScheduler->running);
				thisScheduler->running = pq_dequeue(thisScheduler->ready);
				printf("Consumer %d read incrementPair: %d\r\n", thisScheduler->running->pid, incrementPair);
				printf("M%d condition variable waiting at PC %d\n\n", currMutex->mid, thisScheduler->running->context->pc);
				return 1;
			} else { //this part resets the condition variable so we don't need to keep making a new one
				cond_var_init(currMutex->condVar);
			}
		} else {
			printf("\r\n\t\t\tcurrMutex was null!!!\r\n\r\n");
			exit(0);
		}
		printf("\n");
	}
	
	return 0;
}


void handleKilledQueueInsertion (Scheduler theScheduler) {
	Mutex mutex1 = NULL, mutex2 = NULL;
	PCB found = NULL;
	
	if (theScheduler->interrupted->role == PAIR || theScheduler->interrupted->role == SHARED) {
		mutex1 = take_n_remove_from_mutx_map(theScheduler->mutexes, theScheduler->interrupted->mutex_R1_id);
		if (theScheduler->interrupted->role == SHARED) {
			mutex2 = take_n_remove_from_mutx_map(theScheduler->mutexes, theScheduler->interrupted->mutex_R2_id);
		}
		if (!mutex1) {
			toStringMutexMap(theScheduler->mutexes);
			printf("\r\n\t\t\tmutex1 was null! Tried to find M%d but it wasn't in the map!!!\r\n\r\n", theScheduler->running->mutex_R1_id);
			exit(0);
		}
		
		if (theScheduler->interrupted->role == SHARED && !mutex2) {
			toStringMutexMap(theScheduler->mutexes);
			printf("\r\n\t\t\tmutex2 was null! Tried to find M%d but it wasn't in the map!!!\r\n\r\n", theScheduler->running->mutex_R2_id);
			exit(0);
		}
		
		if (theScheduler->interrupted->role == SHARED) { //if the role is SHARED then I want to check if mutex2 is NULL
			if (mutex1 && mutex2 && mutex1->pcb2 == theScheduler->interrupted) { //if interrupted is the pcb2 in the mutex, find the matching pcb1
				printf("looking for pcb1\n");
				found = pq_remove_matching_pcb(theScheduler->ready, mutex1->pcb1);
			} else { //otherwise the interrupted is pcb1, so find pcb2
				printf("looking for pcb2\n");
				found = pq_remove_matching_pcb(theScheduler->ready, mutex1->pcb2);
			}
		} else { //if the role is PAIR then I don't want to check if mutex2 is NULL because it will always be NULL
			if (mutex1 && mutex1->pcb2 == theScheduler->interrupted) {
				found = pq_remove_matching_pcb(theScheduler->ready, mutex1->pcb1);
			} else { 
				found = pq_remove_matching_pcb(theScheduler->ready, mutex1->pcb2);
			}
		}
		
		q_enqueue(theScheduler->killed, theScheduler->interrupted);
		if (found && !q_contains(theScheduler->killed, found)) { //if found is null then the partnering pcb is already in the killed queue
			q_enqueue(theScheduler->killed, found);
		}
		
		q_enqueue_m(theScheduler->killedMutexes, mutex1);
		if (theScheduler->interrupted->role == SHARED 
			&& !q_contains_mutex(theScheduler->killedMutexes, mutex2)) {
			q_enqueue_m(theScheduler->killedMutexes, mutex2);
		}
	} else {
		q_enqueue(theScheduler->killed, theScheduler->interrupted);
	}
	
	/*printf("Killed List: ");
	toStringReadyQueue(theScheduler->killed);
	printf("Mutex List: ");
	toStringReadyQueueMutexes(theScheduler->killedMutexes);*/
	theScheduler->interrupted = NULL;
}


void handleKilledQueueEmptying (Scheduler theScheduler) {
	
	//PCB emptying
	printf("Emptying Killed queue\r\n");
	PCB toKill = NULL;
	if (!(theScheduler->killed)) {
		printf("\r\n\t\t\tkilled PCB queue is NULL!\r\n\r\n");
		exit(0);
	}
	
	/*if (theScheduler->killed && !q_is_empty(theScheduler->killed)) {
		toKill = q_dequeue(theScheduler->killed);
	}*/
	
	if (theScheduler->killed && !q_is_empty(theScheduler->killed)) {
		while(!q_is_empty(theScheduler->killed)) {
			printf("Killed List: ");
			toStringReadyQueue(theScheduler->killed);
			toKill = q_dequeue(theScheduler->killed);
			//if (theScheduler->killed->size > 1) {
				//printf("pid to kill: P%d\n", toKill->pid);
			if (toKill) {
				printf("toKill: P%d\r\n", toKill->pid);
				PCB_destroy(toKill);
			} else {
				printf("\r\n\t\t\ttoKill PCB was NULL while emptying queue!\r\n\r\n");
				exit(0);
			}
				//printf("toKill == null: %d\n", (toKill == NULL));
				//printf("pid after kill: P%d\n\n", toKill->pid);
			//}
			
		} 
	} else {
		
		printf("Something went wrong while emptying Killed queue\n");
		exit(0);
		/*if (toKill) {
			PCB_destroy(toKill);
		} else {
			printf("\r\n\t\t\ttoKill PCB was NULL after trying to kill a single one!\r\n\r\n");
			exit(0);
		}*/
	}
	printf("After emptying\n");
	printf("Killed List: ");
	toStringReadyQueue(theScheduler->killed);
	printf("is killed PCB list empty? ");
	if (q_is_empty(theScheduler->killed)) {
		printf("true\r\n");
	} else {
		printf("false\r\n");
	}
	
	
	//Mutex emptying
	printf("Emptying killed MUTEX list\r\n");
	Mutex toKillMutex = NULL;
	if (!(theScheduler->killedMutexes)) {
		printf("\r\n\t\t\tkilled MUTEX queue is NULL!\r\n\r\n");
		exit(0);
	}
	
	/*if (theScheduler->killedMutexes && !q_is_empty(theScheduler->killedMutexes)) {
		toKillMutex = q_dequeue_m(theScheduler->killedMutexes);
	}*/
	
	if (theScheduler->killedMutexes && !q_is_empty(theScheduler->killedMutexes)) {
		while(!q_is_empty(theScheduler->killedMutexes)) {
			printf("Killed List: ");
			toStringReadyQueueMutexes(theScheduler->killedMutexes);
			toKillMutex = q_dequeue_m(theScheduler->killedMutexes);
			//if (theScheduler->killed->size > 1) {
				//printf("pid to kill: P%d\n", toKill->pid);
			if (toKillMutex) {
				printf("toKillMutex: M%d\r\n", toKillMutex->mid);
				mutex_destroy(toKillMutex);
			} else {
				printf("\r\n\t\t\tin here toKill MUTEX was NULL!\r\n\r\n");
				exit(0);
			}
				//printf("toKill == null: %d\n", (toKill == NULL));
				//printf("pid after kill: P%d\n\n", toKill->pid);
			//}
			
		} 
	} else {
		if (!(theScheduler->killedMutexes)) {
			printf("Something went wrong while emptying Killed Mutex queue\n");
			exit(0);
		} else {
			printf("Killed Mutex queue is empty.\n");
		}
		/*if (toKillMutex) {
			mutex_destroy(toKillMutex);
		} else {
			if (!q_is_empty(theScheduler->killedMutexes)) {
				printf("\r\n\t\t\tor here toKill MUTEX was NULL!\r\n\r\n");
				exit(0);
			}
		}*/
	}
	
	/*printf("After emptying\n");
	printf("Killed List: ");
	toStringReadyQueueMutexes(theScheduler->killedMutexes);
	printf("is killed MUTEX list empty? ");
	if (q_is_empty(theScheduler->killedMutexes)) {
		printf("true\r\n");
	} else {
		printf("false\r\n");
	}*/
		//exit(0);
}

/*
	After n number of context switches, the deadlock monitor is called to detect if
	the currently running PCB and its paired PCB are in a deadlock. If deadlock, we
	will terminate the pair PCBs.  Returrns 1 if a deadlock is detected, otherwise 0.
*/
int deadlockMonitor(Scheduler thisScheduler) {
	int wasFound = 0;
	
	Mutex mutex1;
	Mutex mutex2;
	
	if (thisScheduler->running->role == SHARED) {
		printf("got into SHARED for deadlockMonitor\n");
		mutex1 = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R1_id);
		mutex2 = get_mutx(thisScheduler->mutexes, thisScheduler->running->mutex_R2_id);
		
		
		if (mutex1->pcb1 != NULL && mutex1->pcb2 != NULL && mutex2->pcb1 != NULL && mutex2->pcb2 != NULL) {
			
			
			if (mutex1->isLocked && mutex2->isLocked) {
		
				if (thisScheduler->running == mutex1->pcb1) { // check if pcb1 also owns the other lock
					if (mutex2->hasLock == mutex1->hasLock) {
					
						printf("PCB%d owns M1 and M2\r\n", thisScheduler->running->pid);
						printf("NO DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
					} else if (mutex2->hasLock == mutex1->pcb2) {
					
						printf("PCB%d owns M1, failed to lock M2\r\n", thisScheduler->running->pid);
						printf("DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
						wasFound = 1;
					} else if (mutex1->hasLock == mutex1->pcb2) {
					
						printf("PCB%d owns M2, failed to lock M1\r\n", thisScheduler->running->pid);
						printf("DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
						wasFound = 1;
					}
					
				} else if (thisScheduler->running == mutex2->pcb2) { // check if pcb2 also owns the other lock
					if (mutex1->hasLock == mutex2->hasLock) {
						
						printf("PCB%d owns M1 and M2\r\n", thisScheduler->running->pid);
						printf("NO DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);

					} else if (mutex1->hasLock == mutex2->pcb1) {
					
						printf("PCB%d owns M2, failed to lock M1\r\n", thisScheduler->running->pid);
						printf("DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
						wasFound = 1;
					} else if (mutex2->hasLock == mutex1->pcb1) {
						
						printf("PCB%d owns M1, failed to lock M2\r\n", thisScheduler->running->pid);
						printf("DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
						wasFound = 1;
					}		
				}
			} else {
				printf("NO DEADLOCK DETECTED FOR PROCESSES PID%d & PID%d\r\n", mutex1->pcb1->pid, mutex1->pcb2->pid);
			}
			
		}	
		
		if (wasFound) {
			deadlockCount++;
			
			thisScheduler->running->term_count = thisScheduler->running->terminate;

		}
	}
	
	
	
	return wasFound;
}
/*
	10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
	
	In this project we will be making an MLFQ scheduling algorithm
	that will take a PriorityQueue of PCBs and run them through our scheduler.
	It will simulate the "running" of the process by incrementing the running PCB's PCB 
	by one on each loop through. It will also incorporate various interrupts to show 
	the effects it has on the scheduling simulator.
	
	This file holds the defined functions declared in the scheduler.h header file.
*/

#include "scheduler.h"


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
time_t t;

// The global counts of each PCB type, the final count at end of program run 
// should be roughly 50%, 25%, 12.5%, 12.5% respectively.
int compCount;
int ioCount;
int pairCount;
int sharedCount;



/*
	This function is our main loop. It creates a Scheduler object and follows the
	steps a normal MLFQ Priority Scheduler would to "run" for a certain length of time,
	check for all interrupt types, then call the ISR, scheduler,
	dispatcher, and eventually an IRET to return to the top of the loop and start
	with the new process.
*/
void osLoop () {
	int totalProcesses = 0, iterationCount = 1;
	Mutex currMutex;
	
	thisScheduler = schedulerConstructor();
	totalProcesses += makePCBList(thisScheduler);
	printSchedulerState(thisScheduler);
	for(;;) {
		if (thisScheduler->running != NULL) { // In case the first makePCBList makes 0 PCBs
			thisScheduler->running->context->pc++;
			
			if (thisScheduler->running->role == PAIR || thisScheduler->running->role == SHARED) {
				if (thisScheduler->running->context->pc == thisScheduler->running->lock_pc) {
					currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running);
					mutex_lock (currMutex, thisScheduler->running);
				} else if (thisScheduler->running->context->pc == thisScheduler->running->unlock_pc) {
					currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running);
					mutex_unlock (currMutex, thisScheduler->running);
				}
			}
			
			
			if (timerInterrupt(iterationCount) == 1) {
				pseudoISR(thisScheduler, IS_TIMER);
				printf("Completed Timer Interrupt\n");
				printSchedulerState(thisScheduler);
				iterationCount++;
			}
			
			if (ioTrap(thisScheduler->running) == 1) {
				printf("Iteration: %d\r\n", iterationCount);
				printf("Initiating I/O Trap\r\n");
				printf("PC when I/O Trap is Reached: %d\r\n", thisScheduler->running->context->pc);
				pseudoISR(thisScheduler, IS_IO_TRAP);
				
				printSchedulerState(thisScheduler);
				iterationCount++;
				printf("Completed I/O Trap\n");
			}
			
			if (thisScheduler->running != NULL)
			{
				if (thisScheduler->running->context->pc >= thisScheduler->running->max_pc) {
					thisScheduler->running->context->pc = 0;
					thisScheduler->running->term_count++;	//if terminate value is > 0
				}
			}
			
			if (ioInterrupt(thisScheduler->blocked) == 1) {
				printf("Iteration: %d\r\n", iterationCount);
				printf("Initiating I/O Interrupt\n");
				pseudoISR(thisScheduler, IS_IO_INTERRUPT);
				
				printSchedulerState(thisScheduler);
				iterationCount++;
				printf("Completed I/O Interrupt\n");
			}
			
			// if running PCB's terminate == running PCB's term_count, then terminate (for real).
			terminate(thisScheduler);
		} else {
			iterationCount++;
			printf("Idle\n");
		}
	
		
		if (!(iterationCount % RESET_COUNT)) {
			printf("\r\nRESETTING MLFQ\r\n");
			printf("iterationCount: %d\n", iterationCount);
			resetMLFQ(thisScheduler);
			//printf("here5.9\n");
			if (rand() % MAKE_PCB_CHANCE_DOMAIN <= MAKE_PCB_CHANCE_PERCENTAGE) {
				//printf("here6\n");
				totalProcesses += makePCBList (thisScheduler);
			}
			//printf("here6.5\n");
			printSchedulerState(thisScheduler);
			//printf("here6.6\n");
			iterationCount = 1;
		}
		if (totalProcesses >= MAX_PCB_TOTAL) {
			printf("Reached max PCBs, ending Scheduler.\r\n");
			break;
		}
	}
	
	schedulerDeconstructor(thisScheduler);
}


/*
	Checks if the global quantum tick is greater than or equal to
	the current quantum size for the running PCB. If so, then reset
	the quantum tick to 0 and return 1 so the pseudoISR can occur.
	If not, increase quantum tick by 1.
*/
int timerInterrupt(int iterationCount)
{
	if (quantum_tick >= currQuantumSize)
	{
		printf("Iteration: %d\r\n", iterationCount);
		printf("Initiating Timer Interrupt\n");
		printf("Current quantum tick: %d\r\n", quantum_tick);
		quantum_tick = 0;
		return 1;
	}
	else
	{
		quantum_tick++;
		return 0;
	}
}


/*
	Checks if the current PCB's PC is one of the premade io_traps for this
	PCB. If so, then return 1 so the pseudoISR can occur. If not, return 0.
*/
int ioTrap(PCB current)
{
	if (current) {
		unsigned int the_pc = current->context->pc;
		int c;
		for (c = 0; c < TRAP_COUNT; c++)
		{
			if(the_pc == current->io_1_traps[c])
			{
				return 1;
			}
		}
		for (c = 0; c < TRAP_COUNT; c++)
		{
			if(the_pc == current->io_2_traps[c])
			{
				return 1;
			}
		}
	}
	return 0;
}


/*
	Checks if the next up PCB in the Blocked queue has reached its max 
	blocked_timer value yet. If so, return 1 and initiate the IO Interrupt.
*/
int ioInterrupt(ReadyQueue the_blocked)
{
	if (the_blocked->first_node != NULL && q_peek(the_blocked) != NULL)
	{
		PCB nextup = q_peek(the_blocked);
		if (io_timer >= nextup->blocked_timer)
		{
			io_timer = 0;
			return 1;
		}
		else
		{
			io_timer++;
		}
	}
	
	return 0;
}


/*
	This creates the list of new PCBs for the current loop through. It simulates
	the creation of each PCB, the changing of state to new, enqueueing into the
	list of created PCBs, and moving each of those PCBs into the ready queue.
*/
int makePCBList (Scheduler theScheduler) {
	printf("making new PCBs\n");
	// change this to make 2 at a time.
	//int newPCBCount = rand() % MAX_PCB_IN_ROUND;
	int newPCBCount = 1;
	
	int lottery = rand();
	//for (int i = 0; i < newPCBCount; i++) {
	Mutex sharedMutex;
	
	printf("making Mutex\n");
	sharedMutex = mutex_init();
	printf("made Mutex\n");
	
	PCB newPCB1 = PCB_create();
	PCB newPCB2 = PCB_create();
	
	printf("newPCB1 == null? %d\n", (newPCB1 == NULL));
	printf("newPCB2 == null? %d\n", (newPCB2 == NULL));
	printf("sharedMutex == null? %d\n", (sharedMutex == NULL));
	
	newPCB2->parent = newPCB1->pid;
	
	initialize_pcb_type (newPCB1, 1, sharedMutex); 
	initialize_pcb_type (newPCB2, 0, sharedMutex); 
	
	incrementRoleCount(newPCB1->role);
	incrementRoleCount(newPCB2->role);
	
	//printf("\r\nM: ");	//STOPPED HERE!!!!
	//toStringReadyQueueMutexes(theScheduler->killedMutexes);
	if (newPCB1->role == COMP || newPCB1->role == IO) { //if the role isn't one that uses a mutex, then destroy it.
		printf("Role wasn't PAIR or SHARED, freeing sharedMutex M%d\r\n", sharedMutex->mid);
		free(sharedMutex);
	} else {
		printf("Role was PAIR or SHARED, enqueuing sharedMutex M%d\r\n", sharedMutex->mid);
		add_to_mutx_map(theScheduler->mutexes, sharedMutex, newPCB1);
	}
	
	newPCB1->state = STATE_NEW;
	newPCB2->state = STATE_NEW;
	printf("theScheduler->created == NULL: %d\n", (theScheduler->created == NULL));
	printf("enqueueing PCBs\n");
	q_enqueue(theScheduler->created, newPCB1);
	printf("enqueued pcb1\n");
	q_enqueue(theScheduler->created, newPCB2);
	printf("PCBs enqueued\n");
	//}
	//printf("Making New PCBs: \r\n");
	if (newPCBCount) {
		//printf("q_is_empty: %d\r\n", q_is_empty(theScheduler->created));
		while (!q_is_empty(theScheduler->created)) {
			PCB nextPCB = q_dequeue(theScheduler->created);
			//toStringPCB(nextPCB, 0);
			nextPCB->state = STATE_READY;
			//printf("\r\n");
			pq_enqueue(theScheduler->ready, nextPCB);
		}
		//printf("\r\n");
		
		//toStringPriorityQueue(theScheduler->ready);
		if (theScheduler->isNew) {
			//printf("Dequeueing PCB ");
			//printf("\r\nNEW!!\r\n");
			//toStringPCB(pq_peek(theScheduler->ready), 0);
			//printf("\r\n\r\n");
			theScheduler->running = pq_dequeue(theScheduler->ready);
			if (theScheduler->running) {
				theScheduler->running->state = STATE_RUNNING;
			}
			theScheduler->isNew = 0;
			currQuantumSize = theScheduler->ready->queues[0]->quantum_size;
		}
	}
	
	return newPCBCount + 1;
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
	Creates a random number between 3000 and 4000 and adds it to the current PC.
	It then returns that new PC value.
*/
unsigned int runProcess (unsigned int pc, int quantumSize) {
	//quantumSize is the difference in time slice length between
	//priority levels.
	unsigned int jump;
	if (quantumSize != 0) {
		jump = rand() % quantumSize;
	}
	
	pc += jump;
	return pc;
}


/*
	Marks the running PCB as terminated. This means it will increment its term_count, if the
	term_count is then over its maximum terminate amount, then it will be enqueued into the
	Killed queue which will empty when it reaches its TOTAL_TERMINATED size.
*/
void terminate(Scheduler theScheduler) {
	if(theScheduler->running != NULL && theScheduler->running->terminate > 0 && theScheduler->running->terminate == theScheduler->running->term_count)
	{
		printf("Marking for termination...\r\n");
		theScheduler->running->state = STATE_HALT;
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
	}
	scheduling(interruptType, theScheduler);
	//printf("finished scheduling\n");
	//printf("entering IRET\n");
	pseudoIRET(theScheduler);
	//printf("finished IRET\n");
	printf("Exiting ISR\n");
}


/*
	Prints the state of the Scheduler. Mostly this consists of the MLFQ, the next
	highest priority PCB in line, the one that will be run on next iteration, and
	the current list of "privileged PCBs" that will not be terminated.
*/
void printSchedulerState (Scheduler theScheduler) {
	
	printf("MLFQ State\r\n");
	toStringPriorityQueue(theScheduler->ready);
	printf("\r\n");
	
	int index = 0;
	// PRIVILIGED PID
	while(privileged[index] != NULL && index < MAX_PRIVILEGE) {
		printf("PCB PID %d, PRIORITY %d, PC %d\n", 
		privileged[index]->pid, privileged[index]->priority, 
		privileged[index]->context->pc);
		index++;
	}
	printf("blocked size: %d\r\n", theScheduler->blocked->size);
	printf("killed size: %d\r\n", theScheduler->killed->size);
	printf("killedMutexes size: %d\r\n", theScheduler->killedMutexes->size);
	printf("\r\n");
	
	if (pq_peek(theScheduler->ready) != NULL) {
		printf("Going to be running ");
		if (theScheduler->running) {
			toStringPCB(theScheduler->running, 0);
		} else {
			printf("\r\n");
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
	Mutex currMutex;
	
	if (interrupt_code == IS_TIMER) {
		printf("Entering Timer Interrupt\r\n");
		theScheduler->interrupted->state = STATE_READY;
		if (theScheduler->interrupted->priority < (NUM_PRIORITIES - 1)) {
			theScheduler->interrupted->priority++;
		} else {
			theScheduler->interrupted->priority = 0;
		}
		printf("\r\nEnqueueing into MLFQ\r\n");
		toStringPCB(theScheduler->running, 0);
		//printf("after1\n");
		
		pq_enqueue(theScheduler->ready, theScheduler->interrupted);
		//printf("done enqueueing\n");
		//toStringPriorityQueue(theScheduler->ready);
		//printf("ENQ!\r\n");
		int index = isPrivileged(theScheduler->running);
		
		if (index != 0) {
			privileged[index] = theScheduler->running;
		}
		printf("Exiting Timer Interrupt\r\n");
	}
	else if (interrupt_code == IS_IO_TRAP)
	{
		// Do I/O trap handling
		printf("Entering IO Trap\r\n");
		int timer = (rand() % TIMER_RANGE + 1);
		theScheduler->interrupted->blocked_timer = timer;
		theScheduler->interrupted->state = STATE_WAIT;
		printf("\r\nEnqueueing into Blocked queue\r\n");
		toStringPCB(theScheduler->interrupted, 0);
		//printf("after\n");
		//exit(0);
		//printf("going to enqueue\n");
		q_enqueue(theScheduler->blocked, theScheduler->interrupted);
		//printf("done enqueueing\n");
		theScheduler->interrupted = NULL;
		
		// schedule a new process
		printf("Exiting IO Trap\r\n");
	}
	else if (interrupt_code == IS_IO_INTERRUPT)
	{
		printf("Entering IO Interrupt\r\n");
		// Do I/O interrupt handling
		printf("\r\nEnqueueing into MLFQ from Blocked queue\r\n");
		toStringPCB(q_peek(theScheduler->blocked), 0);
		pq_enqueue(theScheduler->ready, q_dequeue(theScheduler->blocked));
		//printSchedulerState(theScheduler);
		if (theScheduler->interrupted != NULL)
		{
			theScheduler->running = theScheduler->interrupted;
			theScheduler->running->state = STATE_RUNNING;
		
			sysstack = theScheduler->running->context->pc;
		}
		theScheduler->interrupted = NULL;
		printf("Exiting IO Interrupt\r\n");
	}
	
	if (theScheduler->running != NULL && theScheduler->running->state == STATE_HALT) {
		//printf("entering handleKilling\n");
		handleKilling(theScheduler);
	}
	
	// I/O interrupt does not require putting a new process
	// into the running state, so we ignore this.
	if(interrupt_code != IS_IO_INTERRUPT)
	{
		theScheduler->running = pq_peek(theScheduler->ready);
	}
	
	if (theScheduler->killed->size >= TOTAL_TERMINATED) {
		printf("Emptying killed list\n");
		PCB toKill = NULL;
		
		if (!(theScheduler->killed)) {
			printf("killed queue is NULL!\r\n");
		}
		
		if (theScheduler->killed && !q_is_empty(theScheduler->killed)) {
			toKill = q_dequeue(theScheduler->killed);
		}
		printf("here\n");
		if (theScheduler->killed && !q_is_empty(theScheduler->killed)) {
			while(!q_is_empty(theScheduler->killed)) {
				//if (theScheduler->killed->size > 1) {
					//printf("pid to kill: P%d\n", toKill->pid);
					if (toKill) {
						PCB_destroy(toKill);
					} else {
						printf("toKill was NULL!\r\n");
					}
					//printf("toKill == null: %d\n", (toKill == NULL));
					//printf("pid after kill: P%d\n\n", toKill->pid);
				//}
				toKill = q_dequeue(theScheduler->killed);
			} 
		} else {
			if (toKill) {
				PCB_destroy(toKill);
			} else {
				printf("toKill was NULL!\r\n");
			}
		}
		printf("After emptying\n");
		printf("Killed List: ");
		toStringReadyQueue(theScheduler->killed);
		//exit(0);
	}

	// I/O interrupt does not require putting a new process
	// into the running state, so we ignore this.
	if(interrupt_code != IS_IO_INTERRUPT) 
	{
		//printf("entering dispatcher\n");
		dispatcher(theScheduler);
		//printf("exited dispatcher\n");
	}
}


/*
	This simply gets the next ready PCB from the Ready queue and moves it into the
	running state of the Scheduler.
*/
void dispatcher (Scheduler theScheduler) {
	if (pq_peek(theScheduler->ready) != NULL && pq_peek(theScheduler->ready)->state != STATE_HALT) {
		//printf("inside dispatcher\n");
		currQuantumSize = getNextQuantumSize(theScheduler->ready);
		theScheduler->running = pq_dequeue(theScheduler->ready);
		theScheduler->running->state = STATE_RUNNING;
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
	//printf("\r\n");
	if (theScheduler) {
		
		if (theScheduler->ready) {
			//printf("destroying ready\n");
			//toStringPriorityQueue(theScheduler->ready);
			pq_destroy(theScheduler->ready);
		}
		
		if (theScheduler->created) {
			//printf("destroying created\n");
			q_destroy(theScheduler->created);
		}		
		
		if (theScheduler->killed) {
			//printf("destroying killed\n");
			q_destroy(theScheduler->killed);
		}
		
		if (theScheduler->blocked) {
			//printf("destroying blocked\n");
			q_destroy(theScheduler->blocked);
		}
		
		if (theScheduler->killedMutexes) {
			//printf("destroying killedMutexes\n");
			q_destroy_m(theScheduler->killedMutexes);
			//printf("destroyed killedMutexes\n");
		}
		
		if (theScheduler->mutexes) {
			//destroy mutex map
		}
		
		if (theScheduler->running) {
			//printf("destroying running\n");
			PCB_destroy(theScheduler->running);
		}
		
		if (theScheduler->interrupted && theScheduler->running && theScheduler->interrupted == theScheduler->running) {
			//printf("destroying interrupted\n");
			PCB_destroy(theScheduler->interrupted);
		}
		
		//printf("destroying scheduler\n");
		free (theScheduler);
	}
	
	displayRoleCountResults();
}


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
	setvbuf(stdout, NULL, _IONBF, 0);
	srand((unsigned) time(&t));
	sysstack = 0;
	switchCalls = 0;
	currQuantumSize = 0;
	
	//initialize pcb type counts
	compCount = 0;
	ioCount = 0;
	pairCount = 0;
	sharedCount = 0;
	
	int totalProcesses = 0, iterationCount = 1;
	Mutex currMutex;
	
	thisScheduler = schedulerConstructor();
	totalProcesses += makePCBList(thisScheduler);
	printSchedulerState(thisScheduler);
	for(;;) {
		if (thisScheduler->running != NULL) { // In case the first makePCBList makes 0 PCBs
			thisScheduler->running->context->pc++;
			
			/*if (thisScheduler->running->role == PAIR || thisScheduler->running->role == SHARED) {
				if (thisScheduler->running->context->pc == thisScheduler->running->lock_pc) {
					currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running);
					mutex_lock (currMutex, thisScheduler->running);
				} else if (thisScheduler->running->context->pc == thisScheduler->running->unlock_pc) {
					currMutex = get_mutx(thisScheduler->mutexes, thisScheduler->running);
					mutex_unlock (currMutex, thisScheduler->running);
				}
			}*/
			
			
			if (timerInterrupt(iterationCount) == 1) {
				pseudoISR(thisScheduler, IS_TIMER);
				printf("Completed Timer Interrupt\n");
				printSchedulerState(thisScheduler);
				iterationCount++;
			}
			
			/*if (ioTrap(thisScheduler->running) == 1) {
				printf("Iteration: %d\r\n", iterationCount);
				printf("Initiating I/O Trap\r\n");
				printf("PC when I/O Trap is Reached: %d\r\n", thisScheduler->running->context->pc);
				pseudoISR(thisScheduler, IS_IO_TRAP);
				
				printSchedulerState(thisScheduler);
				iterationCount++;
				printf("Completed I/O Trap\n");
			}*/
			
			if (thisScheduler->running != NULL)
			{
				if (thisScheduler->running->context->pc >= thisScheduler->running->max_pc) {
					thisScheduler->running->context->pc = 0;
					thisScheduler->running->term_count++;	//if terminate value is > 0
				}
			}
			
			/*if (ioInterrupt(thisScheduler->blocked) == 1) {
				printf("Iteration: %d\r\n", iterationCount);
				printf("Initiating I/O Interrupt\n");
				pseudoISR(thisScheduler, IS_IO_INTERRUPT);
				
				printSchedulerState(thisScheduler);
				iterationCount++;
				printf("Completed I/O Interrupt\n");
			}*/
			
			// if running PCB's terminate == running PCB's term_count, then terminate (for real).
			terminate(thisScheduler);
		} else {
			iterationCount++;
			printf("Idle\n");
		}
	
		
		if (!(iterationCount % RESET_COUNT)) {
			printf("\r\nRESETTING MLFQ\r\n");
			printf("iterationCount: %d\n", iterationCount);
			resetMLFQ(thisScheduler);
			//printf("here5.9\n");
			if (rand() % MAKE_PCB_CHANCE_DOMAIN <= MAKE_PCB_CHANCE_PERCENTAGE) {
				//printf("here6\n");
				totalProcesses += makePCBList (thisScheduler);
			}
			//printf("here6.5\n");
			printSchedulerState(thisScheduler);
			//printf("here6.6\n");
			iterationCount = 1;
		}
		if (totalProcesses >= MAX_PCB_TOTAL) {
			printf("Reached max PCBs, ending Scheduler.\r\n");
			break;
		}
	}
	
	schedulerDeconstructor(thisScheduler);
}


/*void main () {
	setvbuf(stdout, NULL, _IONBF, 0);
	srand((unsigned) time(&t));
	sysstack = 0;
	switchCalls = 0;
	currQuantumSize = 0;
	
	//initialize pcb type counts
	compCount = 0;
	ioCount = 0;
	pairCount = 0;
	sharedCount = 0;
	
	osLoop();
}*/


/*void printNull (Mutex mutex) {
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
}*/


/*void main () {
	setvbuf(stdout, NULL, _IONBF, 0);
	srand((unsigned) time(&t));
	
	int outerLoop = 100;
	int innerLoop = 300;
	Scheduler scheduler = schedulerConstructor();
	
	for (int i = 0; i < outerLoop; i++) {
		for (int j = 0; j < innerLoop; j++) {		

			makePCBList(scheduler);
		
			/*Mutex newMutex = mutex_init();
			
			PCB pcb1 = PCB_create();
			PCB pcb2 = PCB_create();
			
			pcb2->parent = pcb1->pid;
			
			initialize_pcb_type(pcb1, 1, newMutex);
			initialize_pcb_type(pcb2, 0, newMutex);
			
			incrementRoleCount(pcb1->role);
			incrementRoleCount(pcb2->role);
			
			pq_enqueue(scheduler->ready, pcb1);
			pq_enqueue(scheduler->ready, pcb2);
			
			if ((pcb1->role == PAIR || pcb1->role == SHARED) && !get_mutx(scheduler->mutexes, pcb1)) {
				add_to_mutx_map (scheduler->mutexes, newMutex, pcb1);
			} else {
				if (get_mutx(scheduler->mutexes, pcb1)) {
					printf("Mutex M%d already in the Mutex List!\r\n", newMutex->mid);
				} else {
					printf("Mutex M%d not added, PCBs weren't type PAIR or SHARED\n", newMutex->mid);
				}
			}
		}
	}
	
	printSchedulerState(scheduler);
	
	while(!pq_is_empty(scheduler->ready)) {
		PCB pcb = pq_dequeue(scheduler->ready);
		printf("destroying P%d\n", pcb->pid);
		PCB_destroy(pcb);
		//toStringPriorityQueue(scheduler->ready);
		//exit(0);
	}
	
	printSchedulerState(scheduler);
	printf("PriorityQueue Empty\n");
	schedulerDeconstructor(scheduler);
}*/


/*void main () {
	setvbuf(stdout, NULL, _IONBF, 0);
	srand((unsigned) time(&t));
	
	Scheduler scheduler = schedulerConstructor();
	for (int i = 0; i < 5; i++) {
		Mutex newMutex = mutex_init();
		
		PCB pcb1 = PCB_create();
		PCB pcb2 = PCB_create();
		
		pcb2->parent = pcb1->pid;
		
		initialize_pcb_type(pcb1, 1, newMutex);
		initialize_pcb_type(pcb2, 0, newMutex);
		
		incrementRoleCount(pcb1->role);
		incrementRoleCount(pcb2->role);
		
		pq_enqueue(scheduler->ready, pcb1);
		pq_enqueue(scheduler->ready, pcb2);
		
		if ((pcb1->role == PAIR || pcb1->role == SHARED) && !q_contains_mutex(scheduler->killedMutexes, newMutex)) {
			//printNull(newMutex);
			add_to_mutx_map (scheduler->mutexes, newMutex, pcb1);
		} else {
			if (get_mutx(scheduler->mutexes, pcb1)) {
				printf("Mutex M%d already in the Mutex List!\r\n", newMutex->mid);
			} else {
				printf("Mutex M%d not added, PCBs weren't type PAIR or SHARED\n", newMutex->mid);
			}
		}
	}
	
	PCB toFind = scheduler->ready->queues[0]->first_node->next->pcb;
	toStringPriorityQueue(scheduler->ready);
	printf("Trying to remove P%d\n", toFind->pid);
	PCB match1 = pq_remove_matching_pcb (scheduler->ready, toFind);
	
	printf("After removing P%d\n", match1->pid);
	toStringPriorityQueue(scheduler->ready);
	
	PCB level12 = PCB_create();
	level12->role = PAIR;
	
	PCB level13 = PCB_create();
	level13->role = PAIR;
	level13->parent = level12->pid;
	
	Mutex sharedMutex = mutex_init();
	sharedMutex->pcb1 = level12;
	sharedMutex->pcb2 = level13;
	
	add_to_mutx_map(scheduler->mutexes, sharedMutex, level12);
	
	q_enqueue(scheduler->ready->queues[12], level12);
	q_enqueue(scheduler->ready->queues[13], level13);
	
	
	PCB toFind2 = scheduler->ready->queues[12]->first_node->pcb;
	toStringPriorityQueue(scheduler->ready);
	printf("Trying to remove P%d\n", toFind2->pid);
	PCB match2 = pq_remove_matching_pcb (scheduler->ready, toFind2);
	
	printf("After removing P%d\n", match2->pid);
	toStringPriorityQueue(scheduler->ready);
	
	schedulerDeconstructor(scheduler);
}*/


void handleKilling (Scheduler theScheduler) {
	Mutex currMutex;
	PCB found;
	
	printf("\r\nEnqueueing into Killed queue\r\n");
	if (theScheduler->running->role == PAIR || theScheduler->running->role == SHARED) {
		printf("going to get a mutex\n");
		currMutex = get_mutx(theScheduler->mutexes, theScheduler->running);
		printf("finished getting mutex\n");
		printf("currMutex == null? %d\n", (currMutex == NULL));
		if (currMutex->pcb2 == theScheduler->running) { //if running is the pcb2 in the mutex, find the matching pcb1
			printf("going to find matching pcb1\n");
			printf("currMutex->pcb2 == null? %d, pid: P%d\n", (currMutex->pcb2 == NULL), currMutex->pcb2->pid);
			printf("currMutex->pcb1 == null? %d, pid: P%d\n", (currMutex->pcb1 == NULL), currMutex->pcb1->pid);
			toStringPriorityQueue(theScheduler->ready);
			found = pq_remove_matching_pcb(theScheduler->ready, currMutex->pcb1);
			printf("found matching pcb1\n");
		} else { //otherwise the running is pcb1, so find pcb2
			printf("going to find matching pcb2\n");
			printf("currMutex->pcb1 == null? %d, pid: P%d\n", (currMutex->pcb1 == NULL), currMutex->pcb1->pid);
			printf("currMutex->pcb2 == null? %d, pid: P%d\n", (currMutex->pcb2 == NULL), currMutex->pcb2->pid);
			toStringPriorityQueue(theScheduler->ready);
			found = pq_remove_matching_pcb(theScheduler->ready, currMutex->pcb2);
			printf("found matching pcb2\n");
		}
		
		printf("running: P%d\n", theScheduler->running->pid);
		printf("killed list: ");
		toStringReadyQueue (theScheduler->killed);
		printf("found == null? %d\n", (found == NULL));
		
		q_enqueue(theScheduler->killed, theScheduler->running);
		if (found) { //if found is null then the partnering pcb is already in the killed queue
			printf("found: P%d\n", found->pid);
			q_enqueue(theScheduler->killed, found);
		}
		
		printf("Mutex List: ");
		toStringMutexMap(theScheduler->mutexes);
		//printf("END\r\n");
	} else {
		printf("enqueuing into killed queue\n");
		q_enqueue(theScheduler->killed, theScheduler->running);
	}
	printf("Killed List: ");
	toStringReadyQueue(theScheduler->killed);
	theScheduler->running = NULL;
}
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
pthread_mutex_t schedulerMutex;
phtread_cond_t trapCond;



/*
	This function is our main loop. It creates a Scheduler object and follows the
	steps a normal MLFQ Priority Scheduler would to "run" for a certain length of time,
	check for all interrupt types, then call the ISR, scheduler,
	dispatcher, and eventually an IRET to return to the top of the loop and start
	with the new process.
*/
void osLoop () {
	int totalProcesses = 0, iterationCount = 1;
	thisScheduler = schedulerConstructor();
	
	pthread_t timer, iotrap, iointerrupt;
	pthread_attr_t attr;
	
	pthread_mutex_init(&schedulerMutex, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	pthread_cond_init(&trapCond, NULL);
	
	pthread_create(&timer, &attr, timerInterrupt, (void *) thisScheduler);
	pthread_create(&iotrap, &attr, ioTrap, (void *) thisScheduler);
	pthread_create(&iointerrupt, &attr, ioInterrupt, (void *) thisScheduler);
	
	
	totalProcesses += makePCBList(thisScheduler);
	printSchedulerState(thisScheduler);
	for(;;) {
		if (thisScheduler->running != NULL) { // In case the first makePCBList makes 0 PCBs
			if (pthread_mutex_trylock(&schedulerMutex)) {
				thisScheduler->running->context->pc++;
				pthread_mutex_unlock(&schedulerMutex);
			}
			/*
			if (timerInterrupt(iterationCount) == 1) {
				pseudoISR(thisScheduler, IS_TIMER);
				printf("Completed Timer Interrupt\n");
				printSchedulerState(thisScheduler);
				iterationCount++;
			}
			*/

			if (ioTrap(thisScheduler->running) == 1) {
				//This is now in the io trap thread.
			}
			
			if (thisScheduler->running != NULL)
			{
				if (thisScheduler->running->context->pc >= thisScheduler->running->max_pc) {
					printf("made it here\n");
					//exit(0);
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
			if (rand() % MAKE_PCB_CHANCE_DOMAIN <= MAKE_PCB_CHANCE_PERCENTAGE) {
				totalProcesses += makePCBList (thisScheduler);
			}
			printSchedulerState(thisScheduler);
			iterationCount = 1;
		}
		if (totalProcesses >= MAX_PCB_TOTAL) {
			printf("Reached max PCBs, ending Scheduler.\r\n");
			break;
		}
	}
	
	// join all of the threads.
	pthread_join(timer, NULL);
	pthread_join(iotrap, NULL);
	pthread_join(iointerrupt, NULL);
	
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&schedulerMutex);
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
	struct timespec quantum;
	quantum.tv_sec = 0;
	for(;;)
	{
		quantum.tv_nsec = currQuantumSize;
		nanosleep(quantum, NULL);
		pthread_mutex_lock(schedulerMutex);
		pseudoISR(thisScheduler, IS_TIMER);
		printf("Completed Timer Interrupt\n");
		printSchedulerState(thisScheduler);
		iterationCount++;
		pthread_mutex_unlock(schedulerMutex);
	}
	/*
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
	*/
}


/*
	Checks if the current PCB's PC is one of the premade io_traps for this
	PCB. If so, then return 1 so the pseudoISR can occur. If not, return 0.
*/
void * ioTrap(void * currentScheduler)
{
	Scheduler curr = (Scheduler) currentScheduler;
	
	unsigned int the_pc = curr->running->context->pc;
	int c;
	
	//change this thread to only be executed when a new running is set?
	for (;;) {
		while (the_pc < curr->running->max_pc) {
			pthread_cond_wait(&trapCond, &schedulerMutex);
			printf("Trap signalled");
		}
		
		printf("Iteration: %d\r\n", iterationCount);
		printf("Initiating I/O Trap\r\n");
		printf("PC when I/O Trap is Reached: %d\r\n", curr->running->context->pc);
		
		pthread_mutex_lock(schedulerMutex);
		pseudoISR(thisScheduler, IS_IO_TRAP);
		pthread_mutex_unlock(schedulerMutex);
		
		printSchedulerState(thisScheduler);
		iterationCount++;
		printf("Completed I/O Trap\n");
	}
	
	
	//should this go into it's own thread?
	/*for (c = 0; c < TRAP_COUNT; c++)
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
	return 0;*/
}


/*
	Checks if the next up PCB in the Blocked queue has reached its max 
	blocked_timer value yet. If so, return 1 and initiate the IO Interrupt.
*/
void * ioInterrupt(ReadyQueue the_blocked)
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
	int newPCBCount = rand() % MAX_PCB_IN_ROUND;
	//int newPCBCount = 3;
	
	int lottery = rand();
	for (int i = 0; i < newPCBCount; i++) {
		PCB newPCB = PCB_create();
		newPCB->state = STATE_NEW;
		q_enqueue(theScheduler->created, newPCB);
	}
	printf("Making New PCBs: \r\n");
	if (newPCBCount) {
		while (!q_is_empty(theScheduler->created)) {
			PCB nextPCB = q_dequeue(theScheduler->created);
			nextPCB->state = STATE_READY;
			toStringPCB(nextPCB, 0);
			printf("\r\n");
			pq_enqueue(theScheduler->ready, nextPCB);
		}
		printf("\r\n");

		if (theScheduler->isNew) {
			printf("Dequeueing PCB ");
			toStringPCB(pq_peek(theScheduler->ready), 0);
			printf("\r\n\r\n");
			theScheduler->running = pq_dequeue(theScheduler->ready);
			theScheduler->running->state = STATE_RUNNING;
			theScheduler->isNew = 0;
			currQuantumSize = theScheduler->ready->queues[0]->quantum_size;
		}
	}
	
	return newPCBCount;
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
	pseudoIRET(theScheduler);
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
		pq_enqueue(theScheduler->ready, theScheduler->interrupted);
		
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
		//exit(0);
		q_enqueue(theScheduler->blocked, theScheduler->interrupted);
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
		printSchedulerState(theScheduler);
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
		printf("\r\nEnqueueing into Killed queue\r\n");
		q_enqueue(theScheduler->killed, theScheduler->running);
		theScheduler->running = NULL;
	}
	
	// I/O interrupt does not require putting a new process
	// into the running state, so we ignore this.
	if(interrupt_code != IS_IO_INTERRUPT)
	{
		theScheduler->running = pq_peek(theScheduler->ready);
	}
	
	if (theScheduler->killed->size >= TOTAL_TERMINATED) {
		PCB toKill;
		if (!q_is_empty(theScheduler->killed)) {
			toKill = q_dequeue(theScheduler->killed);
		}
		if (!q_is_empty(theScheduler->killed)) {
			while(!q_is_empty(theScheduler->killed)) {
				if (theScheduler->killed->size > 1) {
					PCB_destroy(toKill);
				}
				toKill = q_dequeue(theScheduler->killed);
			} 
		} else {
			PCB_destroy(toKill);
		}
	}

	// I/O interrupt does not require putting a new process
	// into the running state, so we ignore this.
	if(interrupt_code != IS_IO_INTERRUPT) 
	{
		dispatcher(theScheduler);
	}
}


/*
	This simply gets the next ready PCB from the Ready queue and moves it into the
	running state of the Scheduler.
*/
void dispatcher (Scheduler theScheduler) {
	if (pq_peek(theScheduler->ready) != NULL && pq_peek(theScheduler->ready)->state != STATE_HALT) {
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
	Scheduler newScheduler = (Scheduler) malloc (sizeof(scheduler_s));
	newScheduler->created = q_create();
	newScheduler->killed = q_create();
	newScheduler->blocked = q_create();
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
	q_destroy(theScheduler->created);
	q_destroy(theScheduler->killed);
	q_destroy(theScheduler->blocked);
	pq_destroy(theScheduler->ready);
	PCB_destroy(theScheduler->running);
	if (theScheduler->interrupted == theScheduler->running) {
		PCB_destroy(theScheduler->interrupted);
	}
	free (theScheduler);
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
	osLoop();
}

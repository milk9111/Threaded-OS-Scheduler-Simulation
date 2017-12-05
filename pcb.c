/*
    10/28/2017
	Authors: Connor Lundberg, Jasmine Dacones, Jacob Ackerman
 */

#include "pcb.h"

int global_largest_PID = 0;

/*
 * Helper function to iniialize PCB data.
 */
void initialize_data(/* in-out */ PCB pcb) {
	pcb->pid = 0;
	pcb->parent = -1;
	PCB_assign_priority(pcb, 0);
	pcb->size = 0;
	pcb->channel_no = 0;
	pcb->state = 0;
	pcb->blocked_timer = -1;

	pcb->mem = NULL;

	pcb->context->pc = 0;
	pcb->context->ir = 0;
	pcb->context->r0 = 0;
	pcb->context->r1 = 0;
	pcb->context->r2 = 0;
	pcb->context->r3 = 0;
	pcb->context->r4 = 0;
	pcb->context->r5 = 0;
	pcb->context->r6 = 0;
	pcb->context->r7 = 0;
  
	pcb->max_pc = makeMaxPC();
	pcb->creation = 0;
	pcb->termination = 0;
	pcb->terminate = rand() % MAX_TERM_COUNT;
	if (pcb->terminate == 0) {
		pcb->terminate++;
	}
	pcb->term_count = 0;
	
	pcb->isProducer = 0;
	pcb->isConsumer = 0;
}


/*
	Chooses a role type for a PCB to be assigned to. The percentages work as such:
		- 50% will be Computationally Intensive
		- 25% will be I/O Trap
		- 12.5% will be Producer/Consumer
		- 12.5% will be Shared Resource
*/
enum pcb_type chooseRole () {
	int num = rand() % ROLE_PERCENTAGE_MAX_RANGE;
	enum pcb_type newRole;
	
	if (num <= COMP_ROLE_MAX_RANGE) {
		newRole = COMP;
	} else if (num >= IO_ROLE_MIN_RANGE && num <= IO_ROLE_MAX_RANGE) {
		newRole = IO;
	} else if (num >= PAIR_ROLE_MIN_RANGE && num <= PAIR_ROLE_MAX_RANGE) {
		newRole = PAIR;
	} else {
		newRole = SHARED;
	}
	
	return newRole;
}


/*
	This will initialize the role type of the given PCB. If it is the first of the pair
	that is created, then the role will be randomly chosen, otherwise the role will match 
	the first one created. From there, if the role is IO, then the PCBs I/O Trap locations
	will be initialized. If the role is PAIR or SHARED, then the 1st and 2nd PCBs contained 
	within the given shared mutex will be set.
*/
void initialize_pcb_type (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2) {
	int lock = 0, unlock = 0, signal = 0, wait = 0;
	
	if (isFirst) {
		pcb->role = chooseRole();
	} else {
		pcb->role = sharedMutexR1->pcb1->role;
	}
  
	
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


/*void populateMutexLocations (PCB pcb) {
	printf("Going to populate Mutex locations\n");
	printf("max_pc: %d\n", pcb->max_pc);
	unsigned int newLock = 0, newUnlock = 0, lastLock = 0, lastUnlock = 0, top = 0;
	int max_pc_range = pcb->max_pc / 4;
	for (int i = 0; i < TRAP_COUNT; i++) {
		newLock = lastLock + (rand() % max_pc_range);
		while (newLock >= pcb->max_pc) {
			newLock = lastLock + (rand() % max_pc_range);		
		}
		
		if (newLock == 0) {
			newLock++;
		}
		
		if (lastUnlock >= 0) {
			while (newLock <= lastUnlock) {
				newLock++;
			}
		}
		pcb->lockR1[top] = newLock;
		lastLock = newLock;
		
		newUnlock = newLock + (rand() % max_pc_range);
		while (newUnlock >= pcb->max_pc) {
			newUnlock = newLock + (rand() % max_pc_range);
		}
		
		if (lastLock >= 0) {
			while (newUnlock <= newLock) {
				newUnlock++;
			}
		}
		pcb->unlockR1[top] = newUnlock;
		lastUnlock = newUnlock;
		top++;
	}
	
	top = 0;
	lastLock = 0, lastUnlock = 0;;
	printPCLocations(pcb->lockR1);
	printPCLocations(pcb->unlockR1);
	for (int i = 0; i < TRAP_COUNT; i++) {
		
	}
	
	
}*/


void printPCLocations (unsigned int pcLocs[]) {
	for (int i = 0; i < TRAP_COUNT; i++) {
		printf("%d ", pcLocs[i]);
	}
	printf("\r\n");
}


/*
	A helper function to populate the IO Trap PC values for the pcb. Creates a random number 
	that is less than the pcb's max PC value. While the random number is contained within either
	pcb trap lists, the random number is incremented by one. Once a new number is created that is 
	not within the two lists, it will be inserted into the next open position for the list specified
	by the ioTrapType parameter.
*/
void populateIOTraps (PCB pcb, int ioTrapType) {
	unsigned int newRand = 0;
	for (int i = 0; i < TRAP_COUNT; i++) {
		newRand = rand() % pcb->max_pc;
		while (ioTrapContains(newRand, pcb->io_1_traps) || ioTrapContains(newRand, pcb->io_2_traps)) {
			newRand++;
		}
		if (!ioTrapType) {
			pcb->io_1_traps[i] = newRand;
		} else {
			pcb->io_2_traps[i] = newRand;
		}
	}
}


void populateMutexTraps1221(PCB pcb, int step) {
	memcpy(pcb->lockR1, ((unsigned int[TRAP_COUNT]) {1 * step, 5 * step, 9 * step, 13 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->lockR2, ((unsigned int[TRAP_COUNT]) {2 * step, 6 * step, 10 * step, 14 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->unlockR2, ((unsigned int[TRAP_COUNT]) {3 * step, 7 * step, 11 * step, 15 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->unlockR1, ((unsigned int[TRAP_COUNT]) {4 * step, 8 * step, 12 * step, 16 * step}), 4 * sizeof(unsigned int));
}

void populateMutexTraps2112(PCB pcb, int step) {
	memcpy(pcb->lockR2, ((unsigned int[TRAP_COUNT]) {1 * step, 5 * step, 9 * step, 13 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->lockR1, ((unsigned int[TRAP_COUNT]) {2 * step, 6 * step, 10 * step, 14 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->unlockR1, ((unsigned int[TRAP_COUNT]) {3 * step, 7 * step, 11 * step, 15 * step}), 4 * sizeof(unsigned int));
	memcpy(pcb->unlockR2, ((unsigned int[TRAP_COUNT]) {4 * step, 8 * step, 12 * step, 16 * step}), 4 * sizeof(unsigned int));
}


void populateProducerConsumerTraps(PCB pcb, int step, int isProducer) {
	memcpy(pcb->lockR1, ((unsigned int[TRAP_COUNT]) {1 * step, 5 * step,  9 * step, 13 * step}), 4 * sizeof(unsigned int)); 
	if (!isProducer) {
		memcpy(pcb->wait_cond, ((unsigned int[TRAP_COUNT]) {2 * step, 6 * step, 10 * step, 14 * step}), 4 * sizeof(unsigned int)); 
    } else {
		memcpy(pcb->signal_cond,((unsigned int[TRAP_COUNT]) {3 * step, 7 * step, 11 * step, 15 * step}), 4 * sizeof(unsigned int));
	}
	memcpy(pcb->unlockR1, ((unsigned int[TRAP_COUNT]) {4 * step, 8 * step, 12 * step, 16 * step}), 4 * sizeof(unsigned int)); 
}





/*
	Checks if the given random number is already within the given ioTraps array. If so,
	return 1, otherwise 0.
*/
int ioTrapContains (unsigned int newRand, unsigned int ioTraps[]) {
	unsigned int *ptr = ioTraps;
	int isContained = 0;
	
	while (*ptr) {
		if (*ptr == newRand) {
			isContained = 1;
			break;
		}
		ptr++;
	}
	
	return isContained;
}



/*
	Makes a random number between 0 and LARGEST_PC_POSSIBLE *found in pcb.h*.
	If the number is less than SMALLEST_PC_POSSIBLE *found in pcb.h*, then maxPC will
	be incremented by a new random number mod SMALLEST_PC_POSSIBLE then + SMALLEST_PC_POSSIBLE
	and returned.
*/
unsigned int makeMaxPC () {
	unsigned int maxPC = rand() % LARGEST_PC_POSSIBLE;
	if (maxPC < SMALLEST_PC_POSSIBLE) maxPC += ((rand() % SMALLEST_PC_POSSIBLE) + SMALLEST_PC_POSSIBLE);
	return maxPC;
}


/*
 * Allocate a PCB and a context for that PCB.
 *
 * Return: NULL if context or PCB allocation failed, the new pointer otherwise.
 */
PCB PCB_create() {
    PCB new_pcb = (PCB) malloc(sizeof(struct pcb));
    if (new_pcb != NULL) {
        new_pcb->context = (CPU_context_p) malloc(sizeof(struct cpu_context));
        if (new_pcb->context != NULL) {
            initialize_data(new_pcb);
			PCB_assign_PID(new_pcb);
        } else {
            free(new_pcb);
            new_pcb = NULL;
        }
		
    }
    return new_pcb;
}

/*
 * Frees a PCB and its context.
 *
 * Arguments: pcb: the pcb to free.
 */
void PCB_destroy(/* in-out */ PCB pcb) {
	if (pcb) {
		//printf("pcb not null\n");
		//printf("pid before context free: P%d\n", pcb->pid);
		if (pcb->context) {
			free(pcb->context); 
		}	
		//printf("pid before free: P%d\n", pcb->pid);
		free(pcb);// that thing
		
		pcb = NULL;
		//printf("pcb == null: %d\n", (pcb == NULL));
		//printf("pid after free: P%d\n\n", pcb->pid);
	}
}

/*
 * Assigns intial process ID to the process.
 *
 * Arguments: pcb: the pcb to modify.
 */
void PCB_assign_PID(/* in */ PCB the_PCB) {
    the_PCB->pid = global_largest_PID;
    global_largest_PID++;
}

/*
 * Sets the state of the process to the provided state.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new state of the process.
 */
void PCB_assign_state(/* in-out */ PCB the_pcb, /* in */ enum state_type the_state) {
    the_pcb->state = the_state;
}

/*
 * Sets the parent of the given pcb to the provided pid.
 *
 * Arguments: pcb: the pcb to modify.
 *            pid: the parent PID for this process.
 */
void PCB_assign_parent(PCB the_pcb, int the_pid) {
    the_pcb->parent = the_pid;
}

/*
 * Sets the priority of the PCB to the provided value.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new priority of the process.
 */
void PCB_assign_priority(/* in */ PCB the_pcb, /* in */ unsigned int the_priority) {
    the_pcb->priority = the_priority;
    if (the_priority > NUM_PRIORITIES) {
        the_pcb->priority = NUM_PRIORITIES - 1;
    }
}


/*
 * Create and return a string representation of the provided PCB.
 *
 * Arguments: pcb: the pcb to create a string representation of.
 * Return: a string representation of the provided PCB on success, NULL otherwise.
 */
void toStringPCB(PCB thisPCB, int showCpu) {
	if (thisPCB) {
		printf("contents: ");
		
		printf("PID: %d, ", thisPCB->pid);

		switch(thisPCB->state) {
			case STATE_NEW:
				printf("state: new, ");
				break;
			case STATE_READY:
				printf("state: ready, ");
				break;
			case STATE_RUNNING:
				printf("state: running, ");
				break;
			case STATE_INT:
				printf("state: interrupted, ");
				break;
			case STATE_WAIT:
				printf("state: waiting, ");
				break;
			case STATE_HALT:
				printf("state: halted, ");
				break;
		}
		
		switch(thisPCB->role) {
			case COMP:
				printf("role: comp, ");
				break;
			case IO:
				printf("role: io, ");
				break;
			case PAIR:
				printf("role: pair, "); //producer/consumer
				break;
			case SHARED:
				printf("role: shared, ");
				break;
		}
		
		printf("priority: %d, ", thisPCB->priority);
		printf("PC: %d, ", thisPCB->context->pc);
		if (thisPCB->role == PAIR) {
			printf("isProducer: %d, ", thisPCB->isProducer);
		}
		//do it like IO if its PAIR or SHARED to show mutex positions
		
		printf("\r\nMAX PC: %d\r\n", thisPCB->max_pc);
		
		if (thisPCB->role == IO) {
			printf("io_1 traps\r\n");
			for (int i = 0; i < TRAP_COUNT; i++) {
				printf("%d ", thisPCB->io_1_traps[i]);
			}
			printf("\r\nio_2 traps\r\n");
			for (int i = 0; i < TRAP_COUNT; i++) {
				printf("%d ", thisPCB->io_2_traps[i]);
			}
			printf("\r\n");
		} else if (thisPCB->role == SHARED) {
			printf("mutex_r1 locks\r\n");
			printPCLocations(thisPCB->lockR1);
			printf("mutex_r1 unlocks\r\n");
			printPCLocations(thisPCB->unlockR1);
			printf("mutex_r2 locks\r\n");
			printPCLocations(thisPCB->lockR2);
			printf("mutex_r2 unlocks\r\n");
			printPCLocations(thisPCB->unlockR2);
		}  else if (thisPCB->role == PAIR) {
			if (thisPCB->isProducer)  {
				printf("cond_var signals\r\n");
				printPCLocations(thisPCB->signal_cond);
			} else {
				printf("cond_var waits\r\n");
				printPCLocations(thisPCB->wait_cond);
			}
		}
		printf("terminate: %d\r\n", thisPCB->terminate);
		printf("term_count: %d\r\n", thisPCB->term_count);
		printf("\r\n");
		
		if (showCpu) {
			printf("mem: 0x%04X, ", thisPCB->mem);
			printf("size: %d, ", thisPCB->size);
			printf("channel_no: %d ", thisPCB->channel_no);
			toStringCPUContext(thisPCB->context);
		}
	} else {
		printf("PCB is null\r\n");
	}
}


void toStringCPUContext(CPU_context_p context) {
	printf(" CPU context values: ");
	printf("ir:  %d, ", context->ir);
	printf("psr: %d, ", context->psr);
	printf("r0:  %d, ", context->r0);
	printf("r1:  %d, ", context->r1);
	printf("r2:  %d, ", context->r2);
	printf("r3:  %d, ", context->r3);
	printf("r4:  %d, ", context->r4);
	printf("r5:  %d, ", context->r5);
	printf("r6:  %d, ", context->r6);
	printf("r7:  %d\r\n", context->r7);
}
 

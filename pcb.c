/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */

#include"pcb.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

int global_largest_PID = 0;

/*
 * Helper function to iniialize PCB data.
 */
void initialize_data(/* in-out */ PCB pcb) {
	pcb->pid = 0;
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
	pcb->term_count = 0;
  
	//time_t t;
	//srand((unsigned) time(&t));
	populateIOTraps (pcb, 0); // populates io_1_traps
	populateIOTraps (pcb, 1); // populates io_2_traps
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
    PCB new_pcb = malloc(sizeof(PCB_s));
    if (new_pcb != NULL) {
        new_pcb->context = malloc(sizeof(CPU_context_s));
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
	if (pcb->context) {
	  free(pcb->context);
	  printf("breaks\n");
	}
	  free(pcb);// that thing
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
	
	printf("priority: %d, ", thisPCB->priority);
	printf("PC: %d, ", thisPCB->context->pc);
	
	printf("\r\nMAX PC: %d\r\n", thisPCB->max_pc);
	printf("io_1_traps\n");
	for (int i = 0; i < TRAP_COUNT; i++) {
		printf("%d ", thisPCB->io_1_traps[i]);
	}
	printf("\r\nio_2_traps\r\n");
	for (int i = 0; i < TRAP_COUNT; i++) {
		printf("%d ", thisPCB->io_2_traps[i]);
	}
	printf("\r\nterminate: %d\r\n", thisPCB->terminate);
	printf("term_count: %d\r\n", thisPCB->term_count);
	printf("\r\n");
	
	if (showCpu) {
		printf("mem: 0x%04X, ", thisPCB->mem);
		printf("parent: %d, ", thisPCB->parent);
		printf("size: %d, ", thisPCB->size);
		printf("channel_no: %d ", thisPCB->channel_no);
		toStringCPUContext(thisPCB->context);
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
 

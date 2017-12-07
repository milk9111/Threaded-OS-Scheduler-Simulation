/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman, Jasmine Dacones
 */

#ifndef PCB_H  /* Include guard */
#define PCB_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define NUM_PRIORITIES 16
#define TRAP_COUNT 4
#define LARGEST_PC_POSSIBLE 300
#define SMALLEST_PC_POSSIBLE 30
#define MAX_TERM_COUNT 3
#define MAX_DIVIDER 16

#define ROLE_PERCENTAGE_MAX_RANGE 200
#define COMP_ROLE_MAX_RANGE 100
#define IO_ROLE_MIN_RANGE 101
#define IO_ROLE_MAX_RANGE 150
#define PAIR_ROLE_MIN_RANGE 151
#define PAIR_ROLE_MAX_RANGE 175

#define MAX_PC_RANGE 50



/* The CPU state, values named as in the LC-3 processor. */
typedef struct cpu_context {
    unsigned int pc;
    unsigned int ir;
    unsigned int psr;
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
} CPU_context_s; // _s means this is a structure definition

typedef CPU_context_s * CPU_context_p; // _p means that this is a pointer to a structure

/* enum for various process states. */
enum state_type {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_INT,
    STATE_WAIT,
    STATE_HALT
};


/*
	COMP = Computationally heavy
	IO = Lots of IO Traps
	PAIR = Producer/Consumer
	SHARED = Shared Resource pair
*/
enum pcb_type {
	COMP,
	IO,
	PAIR,
	SHARED
};

/* Process Control Block - Contains info required for executing processes. */
typedef struct pcb {
    unsigned int pid; // process identification
    enum state_type state; // process state (running, waiting, etc.)
	enum pcb_type role; // type of role in simulation
    unsigned int parent; // parent process pid
    unsigned char priority; // 0 is highest – 15 is lowest.
    unsigned char * mem; // start of process in memory
    unsigned int size; // number of bytes in process
    unsigned char channel_no; // which I/O device or service Q
	unsigned int max_pc; // this is essentially the quantum size
	unsigned int creation;
	unsigned int termination;
	unsigned int terminate;
	unsigned int term_count;
	unsigned int io_1_traps[TRAP_COUNT];
	unsigned int io_2_traps[TRAP_COUNT];
	unsigned int blocked_timer;
	
	unsigned int lock_pc; //for mutex
	unsigned int unlock_pc;
	
	unsigned int signal_pc; //for condition variable
	unsigned int wait_pc;
	
	unsigned int lockR1[TRAP_COUNT];
	unsigned int lockR2[TRAP_COUNT];
	unsigned int unlockR1[TRAP_COUNT];
	unsigned int unlockR2[TRAP_COUNT];

	unsigned int wait_cond[TRAP_COUNT];
	unsigned int signal_cond[TRAP_COUNT];
	
	unsigned int mutex_R1_id;
	unsigned int mutex_R2_id;

	int isProducer;
	int isConsumer;
	
    // if process is blocked, which queue it is in
    CPU_context_p context; // set of cpu registers
    // other items to be added as needed.
} PCB_s;

typedef PCB_s * PCB;


typedef struct COND_VAR {
	int signal;
} cond_var_s;

typedef cond_var_s * ConditionVariable;


typedef struct MUTEX {
	int isLocked;
	unsigned int mid; //mutex id
	PCB pcb1;
	PCB pcb2;
	PCB hasLock;
	PCB blocked;
	ConditionVariable condVar;
} mutex_s;

typedef mutex_s * Mutex;



Mutex mutex_create ();

void mutex_init (Mutex);

void toStringMutex (Mutex mutex);

void mutex_destroy (Mutex mutex);

int mutex_lock (Mutex mutex, PCB pcb);

int mutex_unlock (Mutex mutex, PCB pcb);

int mutex_trylock (Mutex mutex, PCB pcb);

void printPCLocations (unsigned int pcLocs[]);


ConditionVariable cond_var_create ();

void cond_var_init (ConditionVariable);

void toStringConditionVariable (ConditionVariable);

void cond_var_destroy (ConditionVariable);

void cond_var_signal (ConditionVariable);

int cond_var_wait (ConditionVariable);


/*
 * Allocate a PCB and a context for that PCB.
 *
 * Return: NULL if context or PCB allocation failed, the new pointer otherwise.
 */
PCB PCB_create();


enum pcb_type chooseRole();


void initialize_pcb_type (PCB pcb, int isFirst, Mutex sharedMutexR1, Mutex sharedMutexR2);

/*
 * Frees a PCB and its context.
 *
 * Arguments: pcb: the pcb to free.
 */
void PCB_destroy(/* in-out */ PCB pcb);

/*
 * Assigns intial process ID to the process.
 *
 * Arguments: pcb: the pcb to modify.
 */
void PCB_assign_PID(/* in */ PCB pcb);

/*
 * Sets the state of the process to the provided state.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new state of the process.
 */
void PCB_assign_state(/* in-out */ PCB pcb, /* in */ enum state_type state);

/*
 * Sets the parent of the given pcb to the provided pid.
 *
 * Arguments: pcb: the pcb to modify.
 *            pid: the parent PID for this process.
 */
void PCB_assign_parent(PCB the_pcb, int pid);

/*
 * Sets the priority of the PCB to the provided value.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new priority of the process.
 */
void PCB_assign_priority(/* in */ PCB pcb, /* in */ unsigned int priority);

int ioTrapContains(unsigned int, unsigned int[]);

unsigned int makeMaxPC();

void populateMutexLocations (PCB pcb);

void populateIOTraps (PCB, int);

void populateMutexTraps1221(PCB pcb, int step);

void populateMutexTraps2112(PCB pcb, int step);

void populateProducerConsumerTraps(PCB pcb, int step, int type);

/*
 * Create and return a string representation of the provided PCB.
 *
 * Arguments: pcb: the pcb to create a string representation of.
 * Return: a string representation of the provided PCB on success, NULL otherwise.
 */
void toStringPCB(/* in */ PCB pcb, int showCpu);

void toStringCPUContext(CPU_context_p context);

#endif

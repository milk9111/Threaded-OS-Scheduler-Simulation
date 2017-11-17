/*
	10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
	
	In this project we will be making an MLFQ scheduling algorithm
	that will take a PriorityQueue of PCBs and run them through our scheduler.
	It will simulate the "running" of the process by incrementing the running PCB's PCB 
	by one on each loop through. It will also incorporate various interrupts to show 
	the effects it has on the scheduling simulator.
	
	This file holds the definitions of structs and declarations functions for the 
	scheduler.c file.
*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

//includes
#include "priority_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//defines
#define MAX_PCB_TOTAL 30
#define RESET_COUNT 15
#define MAX_PCB_IN_ROUND 5
#define MAX_PC_JUMP 4000
#define MIN_PC_JUMP 3000
#define PC_JUMP_LIMIT 999
#define MAKE_PCB_CHANCE_DOMAIN 100
#define TIMER_RANGE 3
#define MAKE_PCB_CHANCE_PERCENTAGE 10
#define IS_TIMER 1
#define IS_IO_TRAP 2
#define IS_IO_INTERRUPT 3
#define IS_TERMINATING -1
#define SWITCH_CALLS 4
#define MAX_VALUE_PRIVILEGED 15
#define RANDOM_VALUE 101
#define TOTAL_TERMINATED 10
#define MAX_PRIVILEGE 4



//structs
typedef struct scheduler {
	ReadyQueue created;
	ReadyQueue killed;
	ReadyQueue blocked;
	ReadyQueue mutexes;
	PriorityQueue ready;
	PCB running;
	PCB interrupted;
	int isNew;
} scheduler_s;

typedef scheduler_s * Scheduler;


//declarations
int makePCBList (Scheduler);

unsigned int runProcess (unsigned int, int);

void pseudoISR (Scheduler, int);

void scheduling (int, Scheduler);

void dispatcher (Scheduler);

void pseudoIRET (Scheduler);

void printSchedulerState (Scheduler);

Scheduler schedulerConstructor ();

void schedulerDeconstructor (Scheduler);

int isPrivileged(PCB pcb);

void terminate(Scheduler theScheduler);

void resetMLFQ(Scheduler theScheduler);

void resetReadyQueue (ReadyQueue queue);

void osLoop ();

int timerInterrupt (int);

int ioTrap (PCB);

int ioInterrupt (ReadyQueue);

void incrementRoleCount (enum pcb_type);

void displayRoleCountResults();


#endif
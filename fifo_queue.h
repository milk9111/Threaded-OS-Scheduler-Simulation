/*
	10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman, Jasmine Dacones
 */

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include "pcb.h"
#include <stdlib.h>
#include <string.h>
/* primarily for sprintf */
#include <stdio.h>

/* A node used in a fifo queue to store data, and the next node. */
typedef struct node {
    struct node * next;
    PCB pcb;
	Mutex mutex;
} Node_s;

typedef Node_s * ReadyQueueNode;

/* A fifo queue, which stores size and pointers to the first and last nodes. */
typedef struct fifo_queue {
    ReadyQueueNode first_node;
    ReadyQueueNode last_node;
	unsigned int quantum_size;
    unsigned int size;
} FIFOq_s;

typedef FIFOq_s * ReadyQueue;

/*
 * Create a new FIFO Queue.
 *
 * Return: a pointer to a new FIFO queue, NULL if unsuccessful.
 */
ReadyQueue q_create();

void printMutexList (ReadyQueue mutexes);

/*
 * Destroy a FIFO queue and all of its internal nodes.
 *
 * Arguments: FIFOq: the queue to destroy.
 * This will also free all PCBs, to prevent any leaks. Do not use on a non empty queue if processing is still going to occur on a pcb.
 */
void q_destroy(/* in-out */ ReadyQueue FIFOq);


void q_destroy_m(/* in-out */ ReadyQueue FIFOq);

/*
 * Checks if a queue is empty.
 *
 * Arguments: FIFOq: the queue to test.
 * Return: 1 if empty, 0 otherwise.
 */
char q_is_empty(/* in */ ReadyQueue FIFOq);

void setQuantumSize (ReadyQueue queue, int quantumSize);

/*
 * Attempts to enqueue the provided pcb.
 *
 * Arguments: FIFOq: the queue to enqueue to.
 *            pcb: the PCB to enqueue.
 * Return: 1 if successful, 0 if unsuccessful.
 */
int q_enqueue(/* in */ ReadyQueue FIFOq, /* in */ PCB pcb);

/*
 * Dequeues and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to dequeue from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB q_dequeue(/* in-out */ ReadyQueue FIFOq);

/*
 * Attempts to enqueue the provided Mutex.
 *
 * Arguments: FIFOq: the queue to enqueue to.
 *            pcb: the PCB to enqueue.
 * Return: 1 if successful, 0 if unsuccessful.
 */
int q_enqueue_m(/* in */ ReadyQueue FIFOq, /* in */ Mutex mutex);

/*
 * Dequeues and returns a Mutex from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to dequeue from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
Mutex q_dequeue_m(/* in-out */ ReadyQueue FIFOq);


Mutex q_find_mutex (ReadyQueue queue, PCB pcb);

/*
 * Peeks and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to peek from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB q_peek(ReadyQueue FIFOq);


int q_contains_mutex (ReadyQueue queue, Mutex toFind);

int q_contains (ReadyQueue FIFOq, PCB pcb);

/*
 * Creates and returns an output string representation of the FIFO queue.
 *
 * Arguments: FIFOq: The queue to perform this operation on
 *            display_back: 1 to display the final PCB, 0 otherwise.
 * Return: a string of the contents of this FIFO queue. User is responsible for
 * freeing consumed memory.
 */
void toStringReadyQueue(/* in */ ReadyQueue FIFOq);

void toStringReadyQueueNode(ReadyQueueNode theNode, int);

void toStringReadyQueueMutexes(ReadyQueue theQueue);


#endif

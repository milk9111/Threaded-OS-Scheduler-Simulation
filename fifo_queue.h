/*
	10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include "pcb.h"

/* A node used in a fifo queue to store data, and the next node. */
typedef struct node {
    struct node * next;
    PCB         pcb;
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

/*
 * Destroy a FIFO queue and all of its internal nodes.
 *
 * Arguments: FIFOq: the queue to destroy.
 * This will also free all PCBs, to prevent any leaks. Do not use on a non empty queue if processing is still going to occur on a pcb.
 */
void q_destroy(/* in-out */ ReadyQueue FIFOq);

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
 * Peeks and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to peek from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB q_peek(ReadyQueue FIFOq);

/*
 * Creates and returns an output string representation of the FIFO queue.
 *
 * Arguments: FIFOq: The queue to perform this operation on
 *            display_back: 1 to display the final PCB, 0 otherwise.
 * Return: a string of the contents of this FIFO queue. User is responsible for
 * freeing consumed memory.
 */
void toStringReadyQueue(/* in */ ReadyQueue FIFOq);

void toStringReadyQueueNode(ReadyQueueNode theNode);

/*
 * Helper function that resizes a malloced block of memory if the requested
 *   ending position would exceed the block's capacity.
 * I want to pull this into another header/source file, as it can be very useful elsewhere,
 *   but only supposed to submit the 6.
 * Very useful for a variety of things, primarily used in string methods at the moment.
 *
 * Arguments: in_ptr: the pointer to resize.
 *            end_pos: the final desired available position in the resized pointer.
 *            capacity: a pointer to the current capacity, will be changed if resized.
 * Return: NULL if failure, the new (possibly same) pointer otherwise.
 */
void * resize_block_if_needed(/* in */ void * in_ptr, /* in */ unsigned int end_pos, /* in-out */ unsigned int * capacity);


#endif

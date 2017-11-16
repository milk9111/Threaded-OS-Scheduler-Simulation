/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "pcb.h"
#include "fifo_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct priority_queue {
    ReadyQueue     queues[NUM_PRIORITIES];
} PQ_s;

typedef struct priority_queue * PriorityQueue;


/*
 * Creates a priority queue.
 *
 * Return: A new priority queue on success, NULL on failure.
 */
PriorityQueue pq_create();

/*
 * Destroys the provided priority queue, freeing all contents.
 *
 * Arguments: PQ: The Priority Queue to destroy.
 */
void pq_destroy(PriorityQueue PQ);

/*
 * Enqueues a PCB to the provided priority queue, into the correct priority bin.
 *
 * Arguments: PQ: The Priority Queue to enqueue to.
 *            pcb: the PCB to enqueue.
 */
void pq_enqueue(PriorityQueue PQ, PCB pcb);

/*
 * Dequeues a PCB from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to dequeue from.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
PCB pq_dequeue(PriorityQueue PQ);

int getNextQuantumSize (PriorityQueue PQ);

/*
 * Peeks at the top value from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to peek at.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
 PCB pq_peek(PriorityQueue PQ);

/*
 * Checks if the provided priority queue is empty.
 *
 * Arguments: PQ: The Priority Queue to test.
 * Return: 1 if the queue is empty, 0 otherwise.
 */
char pq_is_empty(PriorityQueue PQ);

/*
 * Creates a string representation of the provided priority queue, and returns it.
 *
 * Arguments: PQ: the Priority Queue to create a string representation of.
 * Return: A string representation of the provided Priority Queue, or NULL on failure.
 */
void toStringPriorityQueue(PriorityQueue PQ);

#endif

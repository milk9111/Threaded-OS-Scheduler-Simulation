/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "priority_queue.h"

#define ADDITIONAL_ROOM_FOR_TOSTR 4
#define PRIORITY_JUMP_EXTRA 1000
#define MIN_PRIORITY_JUMP 500

/*
 * Creates a priority queue.
 *
 * Return: A new priority queue on success, NULL on failure.
 */
PriorityQueue pq_create() {
    int i, failed = -1;
    PriorityQueue new_pq = malloc(sizeof(PQ_s));

    if (new_pq != NULL) {
        for (i = 0; i < NUM_PRIORITIES; i++) {
            new_pq->queues[i] = q_create();
            if (new_pq->queues[i] == NULL) {
                failed = i;
                break;
            }
			if (!i) {
				setQuantumSize(new_pq->queues[i], MIN_PRIORITY_JUMP);
			} else {
				setQuantumSize(new_pq->queues[i], i * PRIORITY_JUMP_EXTRA);
			}
        }
        /* If failed is non-zero, we need to free up everything else. */
        for (i = 0; i <= failed; i++) {
            q_destroy(new_pq->queues[i]);
        }
        /* Simlarly, if it is true, we need to free the priority queue. */
        if (failed != -1) {
            free(new_pq);
            new_pq = NULL;
        }
    }

    return new_pq;
}


int getNextQuantumSize (PriorityQueue PQ) {
	int qSize = 0;
	for (int i = 0; i < NUM_PRIORITIES; i++) {
		ReadyQueue curr = PQ->queues[i];
		if (!q_is_empty(curr)) {
			qSize = curr->quantum_size;
			break;
		}
	}
	return qSize;
}

/*
 * Destroys the provided priority queue, freeing all contents.
 *
 * Arguments: PQ: The Priority Queue to destroy.
 */
void pq_destroy(PriorityQueue PQ) {
    int i;

    /* Free all our inner FIFO queues. */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        q_destroy(PQ->queues[i]);
    }

    free(PQ);
}

/*
 * Enqueues a PCB to the provided priority queue, into the correct priority bin.
 *
 * Arguments: PQ: The Priority Queue to enqueue to.
 *            pcb: the PCB to enqueue.
 */
void pq_enqueue(PriorityQueue PQ, PCB pcb) {
    q_enqueue(PQ->queues[pcb->priority], pcb);
}

/*
 * Dequeues a PCB from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to dequeue from.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
PCB pq_dequeue(PriorityQueue PQ) {
    int i;
    PCB ret_pcb = NULL;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!q_is_empty(PQ->queues[i])) {
            ret_pcb = q_dequeue(PQ->queues[i]);
            break;
        }
    }
    return ret_pcb;
}

/*
 * Checks if the provided priority queue is empty.
 *
 * Arguments: PQ: The Priority Queue to test.
 * Return: 1 if the queue is empty, 0 otherwise.
 */
char pq_is_empty(PriorityQueue PQ) {
    int i;
    char ret_val = 1;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        /* If a single queue isn't empty, the priority queue isn't empty. */
        if (q_is_empty(PQ->queues[i]) == 0) {
            ret_val = 0;
            break;
        }
    }

    return ret_val;
}


/*
 * Peeks at the top value from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to peek at.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
 PCB pq_peek(PriorityQueue PQ) {
	PCB pcb = NULL;
	int i = 0;
	
	if (!pq_is_empty(PQ)) {
		while (i < NUM_PRIORITIES) {
			if (!q_is_empty(PQ->queues[i])) {
				pcb = q_peek(PQ->queues[i]);
				break;
			} else {
				i++;
			}
		}
	}
	return pcb;
}
 

/*
 * Creates a string representation of the provided priority queue, and returns it.
 *
 * Arguments: PQ: the Priority Queue to create a string representation of.
 */
 void toStringPriorityQueue(PriorityQueue PQ) {
	printf("\r\n");
	for (int i = 0; i < NUM_PRIORITIES; i++) {
		printf("Q%2d: Count=%d, QuantumSize=%d\r\n", i, PQ->queues[i]->size, PQ->queues[i]->quantum_size);
		//toStringReadyQueue(PQ->queues[i]);
	}
	printf("\r\n");
 }
 
/*char * toStringPriorityQueue(PriorityQueue PQ, int display_back) {
    unsigned int buff_len = 1000;
    unsigned int cpos = 0;
    unsigned int q_str_len = 0;
    char * ret_str = malloc(sizeof(char) * buff_len);
    char * str_resize = NULL;
    char * q_str = NULL;
    int i;

    if (ret_str != NULL) {
        for (i = 0; i < NUM_PRIORITIES; i++) {
            q_str = toStringReadyQueue(PQ->queues[i], display_back);
            if (q_str != NULL) {
                q_str_len = strlen(q_str);
                str_resize = resize_block_if_needed(ret_str, cpos + q_str_len
                                        + ADDITIONAL_ROOM_FOR_TOSTR, &buff_len);
                if (str_resize != NULL) {
                    ret_str = str_resize;
                    cpos += sprintf(ret_str + cpos, "Q%d:%s\n", i, q_str);
                }
                free(q_str);
            }
        }
    }

    return ret_str;
}*/

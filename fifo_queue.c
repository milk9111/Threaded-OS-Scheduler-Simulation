/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */

#include <stdlib.h>
#include <string.h>
/* primarily for sprintf */
#include <stdio.h>

#include "fifo_queue.h"

#define PROCESS_QUEUE_DISPLAY_LENGTH 20
#define POST_OUTPUT_BUFFER 5



/*
 * Create a new FIFO Queue.
 *
 * Return: a pointer to a new FIFO queue, NULL if unsuccessful.
 */
ReadyQueue q_create() {
    ReadyQueue new_queue = malloc(sizeof(FIFOq_s));

    if (new_queue != NULL) {
        new_queue->first_node = NULL;
        new_queue->last_node = NULL;
		new_queue->quantum_size = 0;
        new_queue->size = 0;
    }

    return new_queue;
}

/*
 * Destroy a FIFO queue and all of its internal nodes.
 *
 * Arguments: FIFOq: the queue to destroy.
 * This will also free all PCBs, to prevent any leaks. Do not use on a non empty queue if processing is still going to occur on a pcb.
 */
void q_destroy(/* in-out */ ReadyQueue FIFOq) {
    ReadyQueueNode iter = FIFOq->first_node;
    ReadyQueueNode curr = NULL;

    while (iter != NULL) {
        curr = iter;
        iter = iter->next;
        PCB_destroy(curr->pcb);
        free(curr);
    }
    free(FIFOq);
}


void setQuantumSize (ReadyQueue queue, int quantumSize) {
	queue->quantum_size = quantumSize;
}


/*
 * Peeks and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to peek from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB q_peek(ReadyQueue FIFOq) {
	PCB headPCB = NULL;
	if (FIFOq->first_node) {
		headPCB = FIFOq->first_node->pcb;
	}
	return headPCB;
}

/*
 * Checks if a queue is empty.
 *
 * Arguments: FIFOq: the queue to test.
 * Return: 1 if empty, 0 otherwise.
 */
char q_is_empty(/* in */ ReadyQueue FIFOq) {
    return (FIFOq->first_node == NULL);
}

/*
 * Attempts to enqueue the provided pcb.
 *
 * Arguments: FIFOq: the queue to enqueue to.
 *            pcb: the PCB to enqueue.
 * Return: 1 if successful, 0 if unsuccessful.
 */
int q_enqueue(/* in */ ReadyQueue FIFOq, /* in */ PCB pcb) {
    ReadyQueueNode new_node = malloc(sizeof(Node_s));

    if (new_node != NULL && pcb != NULL) {
        new_node->pcb = pcb;
        new_node->next = NULL;

        if (FIFOq->last_node != NULL) {
			FIFOq->last_node->next = new_node;
			FIFOq->last_node = FIFOq->last_node->next;
            FIFOq->last_node->next = NULL;
        } else {
            FIFOq->first_node = new_node;
            FIFOq->last_node = new_node;
        }

        FIFOq->size++;
    }

    return new_node != NULL;
}

/*
 * Dequeues and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to dequeue from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB q_dequeue(/* in-out */ ReadyQueue FIFOq) {
    PCB ret_pcb = NULL;
    ReadyQueueNode ret_node = FIFOq->first_node;

    if (ret_node != NULL) {
        FIFOq->first_node = ret_node->next;

        FIFOq->size--;

        /* If the user has dequeued the final node, set the last node to null. */
        if (FIFOq->first_node == NULL) {
            FIFOq->last_node = NULL;
        }

        ret_pcb = ret_node->pcb;

        free(ret_node);
    }

    return ret_pcb;
}

/*
 * Creates and returns an output string representation of the FIFO queue.
 *
 * Arguments: FIFOq: The queue to perform this operation on
 *            display_back: 1 to display the final PCB, 0 otherwise.
 */
 void toStringReadyQueueNode(ReadyQueueNode theNode) {
	printf("P%d",theNode->pcb->pid);
    if(theNode->next != 0) {
        printf("->");
    } else {
        printf("->*");
    }
}

void toStringReadyQueue(ReadyQueue theQueue) {
    if(theQueue->first_node == 0) {
        printf("\r\n");
    } else {
        ReadyQueueNode temp = theQueue->first_node;
        while(temp != 0) {
            toStringReadyQueueNode(temp);
            temp = temp->next;
        }
		printf("\r\n");
    }
}
/*char * toStringReadyQueue(/* in  ReadyQueue FIFOq, /* in *char display_back) {
    ReadyQueueNode iter = FIFOq->first_node;

    unsigned int buff_len = 1000;
    unsigned int cpos = 0;
    unsigned int pcb_str_len = 0;
    char * ret_str = malloc(sizeof(char) * buff_len);
    char * str_resize = NULL;

    /* Check if we successfully malloc'd 
    if (ret_str != NULL) {
        /* Initial size is 1000, this should be safe: 
        cpos += sprintf(ret_str, "Q:Count=%u: ", FIFOq->size);

        /* While we have nodes to iterate through: 
        while (iter != NULL) {
            /* Make sure we have enough capacity to sprintf. 
            str_resize = resize_block_if_needed(ret_str, cpos + PROCESS_QUEUE_DISPLAY_LENGTH, &buff_len);
            if (str_resize != NULL) {
                /* If it succeeded, we need to shift to the (possibly same) pointer location. 
                ret_str = str_resize;
                cpos += sprintf(ret_str + cpos, "P%u-", iter->pcb->pid);
                if (iter->next != NULL) {
                    cpos += sprintf(ret_str + cpos, ">");
                } else {
                    cpos += sprintf(ret_str + cpos, "*");
                }
            } else {
                /* If it failed, might as well end the loop. 
                break;
            }
            iter = iter->next;
        }

        /* Write the last PCB to our string: 
        if (FIFOq->last_node != NULL && display_back == 1) {
             There is enough space in PROCESS_QUEUE_DISPLAY_LENGTH to allow for this addition without any additional change 
            cpos += sprintf(ret_str + cpos, " : ");
            char * PCB_string = toStringPCB(FIFOq->last_node->pcb, 1);

            if (PCB_string != NULL) {
                pcb_str_len = strlen(PCB_string);
                str_resize = resize_block_if_needed(ret_str, cpos + pcb_str_len + POST_OUTPUT_BUFFER, &buff_len);
                if (str_resize != NULL) {
                    ret_str = str_resize;
                    sprintf(ret_str + cpos, "%s", PCB_string);
                }
                free(PCB_string);
            }
        }
    }
    /* Finally, return the string: 
    return ret_str;
}*/

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
void * resize_block_if_needed(/* in */ void * in_ptr, /* in */ unsigned int end_pos, /* in-out */ unsigned int * capacity) {
    unsigned int new_capacity = end_pos * 2;
    void * ptr_resize = NULL;

    if (end_pos >= *capacity) {
        ptr_resize = realloc(in_ptr, sizeof(char) * new_capacity);
        if (ptr_resize != NULL) {
            *capacity = new_capacity;
        }
    } else {
        ptr_resize = in_ptr;
    }

    return ptr_resize;
}


#undef PROCESS_QUEUE_DISPLAY_LENGTH
#undef POST_OUTPUT_BUFFER

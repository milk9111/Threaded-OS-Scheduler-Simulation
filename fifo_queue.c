/*
    10/28/2017
	Authors: Connor Lundberg, Jacob Ackerman
 */


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
		curr = NULL;
    }
    free(FIFOq);
	FIFOq = NULL;
}


/*
 * Destroy a FIFO queue and all of its internal nodes.
 *
 * Arguments: FIFOq: the queue to destroy.
 * This will also free all PCBs, to prevent any leaks. Do not use on a non empty queue if processing is still going to occur on a pcb.
 */
void q_destroy_m(/* in-out */ ReadyQueue FIFOq) {
    ReadyQueueNode iter = FIFOq->first_node;
    ReadyQueueNode curr = NULL;

    while (iter != NULL) {
        curr = iter;
        iter = iter->next;
        mutex_destroy(curr->mutex);
        free(curr);
		curr = NULL;
    }
    free(FIFOq);
	FIFOq = NULL;
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
 * Attempts to enqueue the provided pcb.
 *
 * Arguments: FIFOq: the queue to enqueue to.
 *            pcb: the PCB to enqueue.
 * Return: 1 if successful, 0 if unsuccessful.
 */
int q_enqueue_m(/* in */ ReadyQueue FIFOq, /* in */ Mutex mutex) {
    ReadyQueueNode new_node = malloc(sizeof(Node_s));

    if (new_node != NULL && mutex != NULL) {
        new_node->mutex = mutex;
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
 * Dequeues and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to dequeue from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
Mutex q_dequeue_m(/* in-out */ ReadyQueue FIFOq) {
    Mutex ret_mutex = NULL;
    ReadyQueueNode ret_node = FIFOq->first_node;

    if (ret_node != NULL) {
        FIFOq->first_node = ret_node->next;

        FIFOq->size--;

        /* If the user has dequeued the final node, set the last node to null. */
        if (FIFOq->first_node == NULL) {
            FIFOq->last_node = NULL;
        }

        ret_mutex = ret_node->mutex;

        free(ret_node);
    }

    return ret_mutex;
}


/*
	This will find the Mutex that holds the given PCB, NULL otherwise.
*/
Mutex q_find_mutex (ReadyQueue queue, PCB pcb) {
	Mutex ret_mutex = NULL;
	
	ReadyQueueNode curr = queue->first_node;
	ReadyQueueNode last = NULL;
	while (curr) {
		if (curr->mutex->pcb1->pid == pcb->pid || curr->mutex->pcb2->pid == pcb->pid) { //if either of the PCB ids match
			ret_mutex = curr->mutex;
			
			/*if (curr == queue->first_node) { //if the mutex is the first node in the list
				queue->first_node = curr->next;
			} else { //otherwise
				last->next = curr->next;
			}
			
			free(curr);*/
			break;
		}
		
		last = curr;
		curr = curr->next;
	}
	
	return ret_mutex;
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


#undef PROCESS_QUEUE_DISPLAY_LENGTH
#undef POST_OUTPUT_BUFFER

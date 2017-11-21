// mutex map testing file
// for testing purposes only

#include "mutex_map.h"
#include "pcb.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	printf("Beging testing...\n");
	MutexMap myMap = create_mutx_map();
	
	printf("Insertion test - insert new Mutex's until map is full\n");
	int i;
	for(i = 0; i < 201; i++)
	{
		PCB tmp = PCB_create();
		Mutex temp = mutex_init();
		temp->pcb1 = tmp;
		temp->pcb2 = tmp;
		int result = add_to_mutx_map(myMap, temp, tmp);
		printf("Insertion result = %d\n", result);
	}
	
	printf("\n-------------------------\nDeletion test - remove half the mutexes from the map, semi-randomly\n");
	for(i = 0; i < 100; i++)
	{
		PCB tmp = PCB_create();
		tmp->pid = i * 3;
		int result = remove_from_mutx_map(myMap, tmp);
		printf("Deletion result = %d\n", result);
	}
	
	printf("\n-------------------------\nInsertion collision handling test\n");
	for(i = 0; i < 101; i++)
	{
		PCB tmp = PCB_create();
		Mutex temp = mutex_init();
		temp->pcb1 = tmp;
		temp->pcb2 = tmp;
		int result = add_to_mutx_map(myMap, temp, tmp);
		printf("Insertion result = %d, Insertion pid = %d\n", result, tmp->pid);
	}
	
	printf("\n-------------------------\nget_mutx() test, no collision.\n");
	PCB tmp1 = PCB_create();
	tmp1->pid = 2;
	Mutex testMutex = get_mutx(myMap, tmp1);
	printf("Mutex pcb1 pid: %d vs. pcb pid actual: %d.\n", testMutex->pcb1->pid, tmp1->pid);
	
	printf("\n-------------------------\nget_mutx() test, with collision.\n");
	PCB tmp2 = PCB_create();
	tmp2->pid = 397;
	testMutex = get_mutx(myMap, tmp2);
	printf("Mutex pcb1 pid: %d vs. pcb pid actual: %d.\n", testMutex->pcb1->pid, tmp2->pid);
	
}
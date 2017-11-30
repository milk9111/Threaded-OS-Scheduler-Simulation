// mutex map testing file
// for testing purposes only

#include "mutex_map.h"
#include "pcb.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	setvbuf(stdout, NULL, _IONBF, 0);
	
	printf("Beging testing...\n");
	MutexMap myMap = create_mutx_map();
	
	printf("Insertion test - insert new Mutex's until map is full\n");
	int i;
	for(i = 0; i < 201; i++)
	{
		//PCB tmp = PCB_create();
		Mutex temp = mutex_create();
		//temp->mid = tmp;
		//temp->pcb2 = tmp;
		int result = add_to_mutx_map(myMap, temp, temp->mid);
		printf("Insertion result = %d\n", result);
	}
	
	printf("\n-------------------------\nDeletion test - remove half the mutexes from the map, semi-randomly\n");
	for(i = 0; i < 100; i++)
	{
		//PCB tmp = PCB_create();
		//tmp->pid = i * 3;
		int result = remove_from_mutx_map(myMap, i*3);
		printf("Deletion result = %d\n=====\n", result);
	}
	
	printf("\n-------------------------\nInsertion collision handling test\n");
	for(i = 0; i < 101; i++)
	{
		//PCB tmp = PCB_create();
		Mutex temp = mutex_create();
		//temp->pcb1 = tmp;
		//temp->pcb2 = tmp;
		int result = add_to_mutx_map(myMap, temp, temp->mid);
		printf("Insertion result = %d, Insertion pid = %d\n=====\n", result, temp->mid);
	}
	
	printf("\n-------------------------\nget_mutx() test, no collision.\n");
	//PCB tmp1 = PCB_create();
	//tmp1->pid = 2;
	int id = 2;
	Mutex testMutex = get_mutx(myMap, id);
	printf("Mutex pcb1 pid: %d vs. pcb pid actual: %d.\n", testMutex->mid, id);
	
	printf("\n-------------------------\nget_mutx() test, with collision.\n");
	
	id = 212;
	//PCB tmp2 = PCB_create();
	//tmp2->pid = 339;
	testMutex = get_mutx(myMap, id);
	if (testMutex) {
		printf("Mutex pcb1 pid: %d vs. pcb pid actual: %d.\n", testMutex->mid, id);
	} else {
		printf("Mutex was null\n");
	}
	
	printf("\n-------------------------\nTake and Remove function test. No collision.\n");
	//tmp1->pid = 160;
	id = 160;
	testMutex = take_n_remove_from_mutx_map(myMap, id);
	
	if (testMutex != NULL) {
		printf("Found pid: %d vs. expected pid: %d.\n", testMutex->mid, id);
	} else {
		printf("Mutex was null\n");
	}
	printf("Checking that Mutex has been removed properly.\n");
	testMutex = take_n_remove_from_mutx_map(myMap, id);
	
	if (testMutex != NULL) {
		printf("Found pid: %d vs. expected: NULL.\n", testMutex->mid);
	} else {
		printf("Mutex was null\n");
	}
	
	printf("\n-------------------------\nTake and Remove function test. Collision handling.\n");
	//tmp1->pid = 329;
	id = 240;
	testMutex = take_n_remove_from_mutx_map(myMap, id);
	
	if (testMutex != NULL) {
		printf("Found pid: %d vs. expected pid: %d.\n", testMutex->mid, id);
	} else {
		printf("Mutex was null\n");
	}
	printf("Checking that Mutex has been removed properly.\n");
	testMutex = take_n_remove_from_mutx_map(myMap, id);
	
	if (testMutex != NULL) {
		printf("Found pid: %d vs. expected: NULL.\n", testMutex->mid);
	} else {
		printf("Mutex was null\n");
	}
	
	toStringMutexMap(myMap);
}
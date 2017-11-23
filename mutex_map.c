#include "mutex_map.h"
#include <stdio.h>
#include <stdlib.h>


int findKey(PCB theKey, int mapSize);

MutexMap create_mutx_map()
{
	MutexMap newMap = (MutexMap) malloc(sizeof(mutex_map_s));
	int i;
	for (i = 0; i < MAX_INIT_BUCKETS; i++)
	{
		newMap->map[i] = NULL;
	}
	newMap->curr_map_size = MAX_INIT_BUCKETS;

	return newMap;
}


int add_to_mutx_map(MutexMap theMap, Mutex theMutex, PCB theKey)
{
	if (theMap == NULL || theMutex == NULL || theKey == NULL)
	{
		return 1;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	printf("Attempting to insert in location %d.\n", key);
	if(theMap->map[key] != NULL)
	{
		int tmp = key; // If we hit here, we need to resolve a hash collision
		printf("--Insertion collision!--\n");
		tmp++;
		while(theMap->map[tmp] != NULL)
		{
			if(tmp == key)
			{
				return 2; // Map is full. Will implement resizing if this becomes a problem.
			}
			tmp++;
			if (tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
		}
		key = tmp;
	}
	theMap->map[key] = theMutex;
	printf("Inserting at location %d.\n", key);
	return 0;
}


int remove_from_mutx_map(MutexMap theMap, PCB theKey)
{
	if (theMap == NULL || theKey == NULL)
	{
		return 1; // Missing value
	}
	int key = findKey(theKey, theMap->curr_map_size);
	printf("Attempting to remove from location %d.\n", key);
	printf("Starting point pid: %d, looking for: %d\n", theMap->map[key]->pcb1->pid, theKey->pid);
	if (theMap->map[key] != NULL)
	{
		if((theMap->map[key]->pcb1->pid != theKey->pid) 
			&& (theMap->map[key]->pcb1->pid != theKey->parent))
		{
			int tmp = key; // If we hit here, we had a hash collision in the past 
						   // and need to find where our mutex got placed
			printf("--Delete collision!--\n");
			
			
			printf("Obtained pid: %d, looking for: %d\n", theMap->map[tmp]->pcb1->pid, theKey->pid);
			printf("before tmp = %d\n", tmp);
			printf("before tmp + 1 = %d\n", tmp + 1);
			tmp++;
			printf("after tmp = %d\n", tmp);
			printf("Moving in...\n");
			//printf("pointer in location of tmp: %d", theMap->map[tmp]);
			
			while((theMap != NULL) && (theMap->map[tmp] != NULL) && (theMap->map[tmp]->pcb1->pid != theKey->pid) && (theMap->map[tmp]->pcb1->pid != theKey->parent))
			{	
				//printf("theMap == null: %d\n", (theMap == NULL));
				printf("map[tmp]->pcb1->pid: %d\n", theMap->map[tmp]->pcb1->pid);
				//printf("pcb1 == null: %d\n", (theMap->map[tmp]->pcb1 == NULL));
				//printf("theKey->pid: %d\n", theKey->pid);
				printf("Checking at location %d.\nObtained pid: %d, looking for: %d\n", tmp, theMap->map[tmp]->pcb1->pid, theKey->pid);
				printf("passed\n");
				/*if(theMap->map[tmp] == NULL || tmp == key)
				{
					printf("Not found\n");
					return 2; // Object does not exist
				}*/
				printf("passed\n");
				tmp++;
				printf("tmp: %d\n", tmp);
				if (tmp >= theMap->curr_map_size)
				{
					tmp = 0;
					printf("in here\n");
				}
				printf("tmp: %d\n", tmp);
				printf("next loop\n");
				
				printf("theMap->map[tmp]->pcb1->pid: %d\n", (theMap->map[tmp] == NULL));
				printf("theKey->pid: %d\n", theKey->pid);
				printf("theKey->parent: %d\n", theKey->parent);
				if(theMap->map[tmp] == NULL || tmp == key)
				{
					printf("Not found\n");
					return 2; // Object does not exist
				}
			}
			key = tmp;
			printf("here\n");
		}
		Mutex toFree = theMap->map[key];
		printf("Removing from location %d.\nObtained pid: %d, looking for: %d\n", key, toFree->pcb1->pid, theKey->pid);
		mutex_destroy(toFree);
		theMap->map[key] = NULL;
			
		return 0;
	}
	else
	{
		return 2;
	}
}


Mutex take_n_remove_from_mutx_map(MutexMap theMap, PCB theKey)
{
	if (theMap == NULL || theKey == NULL)
	{
		return NULL;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	if (theMap->map[key] == NULL)
	{
		return NULL;
	}
	printf("Attempting to remove from location %d.\n", key);
	if((theMap->map[key]->pcb1->pid != theKey->pid) && (theMap->map[key]->pcb1->pid != theKey->parent))
	{
		int tmp = key; // If we hit here, we had a hash collision in the past 
					   // and need to find where our mutex got placed
		tmp++;
		printf("Delete and Remove collision!\n");
		while((theMap->map[tmp] != NULL) 
			&& (theMap->map[tmp]->pcb1->pid != theKey->pid) 
			&& (theMap->map[tmp]->pcb1->pid != theKey->parent))
		{
			if(theMap->map[tmp] == NULL || tmp == key)
			{
				return NULL; // Object does not exist
			}
			tmp++;
			if (tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
		}
		key = tmp;
	}
	Mutex toFree = theMap->map[key];
	//mutex_destroy(toFree);
	theMap->map[key] = NULL;
	
	return toFree;
}


Mutex get_mutx(MutexMap theMap, PCB theKey)
{
	if (theMap == NULL || theKey == NULL)
	{
		return NULL;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	if (theMap->map[key] == NULL)
	{
		return NULL;
	}
	printf("Searching for mutex with key: %d.\n", key);
	if((theMap->map[key]->pcb1->pid != theKey->pid) 
		&& (theMap->map[key]->pcb1->pid != theKey->parent))
	{
		int tmp = key; // If we hit here, we had a hash collision in the past 
					   // and need to find where our mutex got placed
		tmp++;
		printf("--Search collision!--\n");
		printf("Want: %d, have: %d, also have: %d vs %d\n", theKey->pid, theMap->map[tmp]->pcb1->pid, theMap->map[tmp]->pcb2->parent, theKey->parent);
		while((theMap->map[tmp] != NULL) && (theMap->map[tmp]->pcb1->pid != theKey->pid) && (theMap->map[tmp]->pcb1->pid != theKey->parent))
		{
			//printf("Not found yet, moving on from %d...\n", tmp);
			if(theMap->map[tmp] == NULL || tmp == key)
			{
				printf("Not found\n");
				return NULL; // Object does not exist
			}
			tmp++;
			if (tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
		}
		key = tmp;
	}
	Mutex toFree = theMap->map[key];
	//mutex_destroy(toFree);
	//theMap->map[key] = NULL;
	printf("Grabbing from location %d.\nObtained pid: %d, looking for: %d\n", key, toFree->pcb1->pid, theKey->pid);
	
	return toFree;
}


int findKey(PCB theKey, int mapSize)
{
	int key = 0;
	if(theKey->parent == -1) // Process is non-companion
	{
		key = theKey->pid;
	}
	else	// Process IS comanion
	{
		key = theKey->parent;
	}
	if (key > 0)
	{	
		key = key % mapSize;
	}
	return key;
}



void toStringMutexMap (MutexMap theMap) {
	printf("MutexMap\r\n");
	for (int i = 0; i < MAX_INIT_BUCKETS; i++) {
		if (theMap->map[i]) {
			printf("map[%d]->pcb: P%d\r\n", i, theMap->map[i]->pcb1->pid);
		} else {
			printf("map[%d]->pcb: *\r\n", i);
		}
	}
	printf("Total MutexMap size: %d\r\n", theMap->curr_map_size);
}


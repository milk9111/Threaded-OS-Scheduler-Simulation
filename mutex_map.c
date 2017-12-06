#include "mutex_map.h"
#include <stdio.h>
#include <stdlib.h>


int findKey(int theKey, int mapSize);

MutexMap create_mutx_map()
{
	MutexMap newMap = (MutexMap) malloc(sizeof(mutex_map_s));
	
	int i;
	for (i = 0; i < MAX_INIT_BUCKETS; i++)
	{
		newMap->map[i] = NULL;
		newMap->hadCol[i] = 0;
	}
	newMap->curr_map_size = MAX_INIT_BUCKETS;

	return newMap;
}


int add_to_mutx_map(MutexMap theMap, Mutex theMutex, int theKey)
{
	if (theMap == NULL || theMutex == NULL)
	{
		return 1;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	printf("Attempting to insert in location %d.\r\n", key);
	if(theMap->map[key] != NULL)
	{
		int tmp = key; // If we hit here, we need to resolve a hash collision
		printf("--Insertion collision!--\r\n");
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
		theMap->hadCol[key] = 1;
	}
	theMap->map[key] = theMutex;
	printf("Inserting at location %d.\r\n", key);
	return 0;
}


int remove_from_mutx_map(MutexMap theMap, int theKey)
{
	if (theMap == NULL)
	{
		return 1; // Missing value
	}
	int key = findKey(theKey, theMap->curr_map_size);
	printf("Starting point pid: %d, looking for: %d\r\n", theMap->map[key]->mid, theKey);
	if (theMap->map[key] != NULL)
	{
		if((theMap->map[key]->mid != theKey) 
			&& (theMap->map[key]->mid != theKey))
		{
			int tmp = key; // If we hit here, we had a hash collision in the past 
						   // and need to find where our mutex got placed
			printf("--Delete collision!--\r\n");
			
			
			//printf("Obtained pid: %d, looking for: %d\n", theMap->map[tmp]->pcb1->pid, theKey->pid);
			//printf("before tmp = %d\n", tmp);
			//printf("before tmp + 1 = %d\n", tmp + 1);
			tmp++;
			//printf("after tmp = %d\n", tmp);
			//printf("Moving in...\n");
			//printf("pointer in location of tmp: %d", theMap->map[tmp]);
			while(theMap->map[tmp] == NULL)
			{
				if(tmp == key)
				{
					return 2;
				}
				tmp++;
				if(tmp >= theMap->curr_map_size)
				{
					tmp = 0;
				}
			}
			while((theMap != NULL) && (theMap->map[tmp]->mid != theKey))
			{	
				//printf("theMap == null: %d\n", (theMap == NULL));
				//printf("map[tmp]->pcb1->pid: %d\n", theMap->map[tmp]->pcb1->pid);
				//printf("pcb1 == null: %d\n", (theMap->map[tmp]->pcb1 == NULL));
				//printf("theKey->pid: %d\n", theKey->pid);
				//printf("Checking at location %d.\nObtained pid: %d, looking for: %d\n", tmp, theMap->map[tmp]->pcb1->pid, theKey->pid);
				//printf("passed\n");
				/*if(theMap->map[tmp] == NULL || tmp == key)
				{
					printf("Not found\n");
					return 2; // Object does not exist
				}*/
				//printf("passed\n");
				do
				{
				tmp++;
				//printf("tmp: %d\n", tmp);
				
					if (tmp >= theMap->curr_map_size)
					{
						tmp = 0;
						printf("in here\n");
					}
					//printf("tmp: %d\n", tmp);
					//printf("next loop\n");
				
					//printf("theMap->map[tmp]->pcb1->pid: %d\n", (theMap->map[tmp] == NULL));
					//printf("theKey->pid: %d\n", theKey->pid);
					//printf("theKey->parent: %d\n", theKey->parent);
					if(tmp == key)
					{
						printf("Not found\r\n");
						return 2; // Object does not exist
					}
				}
				while(theMap->map[tmp] == NULL);
			}
			key = tmp;
			printf("here\n");
		}
		Mutex toFree = theMap->map[key];
		printf("Removing from location %d.\r\nObtained pid: %d, looking for: %d\r\n", key, toFree->mid, theKey);
		mutex_destroy(toFree);
		theMap->map[key] = NULL;
			
		return 0;
	}
	else
	{
		return 2;
	}
}


Mutex take_n_remove_from_mutx_map(MutexMap theMap, int theKey)
{
	//toStringMutexMap(theMap);
	if (theMap == NULL)
	{
		return NULL;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	if (theMap->map[key] == NULL && theMap->hadCol[key] == 0)
	{
		return NULL;
	}
	else if (theMap->map[key] == NULL && theMap->hadCol[key] == 1)
	{
	//	printf("Skipping nulls...\n");
		int tmp = key;
		
		do
		{
			tmp++;
			if(tmp == key)
			{
				return NULL;
			}
			if(tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
	//	printf("Skipping nulls...\n");
		}
		while(theMap->map[tmp] == NULL);
		key = tmp;
				
	}
	printf("Attempting to remove M%d from location %d.\n", theKey, key);
	if((theMap->map[key]->mid != theKey))
	{
		int tmp = key; // If we hit here, we had a hash collision in the past 
					   // and need to find where our mutex got placed
		tmp++;
		printf("Delete and Remove collision!\r\n");
		while(theMap->map[tmp] == NULL)
		{
			if(tmp == key)
			{
	//			printf("Searched entire list...\n");
				return NULL;
			}
			tmp++;
			if(tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
		}
		while(theMap->map[tmp]->mid != theKey)
		{
			do
			{
				if(tmp == key)
				{
					
	//				printf("Searched entire list...\n");
					return NULL; // Object does not exist

				}
			
				tmp++;
				if (tmp >= theMap->curr_map_size)
				{
					tmp = 0;
				}
			}
			while(theMap->map[tmp] == NULL);
		}
		key = tmp;
	}
	Mutex toFree = theMap->map[key];
	//mutex_destroy(toFree);
	theMap->map[key] = NULL;
	//toStringMutexMap(theMap);
	return toFree;
}


Mutex get_mutx(MutexMap theMap, int theKey)
{
	
	if (theMap == NULL)
	{
		// printf("in here\n");
		return NULL;
	}
	int key = findKey(theKey, theMap->curr_map_size);
	if (theMap->map[key] == NULL && theMap->hadCol[key] == 0)
	{
		//toStringMutexMap(theMap);
		printf("Trying to get key %d from theKey %d, but found NULL in map\r\n", key, theKey);
		// printf("in here2\n");
		return NULL;
	}
	else if (theMap->map[key] == NULL && theMap->hadCol[key] == 1)
	{
	//	printf("Skipping nulls...\n");
		int tmp = key;
		
		do
		{
			tmp++;
			if(tmp == key)
			{
				return NULL;
			}
			if(tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
	//	printf("Skipping nulls...\n");
		}
		while(theMap->map[tmp] == NULL);
		key = tmp;
				
	}
	//toStringMutexMap(theMap);
	printf("Searching for mutex with key: %d.\r\n", key);
	
	if((theMap->map[key]->mid != theKey))
	{
		int tmp = key; // If we hit here, we had a hash collision in the past 
					   // and need to find where our mutex got placed
		tmp++;
		printf("--Search collision!--\r\n");
		/*toStringMutexMap(theMap);
		printf("Trying to find P%d\n", theKey->pid);
		printf("tmp: %d\n", tmp);*/
		//printf("Want: %d, have: %d, also have: %d vs %d\n", theKey->pid, theMap->map[tmp]->pcb1->pid, theMap->map[tmp]->pcb2->parent, theKey->parent);
		while(theMap->map[tmp] == NULL)
		{
			if(tmp == key)
			{
				return NULL;
			}
			tmp++;
			if(tmp >= theMap->curr_map_size)
			{
				tmp = 0;
			}
		}
		while(theMap->map[tmp]->mid != theKey)
		{
			//printf("Not found yet, moving on from %d...\n", tmp);
			do
			{
				if(tmp == key)
				{
					return NULL; // Object does not exist
				}
			
				tmp++;
				if (tmp >= theMap->curr_map_size)
				{
					tmp = 0;
				}
			}
			while(theMap->map[tmp] == NULL);
		}
		key = tmp;
	}
	Mutex toFree = theMap->map[key];
	if (toFree) {
		printf("returning M%d\r\n", toFree->mid);
	} else {
		printf("return NULL\r\n");
	}
	//mutex_destroy(toFree);
	//theMap->map[key] = NULL;
	//printf("Grabbing from location %d.\nObtained pid: %d, looking for: %d\n", key, toFree->pcb1->pid, theKey->pid);
	
	return toFree;
}


int findKey(int theKey, int mapSize)
{
	int key = theKey;
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
			printf("map[%d]->pcb: M%d\r\n", i, theMap->map[i]->mid);
		} else {
			printf("map[%d]->pcb: *\r\n", i);
		}
	}
	printf("Total MutexMap size: %d\r\n", theMap->curr_map_size);
}


void mutex_map_destroy (MutexMap theMap) {
	for (int i = 0; i < MAX_INIT_BUCKETS; i++) {
		if (theMap->map[i]) {
			mutex_destroy(theMap->map[i]);
		}
	}
	
	free(theMap);
}

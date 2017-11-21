#include "mutex_map.h"


int findKey(PCB theKey, int mapSize);

MutexMap create_mutx_map()
{
MutexMap newMap = calloc(sizeOf(MutexMap));
newMap->curr_map_size = MAX_INIT_BUCKETS;

return newMap;
}

int add_to_mutx_map(MutexMap theMap, Mutex theMutex, PCB theKey)
{
if ((theMap || theMuxtex || theKey) == NULL)
	{
	return 1;
	}
int key = findKey(theKey, theMap->curr_map_size);
if(theMap->map[key] != NULL)
	{
	int tmp = key; // If we hit here, we need to resolve a hash collision
	tmp++;
	while(theMap->map[key] != NULL)
		{
		if(tmp == key)
			{
			return 2; // Map is full. Will implement resizing if this becomes a problem.
			}
		tmp++;
		}
	key = tmp;
	}
theMap->map[key] = theMutex;

return 0;
}

int remove_from_mutx_map(MutexMap theMap, PCB theKey)
{
if ((theMap || theKey) == NULL)
	{
	return 1;
	}
int key = findKey(theKey, theMap->curr_map_size);
if((theMap->map[key]->  != 
}

Mutex take_n_remove_from_mutx_map(MutexMap theMap, PCB theKey)
{

}

Mutex get_mutx(MutexMap theMap, PCB theKey)
{

}

int findKey(PCB theKey, int mapSize)
{
int key = 0;
if(theKey->parent == -1) // Process is non-companion
	{
	key = theKey->pid;
	}
else			// Process IS comanion
	{
	key = theKey->parent;
	} 
key = key % mapSize;
return key;
}

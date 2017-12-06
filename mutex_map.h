#ifndef MUTEX_MAP_H
#define MUTEX_MAP_H

#include "pcb.h"

#define MAX_INIT_BUCKETS 200

typedef struct mutex_map
{
Mutex map[MAX_INIT_BUCKETS];
int hadCol[MAX_INIT_BUCKETS];
int curr_map_size;
} mutex_map_s;

typedef mutex_map_s* MutexMap;

/*
 * Standard constructor. Will make a new Mutex Map with default values.
 */
MutexMap create_mutx_map();



/*
 * Add a Mutex to the specified MutexMap. The key for the map is determined by the 
 * Mutex ID (i.e. Mutex->mid), to be passed in as an int.
 *
 * Return: 0 if Mutex was added successfully, 1 on failure, 2 on full map
 *
 * Implementation note: relies on the current fact that pcb->parent is unused by the program
 * Implementation note2: The actual key value will be a modulus of the current max size of the map
 */
int add_to_mutx_map(MutexMap theMap, Mutex theMutex, int theKey);


void toStringMutexMap (MutexMap theMap);


/*
 * Remove a Mutex from the specified MutexMap.
 *
 * Return: 0 if Mutex was removed successfully, 1 on failure 
 */
int remove_from_mutx_map(MutexMap theMap, int theKey);




/*
 * Remove a Mutex from the specified map and pass it back to the caller for use.
 *
 * Return: a Mutex found based on the passed in key. NULL otherwise.
 */
Mutex take_n_remove_from_mutx_map(MutexMap theMap, int theKey);




/*
 * Find a Mutex based on the passed in key and return a pointer to it.
 *
 * Return: a Mutex found based on the passed in key. NULL otherwise.
 */
Mutex get_mutx(MutexMap theMap, int thekey);


void mutex_map_destroy (MutexMap theMap);



#endif

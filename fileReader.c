#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void main () {
	FILE *in;
	FILE *out = fopen("deadlockResults.txt", "w");
	
	char deadlock[] = "deadlock";
	char txt[] = ".txt";
	
	char concat[14];
	
	
	for (int i = 1; i <= 10; i++) {
		sprintf(concat,"%s%d%s",deadlock,i,txt);
		in = fopen(concat, "r");
		while (!feof(in)) {
			char *holder = (char *) malloc(sizeof (char) * 10000);
			
			
			free(holder);
		}
	}
	
	
}
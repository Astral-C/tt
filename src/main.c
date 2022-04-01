#include <stdio.h>
#include "tracker.h"

int main(){
	ModTracker tracker;

	tracker_open_mod(&tracker, "mod.highspeed.mod");
	printf("Finished loading data module...\n");
	tracker_close_mod(&tracker);

	return 0;
}

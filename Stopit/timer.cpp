// adapted from one of the examples found in C:\pharemb\examples\timer

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <string.h>

#include <embkern.h>


//
// Global Variables
//
volatile BOOL tickOccurred = FALSE;  // A timer tick happened.
DWORD elapsedWhole = 0;	             // whole # of milliseconds elapsed
DWORD elapsedFrac = 0;               // fractional ms * 2^32 elapsed

//
// Timer callback, updates global variables accordingly.
//

// second argument is a dummy variable. we only need ETS_INPUT_RECORD obejct
// conviniently enough, it seems like ETS_INPUT_RECORD actually contains real timing information
// tick is simply what it is; a tick.
int __cdecl TimerCallback(ETS_INPUT_RECORD *r, DWORD dmy)
{
	elapsedWhole += r->Event.TimerEvent.elapsedWhole;
	elapsedFrac += r->Event.TimerEvent.elapsedFrac;
	if(elapsedFrac < r->Event.TimerEvent.elapsedFrac)
	{
		// we have to carry.
		++ elapsedWhole;
	}
	tickOccurred = TRUE;
	return ETS_CB_CONTINUE; //tells the system to proceed
}

// registers the timer callback
void RegisterTimerCallback() {
	//variables we'll use to register the callback
	EK_KERNELINFO ki; //kernel info
	EK_SYSTEMINFO si; //system info

	//install the callbacks
	printf("Installing timer callback function\n");
	// ETS_CB_TIMER is the callback type
	// TimerCallback is the function
	// ETS_CB_ADD signifies that we want to add a callback
	if (!EtsRegisterCallback((WORD)ETS_CB_TIMER, TimerCallback, 0, ETS_CB_ADD)) {
		printf("Timer callback installation failed.\n");
	}
	printf("Callback installed.\n");
}


//DWORD is 2 words, or 4 bytes.
DWORD getTimeNow() {
	return elapsedWhole;
}
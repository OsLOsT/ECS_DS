/****************************************************************/
/* demo.c														*/
/****************************************************************/
#include "peg.hpp"
#include "demo.h"
#include "math.h"
#include "HEADER.h"
#include <embkern.h>
#include <embclib.h>
#include <malloc.h>
#include <video.h>

#include "Header.h"
#include "tunes.inc"
#include <process.h>

#include "timer.hpp"
#include "motor.h"

#include "time.h"

unsigned int __stdcall frame_thread(void* pArguments);

/****************************************************************/
/* MAIN                                                         */
/****************************************************************/
Ubyte KeyBeep(void);  // wait for key, then BEEP

void EcsGuiInit(HANDLE *hGuiThread, HANDLE *hpScrMgr , HANDLE *hpEcsPic);
char EcsDrawBMP(char *file, PegPresentationManager *pScrMgr);
void EcsConsoleMsg(char*);

/*********
STOPIT PROGRAM
Does things for StopIt
For the modals, returning -1 will create an escape route all the way to the root.
For the modals, returning 0 will escape the parent modal.
For the modals, returning any other number but -1 or 0 will cause parent modal to repeat
***********/

/* Global Variable Declaration */

//stepper motor initializations
HANDLE hThread_sm;
unsigned int threadID_sm;

//music initializations
HANDLE hThread_music;
unsigned int threadID_music;

//HANDLE playMusicEvent; <-- delcared in music.cpp now

//gui initializations
HANDLE hThread_Frame;
unsigned int threadID_frame;
PegPresentationManager *pScrMgr;
EcsPic *pEcsPic;

HANDLE hpEcsPic, hpScrMgr, hGuiThread;
HANDLE startFrameEvent;

char pathPrefix_STPWTCHM[] = "C:\\STOPRM\\";
char pathPrefix_STPWTCHS[] = "C:\\STOPRS\\";
char pathPrefix_TIMERMEN[] = "C:\\TIMERRM\\";
char pathPrefix_TIMERSTP[] = "C:\\TIMERRS\\";

char pathPrefix_MAIN[] = "C:\\MAINM\\";
char pathPrefix_STPWTCH[] = "C:\\STOPM\\";
char pathPrefix_TIMER[] = "C:\\REFT\\";

//This will be used by the drawing thread to know which prefix to use.
volatile char* currentPathPrefix = pathPrefix_STPWTCHM;
volatile int frameNo = 2; //the frame number

/* Function Declaration */
void stopit_main();
int stopit_after_countdown_modal();
int stopit_countdown_modal(DWORD duration, short cursorPos);
int stopit_start_countdown_modal();

int stopit_start_stopwatch_modal();
int stopit_stopwatch_modal();
int stopit_after_stopwatch_modal();

/* Utility Declaration */
void stopit_countdown(DWORD duration);
void stopit_countup(char escape_key);

/* Utility Definition */
void stopit_countdown(DWORD duration, char escape_key) {
	DWORD end_time = getTimeNow() + duration;
	DWORD start_time = getTimeNow();
	DWORD lastTimeCheck = 1;
	int min_size = 999;
	char buffer[17];

	const int interval = duration / 60; //the interval to add a frame to the progress bar

	frameNo = 1;
	// start the countdown
	for (DWORD current_time = getTimeNow(); current_time < end_time; 
	current_time = getTimeNow()) {
		DWORD diff_in_ms = end_time - current_time;

		sprintf(buffer, "%.1fs", (double) diff_in_ms / 1000);
		if (strlen(buffer) < min_size) {
			LcdCmd(0x80 + strlen(buffer));
			LcdMsg(" ");
			min_size = strlen(buffer);
		}

		LcdLine1();
		LcdMsg(buffer);

		if (KeyAny() == 0) { //check for KeyPress
			if (ScanKey() == escape_key) { //if the key press is zero
				break; //break out of the loop
			}
		}

		//update frames using a thread, only if the time has been sufficient
		//this has to be done because drawing images take time, and time
		//will affect the speed of the timer on the lcd
		if (lastTimeCheck * interval < current_time - start_time) {
			lastTimeCheck++;
			frameNo++;
		}
	}

	// set countdown to 0
	LcdLine1();
	LcdMsg("Done");
}

void stopit_countup(char escape_key) {
	char buffer[17];
	DWORD start_time = getTimeNow();
	DWORD timeElapsed = 0;
	DWORD lastTimeCheck = 0; //time check is basically timeElapsed / 1000

	// continuously count up
	while (true) {
		timeElapsed = getTimeNow() - start_time;
		sprintf(buffer, "%.1fs", (double) timeElapsed / 1000);

		LcdLine1();
		LcdMsg(buffer);

		if (KeyAny() == 0) { //check for KeyPress
			if (ScanKey() == escape_key) { //if the key press is zero
				break; //break out of the loop
			}
		}

		//update frames using a thread, only if the time has been sufficient
		//this has to be done because drawing images take time, and time
		//will affect the speed of the timer on the lcd
		if (lastTimeCheck < timeElapsed / 1000) {
			lastTimeCheck++;
			frameNo++;
		}
	}

	sprintf(buffer, "Elapsed: %.1fs", (double) (getTimeNow() - start_time) / 1000);
	LcdLine1();
	LcdMsg(buffer);
}

/* Function Definition */
int stopit_after_countdown_modal() {
	//variables
	char* prompt1 = "0. Main Menu";
	char* prompt2 = "1. Countdown";

	char fullPath[60];
	sprintf(fullPath, "%sframe-%d.bmp", pathPrefix_TIMERMEN, frameNo);
	EcsDrawBMP(fullPath, pScrMgr);
	pEcsPic->Draw();

	LcdClear(); //clears the lcd
	LcdLine1();
	LcdMsg(prompt1);
	LcdLine2();
	LcdMsg(prompt2);

	switch (KeyBeep()) {
	case 0:
		return -1;
	case 1:
		return 0;
	}
}

int stopit_countdown_modal(DWORD duration) {
	//variable
	char* prompt = "REDIAL: Stop";

	LcdClear(); //clears the lcd before the countdown
	LcdLine2();
	LcdMsg(prompt);

	max_time = duration; //set the max time used by the spinning thread
	stopwatch = false;

	currentPathPrefix = pathPrefix_TIMERSTP;
	frameNo =	1;
	SetEvent(startFrameEvent);
	SetEvent(startMotorEvent); //start the motor
	stopit_countdown(duration * 1000, 10); //blocking timing call
	ResetEvent(startMotorEvent);
	ResetEvent(startFrameEvent);

	//Choose a random piece to play
	music_to_play = ReinMaple;
	SetEvent(playMusicEvent);

	//process return value
	switch(stopit_after_countdown_modal()) {
	case -1:
		return -1;
	default:
		return 2;
	}
}

int stopit_start_countdown_modal() {
	//variables / constants
	char* prompt1 = "Duration: ";
	char* prompt2 = "REDIAL: Start";
	DWORD duration = 0; //set to zero by default
	short cursorPos = 0; //cursorPos is offset by sizeof(duration_msg)
	char buffer[16];

	// get user input to how long they want the countdown to be
	while (true) { //Outer Loop - Repeats if the return value from child dialogs are >2
		//show screen for start menu
		currentPathPrefix = pathPrefix_TIMER;
		frameNo = 1;
		SetEvent(startFrameEvent);
		ResetEvent(startFrameEvent);

		LcdClear();
		LcdLine2();
		LcdMsg(prompt2);
		LcdLine1();
		LcdMsg(prompt1);
		while (true) { //Inner Loop - Checks for key presses
			Ubyte c = KeyPressed();

			if (c == 10) //10 is enter
				break;
			else if (c == 11) { //11 is backspace
				if (cursorPos > 0) { // only if the cursor is beyond 0
					LcdCmd(0x80 + strlen(prompt1) + --cursorPos); //move one step back
					LcdMsg(" "); //clear it
					LcdCmd(0x80 + strlen(prompt1) + cursorPos); //go back to that pos

					duration /= 10; //removes the last digit in the duration
				} else {
					return -1;
				}
			} else { //it's a number
				if (duration == 0) {
					if (c == 0) //if the duration is 0, and the joker presses 0, ignore it
						continue;

					duration = c;
				} else duration = duration * 10 + c; //adds the duration

				//update the timing
				LcdCmd(0x80 + strlen(prompt1) + cursorPos);
				sprintf(buffer, "%d", c);
				LcdMsg(buffer);
				cursorPos++;
			}
		} //End of inner loop
			
		if(stopit_countdown_modal(duration) == -1)
			return -1; //returns straight away, otherwise repeat the loop

		/* cleanup */
		duration = 0;
		cursorPos = 0;
	} //End of outer loop
}

int stopit_after_stopwatch_modal() {
	//variables
	char* prompt = "0. Menu 1. Reset";

	char fullPath[60];
	sprintf(fullPath, "%sframe-%d.bmp", pathPrefix_STPWTCHM, frameNo);
	EcsDrawBMP(fullPath, pScrMgr);
	pEcsPic->Draw();

	LcdLine2();
	LcdMsg("                "); //clear the second line
	LcdLine2(); //reset position
	LcdMsg(prompt);

	switch (KeyBeep()) {
	case 0:
		return -1;
	case 1:
		return 1;
	}
}

int stopit_stopwatch_modal() {
	//variables
	char* prompt = "1. Stop";

	LcdClear();
	LcdLine2();
	LcdMsg(prompt);

	currentPathPrefix = pathPrefix_STPWTCHS;
	frameNo = 1;
	SetEvent(startMotorEvent);
	SetEvent(startFrameEvent); //start the frame drawing
	stopwatch = true; //make motor turn like stopwatch
	stopit_countup(1); //count up thingy
	ResetEvent(startFrameEvent);
	ResetEvent(startMotorEvent);

	//Choose a random piece to play (TODO)
	music_to_play = ReinMaple;
	SetEvent(playMusicEvent);

	switch (stopit_after_stopwatch_modal()) {
	case -1:
		return -1;
	case 1:
		return 2;
	}
}

int stopit_start_stopwatch_modal() {
	//variables
	char* prompt1 = "0. Main menu";
	char* prompt2 = "1. Start";

	//show screen for start menu
	while (true) {
		currentPathPrefix = pathPrefix_STPWTCH;
		frameNo = 1;
		SetEvent(startFrameEvent);
		ResetEvent(startFrameEvent);

		LcdClear(); //clear the LCD
		LcdLine1();
		LcdMsg(prompt1);
		LcdLine2();
		LcdMsg(prompt2);

		switch(KeyBeep()) {
		case 0:
			return -1;
		case 1:
			switch(stopit_stopwatch_modal()) {
			case -1:
				return -1;
			default:
				break;
			}
		}
	}
}

void stopit_display_main() {
	currentPathPrefix = pathPrefix_MAIN;
	frameNo = 1;
	SetEvent(startFrameEvent);
	ResetEvent(startFrameEvent);
}



void stopit_main() {
	/* initialization */
    LcdInit(); //initialize the lcd screen
	LcdClear();

	LcdMsg(" Wait... "); //show the user a message while registering callback

	// create an event for the drawing thread
	startFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // stepper motor
	startMotorEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hThread_sm = (HANDLE)_beginthreadex(NULL,0,&setTimeMotor_ThreadWrapper,NULL,0,&threadID_sm);

	// images
	EcsGuiInit(&hGuiThread, &hpScrMgr, &hpEcsPic);	/* initialise graphics */
	pEcsPic = (EcsPic *)hpEcsPic;
	pScrMgr = (PegPresentationManager *)hpScrMgr;
	//EcsDrawBMP(image1, pScrMgr);	/* Demo Welcome */

	// frames (the thing will start automatically)
	hThread_Frame = (HANDLE)_beginthreadex(NULL,0,&frame_thread,NULL,0,&threadID_frame);

	//music (start the music thread immediately)
	hThread_music = (HANDLE)_beginthreadex(NULL,0,&stopit_music_thread,NULL,0,&threadID_music);
	playMusicEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    //callback function initializations
	RegisterTimerCallback();

	//draw stuff (sleep a little first, seems like the thread takes a while to spawn)
	Sleep(1000);
	stopit_display_main();

	// loop forever
	while(true) {
		LcdClear();
		LcdLine1();
		LcdMsg("1. Stopwatch");
		LcdLine2();
		LcdMsg("2. Countdown");

		switch (KeyBeep()) {
		case 1:
			LcdClear();
			stopit_start_stopwatch_modal();
			stopit_display_main(); //show main menu
			break;
		case 2:
			LcdClear();
			stopit_start_countdown_modal();
			stopit_display_main(); //show main menu
			break;
		}
	}
}


Ubyte KeyBeep(void)
{
	Ubyte c;
	c = KeyPressed();
	LedDisp(c);
	BEEP();
	
	//Reset any song events
	ResetEvent(playMusicEvent);
	return c;
}

unsigned int __stdcall frame_thread(void* pArguments) {
	char fullPath[255];

	while (true) {
		//wait until an event says it's okay to draw frames
		WaitForSingleObject(startFrameEvent,INFINITE);

		sprintf(fullPath, "%sframe-%d.bmp", currentPathPrefix, frameNo);
		EcsDrawBMP(fullPath, pScrMgr);
		pEcsPic->Draw();

		if (frameNo > 60)
			frameNo = 1;
		//else frameNo++;
	}
}

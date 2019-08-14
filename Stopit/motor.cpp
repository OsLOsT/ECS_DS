#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream.h>

#include "motor.h"
#include "timer.hpp"

#include "HEADER.h"

volatile DWORD max_time = 20;
volatile bool finish = true;
volatile bool stopwatch = false;

volatile HANDLE startMotorEvent;


void setTimeMotor(int t,int a){

	unsigned char Ptable []={0x01,0x02,0x04,0x08};
	DWORD currentTime = 0;
	DWORD endTime = 0;
	int i, j;
	i=0;

	for (j=200;j>0;j--)
		{
		if (a ==1){
		_outp (0x335,Ptable [i]);	//0x335 is address of SMPort	/* output to port */
		}
		else{
		_outp (0x335,Ptable [3-i]);
		}
		currentTime = getTimeNow();
		endTime = currentTime + t*5;
		while (currentTime < endTime);
		i++;
		if (i>=4) i=0;
		}
 return;}	

unsigned int __stdcall setTimeMotor_ThreadWrapper(void* params) {
	while (true) {
		WaitForSingleObject(startMotorEvent,INFINITE);
		
		if (!stopwatch) {
			Spin(CCW, 4, 5 * max_time - 10);
			//Spin(CW,200,5 * max_time - 10);
		}
		else {
			Spin(CW,4,290);
		}
	}
	return 0;
}
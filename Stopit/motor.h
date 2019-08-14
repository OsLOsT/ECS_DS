#pragma once

extern volatile DWORD max_time;
extern volatile bool finish;
extern volatile bool stopwatch;
unsigned int __stdcall setTimeMotor_ThreadWrapper(void*);

extern volatile HANDLE startMotorEvent;

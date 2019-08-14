#pragma once

//call this once in main, it'll register the timer callback function
void RegisterTimerCallback();

//gets the current time in milliseconds
DWORD getTimeNow();
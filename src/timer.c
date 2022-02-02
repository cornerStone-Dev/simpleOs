// timer.c
#include "../localTypes.h"

typedef struct TimerData {
	ProcessInfo *nextProcess;
} TimerData;

typedef struct TimersInfo {
	u32       usageBitMap;
	TimerData alarm[4];
} TimersInfo;

static TimersInfo timerInfo;

/*e*/
s32 os_sleep(u32 micoseconds)/*p;*/
{
	if (micoseconds < 10) { micoseconds = 10; }
	u32 selectedTimer = 0;
	u32 retVal = 0;
	asm("CPSID i");
	os_takeSpinLock(LOCK_ALARM);
	u32 usageBitMap = timerInfo.usageBitMap;
	u32 currentBit = 1;
	while ( (usageBitMap & currentBit) == 1) { 
		selectedTimer++;
		currentBit=currentBit<<1;
	}
	if (selectedTimer > 3) {
		retVal = -1;
		os_giveSpinLock(LOCK_ALARM);
		asm("CPSIE i");
	} else {
		ProcessInfo *current = _getCurrentProcess();
		timerInfo.alarm[selectedTimer].nextProcess = current;
		timerInfo.usageBitMap |= currentBit;
		TimerMemMap *timer = (void*)TIMER_BASE;
		u32 currentTime = timer->timeReadLowRaw;
		u32 targetTime = currentTime + micoseconds;
		timer->alarm[selectedTimer] = targetTime;
		os_giveSpinLock(LOCK_ALARM);
		asm("CPSIE i");
		asm("svc #1");
	}
	return retVal;
}

/*e*/
void alarmISR(void)/*p;*/
{
	os_takeSpinLock(LOCK_ALARM);
	TimerMemMap *timer = (void*)TIMER_BASE;
	u32 currentAlarm = timer->ints;
	timer->intr = currentAlarm;
	u32 selectedTimer = 0;
	u32 usageBitMap = currentAlarm;
	u32 currentBit = 1;
	while ( (usageBitMap & currentBit) == 0) {
		selectedTimer++;
		currentBit=currentBit<<1;
	}
	// selected timer is the timer array index and current bit is the bit
	os_processEnqueue(timerInfo.alarm[selectedTimer].nextProcess, 0);
	timerInfo.usageBitMap &= ~currentBit;
	os_giveSpinLock(LOCK_ALARM);
}

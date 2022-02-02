// os.c
#include "../localTypes.h"

ROMFunctions rom_func;
void (*lineHandler)(u8 *in);

typedef void (*ProcessEntryPoint)(void *arg1, void *arg2);
static  ProcessInfo *processQueue[2];


// remove process that is ready to run or 0 is there are none.
/*e*/
ProcessInfo*
os_processDequeue(void)/*p;*/
{
	ProcessInfo *process;
	asm("CPSID i");
	os_takeSpinLock(LOCK_PROCESS_QUEUE);
	process = list_removeFirst(&processQueue[0]);
	if (process == 0)
	{
		process = list_removeFirst(&processQueue[1]);
	}
	os_giveSpinLock(LOCK_PROCESS_QUEUE);
	asm("CPSIE i");
	return process;
}

// insert process to end of queue
/*e*/
void
os_processEnqueue(ProcessInfo *newProcess, u32 queNum)/*p;*/
{
	asm("CPSID i");
	os_takeSpinLock(LOCK_PROCESS_QUEUE);
	processQueue[queNum] = list_append(newProcess, processQueue[queNum]);
	asm("sev"); // signal CPU's to wake up and get to work
	os_giveSpinLock(LOCK_PROCESS_QUEUE);
	asm("CPSIE i");
	return;
}

typedef struct StackLayout {
	u32 R4;
	u32 R5;
	u32 R6;
	u32 R7;
	u32 LR;
	u32 R0;
	u32 R1;
	u32 R2;
	u32 R3;
	u32 R12;
	u32 LR_processDoesNotReturn;
	u32 PC;
	u32 PSR;
} StackLayout;

/*e*/
void
os_createProcess(void *entryPoint, void *arg1, void *arg2)/*p;*/
{
	// get memory for the stack
	u32 *memory = allocStack();
	// use part of stack as process information
	ProcessInfo *newProcess = (ProcessInfo *)memory;
	// stack grows down, therefore the stack starts at the highest address
	StackLayout *stackPointer = (void*)&memory[512/4 - 13];
	// stack grows downward
	stackPointer->PSR = (1<<24); // PSR thumb bit
	stackPointer->PC = (u32)os_processWrapper; // pointer to function to resume
	// stackPointer[10]  LR which has no meaning in this context
	// stackPointer[9]   R12 which has no meaning in this context
	// stackPointer->R3  R3 unused argument
	stackPointer->R2 = (u32)entryPoint; // R2 function to call by wrapper
	stackPointer->R1 = (u32)arg2; // R1
	stackPointer->R0 = (u32)arg1; // R0
	stackPointer->LR = 0xFFFFFFF9; // LR indicate to return from interrupt
	// stackPointer[3]   R7 which has no meaning in this context
	// stackPointer[2]   R6 which has no meaning in this context
	// stackPointer[1]   R5 which has no meaning in this context
	// stackPointer[0]   R4 which has no meaning in this context
	newProcess->stack = (void*)stackPointer;
	os_processEnqueue(newProcess, 0);
	return;
}

/*e*/
void os_disableSysTick(void)/*p;*/
{
	SYS_TICK_S *sys = (void*)PPB_SYSTICK;
	sys->csr = 4;
}

/*e*/
void os_enableSysTick(void)/*p;*/
{
	SYS_TICK_S *sys = (void*)PPB_SYSTICK;
	sys->rvr = 100000 - 1; // 120mhz clock, this puts us at 1200 hz
	sys->cvr = 0; // reset current counter
	sys->csr = 7; // enable, usu system clock, and interrupt
}

/*e*/
void os_enterIdleProcess(void)/*p;*/
{
	os_enableSysTick();
	asm("svc #1");
	while (1)
	{
		asm("wfe");
	}
}

/*e*/
void os_programBootStrap(void *bootCode)/*p;*/
{
	u32 *codeBuff = zalloc(256);
	codeBuff[63] = ut_crcCopy(bootCode, codeBuff, 252);
	io_programFlash(codeBuff, 256, 0);
	free(codeBuff);
}



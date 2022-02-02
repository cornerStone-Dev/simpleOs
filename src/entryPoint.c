// entryPoint.c
#include "../localTypes.h"

#define NUM_PRINTS 4234

#define FLASH_BLOCK_NUM 75 //512*1024
#define FLASH_OFFSET (FLASH_BLOCK_NUM*4096)

void testFlashProgramming(void)
{
	
	u32 volatile *data = (u32*)((u8*)FLASH_BASE+FLASH_OFFSET);
	io_prints("flash\n");
	//~ io_programFlash("\n\nHello new world from flash!22\n\n\n",
		//~ 256, FLASH_BLOCK_NUM);
	io_printsl((u8*)FLASH_BASE+FLASH_OFFSET, 34);
	//~ io_printi(s32 in)
	io_printh(data[0]);
	io_prints("\ndone trying\n");
}

void testPrintP2(void *arg)
{
	for (u32 i = 0; i < NUM_PRINTS; i++)
	{
		io_prints("b");
	}
	io_prints("OK\n");
}
void testPrintP3(void *arg)
{
	for (u32 i = 0; i < NUM_PRINTS; i++)
	{
		io_prints("c");
	}
	io_prints("OK\n");
}

void testPrintP4(void *arg)
{
	for (u32 i = 0; i < NUM_PRINTS; i++)
	{
		io_prints("x");
	}
	io_prints("OK\n");
}

void testPrintP5(void *arg)
{
	for (u32 i = 0; i < NUM_PRINTS; i++)
	{
		io_prints("r");
	}
	io_prints("OK\n");
}

void testPrintP6(void)
{
	while(1) {
		os_sleep(1000*1000*5);
		io_prints("Sleepy Head!\n");
	}
}


/*e*/
void cEntryPoint(void)/*p;*/
{
	picoInit(); // this must be first
	
	
	//~ prints("\nB\n");
	os_createProcess(testFlashProgramming, 0, 0);
	//~ os_createProcess(testPrintP6, 0, 0);
	os_createProcess(testPrintP3, 0, 0);
	os_createProcess(testPrintP2, 0, 0);
	os_createProcess(testPrintP4, 0, 0);
	os_createProcess(testPrintP5, 0, 0);
}

/*e*/
void createFirstProcess(void)/*p;*/
{
	os_createProcess(cEntryPoint, 0, 0);
	os_enterIdleProcess();
}

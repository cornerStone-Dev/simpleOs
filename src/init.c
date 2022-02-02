// init.c
#include "../localTypes.h"

static void
enableWatchDogTick(void)
{
	u32 *watchDogTickReg = (void*)0x4005802C;
	*watchDogTickReg = 12|(1<<9);
}

static void
timerInit(void)
{
	TimerMemMap *timer = (void*)TIMER_BASE;
	timer->intr = 0xFF;
	timer->inte = (1<<0)|(1<<1)|(1<<2)|(1<<3);
}

// Bootrom function: rom_table_lookup
// Returns the 32 bit pointer into the ROM if found or NULL otherwise.
typedef void *(*rom_table_lookup_fn)(u16 *table, u32 code);

static void*
rom_func_lookup(u32 code)
{
	rom_table_lookup_fn *rom_table_lookup = (rom_table_lookup_fn*)0x18;
	u16 *func_table = (u16 *)0x14;
	return (*rom_table_lookup)((u16 *)(u32)(*func_table), code);
}

//~ static void*
//~ rom_data_lookup(u32 code)
//~ {
	//~ rom_table_lookup_fn *rom_table_lookup = (rom_table_lookup_fn*)0x18;
	//~ u16 *data_table = (u16 *)0x16;
	//~ return (*rom_table_lookup)((u16 *)(u32)(*data_table), code);
//~ }

static u32 rom_table_code(u8 c1, u8 c2)
{
	return (c2 << 8) | c1;
}

static void
loadRomFuncAddrs(void)
{
	// grab all function addresses
	rom_func._connect_internal_flash = rom_func_lookup(rom_table_code('I','F'));
	rom_func._flash_exit_xip = rom_func_lookup(rom_table_code('E','X'));
	rom_func._flash_range_erase = rom_func_lookup(rom_table_code('R','E'));
	rom_func.flash_range_program = rom_func_lookup(rom_table_code('R','P'));
	rom_func._flash_flush_cache = rom_func_lookup(rom_table_code('F','C'));
	rom_func.memcpy = rom_func_lookup(rom_table_code('M','C'));
	rom_func.memset = rom_func_lookup(rom_table_code('M','S'));
}

/*e*/
void picoInit(void)/*p;*/
{
	// clear error flags in FIFO
	u32 *fifoStatus = (void*)SIO_FIFO_ST;
	*fifoStatus = 0x0C;
	// disable all interrupts
	u32 *intDisable = (void*)PPB_INTERRUPT_DISABLE;
	*intDisable = 0xFFFFFFFF;
	// clear all pending interrupts
	u32 *intPend = (void*)PPB_INTERRUPT_CLEAR_PEND;
	*intPend = 0xFFFFFFFF;
	// enable NVIC interrupts
	u32 *intEnable = (void*)PPB_INTERRUPT_ENABLE;
	*intEnable = (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<15)|(1<<20)|(1<<26)|(1<<27)|(1<<28);
	// set interrupt priorities
	u32 *intPriority = (void*)PPB_NVIC_IPR6;
	*intPriority++ = (1<<22)|(2<<30);
	*intPriority = (3<<6);
	// disable deep sleep and disable "wake up on every possible event"
	u32 *sysCtrl = (void*)PPB_SYS_CNTRL;
	*sysCtrl = 0;
	// set sysTick to lowest priority and PendSV
	intPriority = (void*)PPB_SYS_SHPR2;
	*intPriority = (3<<30)|(3<<22);
	intPriority--; *intPriority = (3<<30); // SVC is also lowest priority
	enableWatchDogTick();
	timerInit();
	loadRomFuncAddrs();
	
	
	// set up flash
	rom_func._connect_internal_flash();
	rom_func._flash_exit_xip();
	rom_func._flash_flush_cache();
	xipSetup();
	// we can now read flash addresses
	// set up uart charcater handler
	//~ uartHandler = term_processCharacter;
	io_setUartHandler(term_processCharacter);
	//~ uartHandler = byteHandelFunc;
	// set up line handler to be kicked off by terminal
	lineHandler = shell_inputLine;
	shell_init();
}




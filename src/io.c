// io.c
#include "../localTypes.h"

static u8   uart0CircularBuffer[SIZE_PRINT_BUFF];
static u32  uart0WriteIndex;
static u32  uart0ReadIndex;
static void (*uartHandler)(u32 in);

static void txByte(u32 byte)
{
	Uart0MemMap *uart = (void*)UART0_BASE;
	printAgain:
	// check if print buffer is full
	if (((uart0WriteIndex+1)&(SIZE_PRINT_BUFF-1)) == uart0ReadIndex)
	{
		// print buffer is full, wait for space to open in tx fifo
		while ((uart->flags&(1<<5))!=0) { }
		uart->data = uart0CircularBuffer[uart0ReadIndex++];
		uart0ReadIndex &= SIZE_PRINT_BUFF - 1;
	}
	// put byte into write buffer
	uart0CircularBuffer[uart0WriteIndex++] = byte;
	uart0WriteIndex &= SIZE_PRINT_BUFF - 1;
	// attempt to put that character into the tx fifo if it is not full
	if ((uart->flags&(1<<5))==0)
	{
		uart->data = uart0CircularBuffer[uart0ReadIndex++];
		uart0ReadIndex &= SIZE_PRINT_BUFF - 1;
	}
	// deal with /r/n sequence
	if (byte == '\n') { byte = '\r'; goto printAgain; }
}

/*e*/
void io_txByte(u32 byte)/*p;*/
{
	asm("CPSID i");
	os_takeSpinLock(LOCK_UART0_OUT);
	txByte(byte);
	os_giveSpinLock(LOCK_UART0_OUT);
	asm("CPSIE i");
}


/*e*/
void io_prints(u8 *string)/*p;*/
{
	asm("CPSID i");
	os_takeSpinLock(LOCK_UART0_OUT);
	u32 byte = *string++;
	if (byte != 0)
	{
		do {
			txByte(byte);
			byte = *string++;
		} while (byte != 0);
	}
	os_giveSpinLock(LOCK_UART0_OUT);
	asm("CPSIE i");
}

/*e*/
void io_printsl(u8 *string, u32 len)/*p;*/
{
	asm("CPSID i");
	os_takeSpinLock(LOCK_UART0_OUT);
	for (u32 i = 0; i < len; i++)
	{
		txByte(string[i]);
	}
	os_giveSpinLock(LOCK_UART0_OUT);
	asm("CPSIE i");
}

/*e*/
void io_printi(s32 in)/*p;*/
{
	u8 number[16];
	i2s(in, number);
	io_prints(number);
}

/*e*/
void io_printh(s32 in)/*p;*/
{
	u8 number[16];
	i2sh(in, number);
	io_prints(number);
}

/*e*/
s32 io_programFlash(void *data, u32 size, u32 targetBlockNum)/*p;*/
{
	// reserve the first 67 blocks for RAM mirror
	//~ if (targetBlockNum < 67) { return -1; }
	// round up size to 256
	
	size = (size + 255) / 256 * 256;
	u32 blockAddr = targetBlockNum * 4096;
	u32 blockSize = (size + 4095) / 4096 * 4096;
	asm("CPSID i");
	os_takeSpinLock(LOCK_FLASH);
	rom_func._connect_internal_flash();
	rom_func._flash_exit_xip();
	rom_func._flash_range_erase(blockAddr, blockSize, 4096, 0x20);
	rom_func.flash_range_program(blockAddr, data, size);
	rom_func._flash_flush_cache();
	xipSetup();
	os_giveSpinLock(LOCK_FLASH);
	asm("CPSIE i");
	return 0;
}

#define SIO_GPIO_OUT_XOR (SIO_BASE + 0x1C) 

/*e*/
void io_ledToggle(void)/*p;*/
{
	u32 *gpioXor = (void*)SIO_GPIO_OUT_XOR;
	*gpioXor = (1<<25);
}

//~ typedef void (*fByte)(u32 byte);
//~ /*e*/
//~ void sMap(u8 *string, void *func)/*p;*/
//~ {
	//~ fByte f = func;
	//~ u32 byte = *string++;
	//~ if (byte != 0)
	//~ {
		//~ do {
			//~ f(byte);
			//~ byte = *string++;
		//~ } while (byte != 0);
	//~ }
//~ }

/*e*/
void byteHandelFunc(u32 in)/*p;*/
{
	//~ u8 buff[16];
	//~ buff[0] = in;
	//~ buff[1] = 0;
	if (in == '\r') {
		io_prints("\n");
	} else {
		for (u32 i = 0; i < 1; i++)
		{
			//~ io_prints(buff);
			io_printi(in);
			io_prints(" ");
		}
	}
	//~ io_prints(buff);
	//~ io_prints("OK\n");
}

/*e*/
void io_setUartHandler(void *function)/*p;*/
{
	uartHandler = function;
}

void uart0SingleChar(void *arg)
{
	Uart0MemMap *uart = arg;
	do {
		(*uartHandler)(uart->data);
	} while ( (uart->flags & (1<<4)) == 0);
	// we have finished draining the fifo from input. Enable rx interrupts.
	uart->intMaskSet |= (1<<4)|(1<<6);
}

/*e*/
void isr_Uart0(void)/*p;*/
{
	Uart0MemMap *uart = (void*)UART0_BASE;
	// clear the interrupts
	u32 intStatus = uart->maskedIntStatus;
	uart->intClear = 0x7FF;
	//~ u32 *clearPending = (void*)PPB_INTERRUPT_CLEAR_PEND;
	//~ *clearPending = (1<<20);
	if (intStatus & (1<<5))
	{
		os_takeSpinLock(LOCK_UART0_OUT);
		while ( (uart0ReadIndex!=uart0WriteIndex) && ((uart->flags&(1<<5))==0) )
		{
			uart->data = uart0CircularBuffer[uart0ReadIndex++];
			uart0ReadIndex &= SIZE_PRINT_BUFF - 1;
		}
		os_giveSpinLock(LOCK_UART0_OUT);
	} else {
		// set interrupts to tx only because we do NOT want to spawn multiple
		// tasks all trying to read from the uart. This ensures only one task
		// is created. Inside of uart0SingleChar we turn back on interrupts
		// after we have drained the fifo to empty.
		uart->intMaskSet = (1<<5);
		//~ addTaskToQueue(uart0SingleChar, uart, 2);
		os_createProcess(uart0SingleChar, uart, 0);
	}
}

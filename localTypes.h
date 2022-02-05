#ifndef LOCALTYPES_HEADER
#define LOCALTYPES_HEADER

typedef unsigned char  u8;
typedef signed char    s8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;
typedef void (*genericFunction)(void *);

// assembly related symbols

s32 asmDiv(s32 dividend, s32 divisor);
s32 asmMod(s32 dividend, s32 divisor);
s32 asmGetDiv(void);
void os_takeSpinLock(u32 lockNum);
void os_giveSpinLock(u32 lockNum);
void dmaWordForwardCopy(void *src, void *dst, s32 size);
void setZero(void *dst, s32 size);
void setZeroWait(void);
void _setR8(void);
void copyBackward(void *src, void* dst, u32 size);
void copyForward(void *src, void* dst, u32 size);
void xipSetup(void);
void configDmaAtl1Data(void *read, void *write, u32 ctrl, void *dma);
void os_processWrapper(void *arg1, void *arg2, void *entryPoint);
void REBOOT(void);

extern u32 vector_table[];
extern u32 __bss_end__[];
extern u32 __bss_start__[];
extern u32 boot2Entry[];

#define LOCK_MEMORY_ALLOC  0
#define LOCK_PROCESS_QUEUE 1
#define LOCK_UART0_OUT     2
#define LOCK_UART0_IN      3
#define LOCK_ALARM         4
#define LOCK_FLASH         5
#define LOCK_COPY_DMA      6


#define SIZE_TASK_QUEUE 32
#define SIZE_PRINT_BUFF (1<<11)

#define ATOMIC_XOR         = 0x1000
#define ATOMIC_SET         = 0x2000
#define ATOMIC_CLR         = 0x3000

#define SIO_BASE        0xD0000000
#define SIO_FIFO_ST    (SIO_BASE+0x50)
#define SIO_SPINLOCK_0 (SIO_BASE + 0x16C)

#define PPB_BASE              0xE0000000
#define PPB_SYSTICK              (PPB_BASE+0xE010)
#define PPB_INTERRUPT_ENABLE     (PPB_BASE+0xE100)
#define PPB_INTERRUPT_DISABLE    (PPB_BASE+0xE180)
#define PPB_INTERRUPT_SET_PEND   (PPB_BASE+0xE200)
#define PPB_INTERRUPT_CLEAR_PEND (PPB_BASE+0xE280)
#define PPB_NVIC_IPR6            (PPB_BASE+0xE418)
#define PPB_SYS_ICSR             (PPB_BASE+0xED04)
#define PPB_SYS_CNTRL            (PPB_BASE+0xED10)
#define PPB_SYS_SHPR2            (PPB_BASE+0xED20)


#define FIFO_BUFFER      0x20040000
#define FIFO_WRITER_ADDR 0x20040100
#define FIFO_READER_ADDR 0x20040104

#define UART0_BASE         0x40034000
#define UART0_DR           0x000
#define UART0_FR           0x018
#define UART0_IBRD         0x024
#define UART0_FBRD         0x028
#define UART0_LCR          0x02C
#define UART0_CR           0x030
#define UART0_IRMASK       0x038
#define UART0_IRMASKSTATUS 0x040
#define UART0_IRCLEAR      0x044
#define UART0_DMA_CR       0x048

#define TIMER_BASE        0x40054000

#define FLASH_BASE        0x10000000

#define END_OF_RAM        ((u32)0x20042000)

typedef struct Uart0MemMap {
	volatile u32 data;
	volatile u32 errorStatus;
	u32 pad1; // 8
	u32 pad2; // C
	u32 pad3; // 10
	u32 pad4; // 14
	volatile u32 flags; // 18
	u32 pad5; // 1C
	volatile u32 lowPower; // 20
	volatile u32 integerBaud; // 24
	volatile u32 fracBaud; // 28
	volatile u32 lineControl; // 2C
	volatile u32 controlReg; // 30
	volatile u32 fifoLevelSelect; // 34
	volatile u32 intMaskSet; // 38
	volatile u32 rawIntStatus; // 3C
	volatile u32 maskedIntStatus; // 40
	volatile u32 intClear; // 44
	volatile u32 dmaCntrl; // 48
} Uart0MemMap;

typedef struct TimerMemMap {
	volatile u32 timeWriteHigh;
	volatile u32 timeWriteLow;
	volatile u32 timeReadHigh;
	volatile u32 timeReadLow;
	volatile u32 alarm[4];
	volatile u32 armed;
	volatile u32 timeReadHighRaw;
	volatile u32 timeReadLowRaw;
	volatile u32 debugPause;
	volatile u32 pause;
	volatile u32 intr;
	volatile u32 inte;
	volatile u32 intf;
	volatile u32 ints;
} TimerMemMap;

typedef struct ProcessInfo ProcessInfo;
typedef struct ProcessInfo {
	ProcessInfo *next;
	u32         *stack;
} ProcessInfo;

typedef struct TokenInfo {
	s32  tokenType;
	u8  *string;
	s32  length;
	u32  lineNumber;
} TokenInfo;

typedef struct {
	s32  exprType; // must always be first
	u8  *string;
	s32  length;
	u32  lineNumber;
} Literal;

typedef union NonTerminal {
	TokenInfo tok;
	Literal   lit;
} NonTerminal;

typedef struct {
	u32 csr;
	u32 rvr;
	u32 cvr;
} SYS_TICK_S;

typedef struct {
	void (*_connect_internal_flash)(void);
	void (*_flash_exit_xip)(void);
	void (*_flash_range_erase)(u32 addr, u32 count, u32 block_size, u8 block_cmd);
	void (*flash_range_program)(u32 addr, u8 *data, u32 count);
	void (*_flash_flush_cache)(void);
	u8*  (*memcpy)(void *dest, void *src, u32 count);
	u8*  (*memset)(void *dest, u32 val, u32 count);
} ROMFunctions;

extern ROMFunctions rom_func;
extern void (*lineHandler)(u8 *in);

void printWord(u32 data);
//~ void io_prints(u8 *string);
void
os_createProcess(void *entryPoint, void *arg1, void *arg2);
void os_enterIdleProcess(void);
ProcessInfo* _getCurrentProcess(void);
void asm_processChar(u32 in);
void shell_inputLine(u8 *input);
void shell_init(void);
u32 st_len(u8 *string);
void os_programBootStrap(void *bootCode);

#include "avl.h"
#include "inc/os.h"
#include "inc/list.h"
#include "inc/memory.h"
#include "inc/timer.h"
#include "inc/helperCpu.h"
#include "inc/terminal.h"
#include "inc/lex.h"
#include "inc/simple.h"
#include "inc/io.h"
#include "inc/string.h"
#include "inc/util.h"
#include "inc/entryPoint.h"
#include "inc/init.h"


#endif

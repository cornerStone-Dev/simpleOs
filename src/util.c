// util.c

#include "../localTypes.h"

#define DMA_BASE 0x50000000
#define DMA_SNIFF_CTRL (DMA_BASE + 0x434)
#define DMA_0_ALT1     (DMA_BASE + 0x010)
#define DMA_3_ALT1     (DMA_BASE + 0x0D0)

typedef struct {
	volatile u32 ctrl;
	volatile u32 data;
} DmaSniffCtrl;

typedef struct {
	volatile u32   ctrl;
	volatile void *read;
	volatile void *write;
	volatile u32   count;
} DmaAlt1Ctrl;

/*e*/
u32 ut_crcCopy(void *data, void *dest, u32 length)/*p;*/
{
	DmaSniffCtrl *dmaSniff = (void*)DMA_SNIFF_CTRL;
	DmaAlt1Ctrl  *dma0     = (void*)DMA_0_ALT1;
	u32          dmaTarget;
	u32          ctrl;
	void *read, *write;
	dmaSniff->ctrl = 1;
	dmaSniff->data = -1;
	ctrl           = (1<<23)|(0x3f<<15)|(1<<4)|(1<<0);
	read           = data;
	write          = &dmaTarget;
	// choose settings
	if ( (((u32)data & 0x03) == 0) && ((length & 0x03) == 0) )
	{
		// aligned AND multiple of 4, use fast settings
		ctrl  += 2<<2;
		length = length / 4;
	} else {
		// unaligned OR length not multiple of 4, use slower settings
	}
	if (dest)
	{
		// destination address provided, so do an actual copy
		ctrl += 1<<5;
		write = dest;
	}
	// send dma settings
	configDmaAtl1Data(read, write, ctrl, dma0);
	// write final setting that is also the trigger
	dma0->count    = length;
	// wait for the transfer to complete
	while (dma0->ctrl & (1<<24)) { }
	return dmaSniff->data;
}

/*e*/
void
ut_memSet(void *target, u32 byte, u32 count)/*p;*/
{
	(void)rom_func.memset(target, byte, count);
}

/*e*/
void
ut_memCopy(void *dest, void *src, u32 count)/*p;*/
{
	(void)rom_func.memcpy(dest, src, count);
}

/*e*/
void
ut_dmaCopy(void *src, void *dst, u32 count)/*p;*/
{
	asm("CPSID i"); // disable interrupts
	os_takeSpinLock(LOCK_COPY_DMA);
	dmaWordForwardCopy(src, dst, count);
	os_giveSpinLock(LOCK_COPY_DMA);
	asm("CPSIE i"); // enable interrupts
}

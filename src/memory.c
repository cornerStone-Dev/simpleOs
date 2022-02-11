/*
** A minimum allocation is an instance of the following structure.
** Larger allocations are an array of these structures where the
** size of the array is a power of 2.
**
** The size of this object must be a power of two.  That fact is
** verified in memsys5Init().
*/

#include "../localTypes.h"

typedef struct MemDLL MemDLL;
struct MemDLL {
  MemDLL *next;       /* Index of next free chunk */
  MemDLL *prev;       /* Index of previous free chunk */
};


/*
** Maximum size of any allocation is ((1<<LOGMAX)*mem.szAtom). Since
** mem.szAtom is always at least 8 and 32-bit integers are used,
** it is not actually possible to reach this limit.
*/
//~ #define LOGMAX 30
#define LOGMAX 15 /* pico's memory is much smaller */
#define ATOM_SIZE 32

/*
** Masks used for mem.aCtrl[] elements.
*/
#define CTRL_LOGSIZE  0x1f    /* Log2 Size of this block */
#define CTRL_FREE     0x20    /* True if not checked out */

/*
** All of the static variables used by this module are collected
** into a single structure named "mem".  This is to keep the
** static variables organized and to reduce namespace pollution
** when this module is combined with other in the amalgamation.
*/

typedef struct MemGlobal
{
	MemDLL sentinalNode; /* Sentinal Node used in free lists */
	u32 nBlock;      /* Number of szAtom sized blocks in zPool */
	u8 *zPool;       /* Memory available to be allocated */
	/*
	** Space for tracking which blocks are checked out and the size
	** of each block.  One byte per block.
	*/
	u8 *aCtrl;
	void *cache;
	/*
	** Lists of free blocks.  aiFreelist[0] is a list of free blocks of
	** size mem.szAtom.  aiFreelist[1] holds blocks of size szAtom*2.
	** aiFreelist[2] holds free blocks of size szAtom*4.  And so forth.
	*/
	MemDLL *aiFreelist[LOGMAX+1];
} MemGlobal;

static MemGlobal mem;

/*
** Assuming mem.zPool is divided up into an array of memLink
** structures, return a pointer to the idx-th such link.
*/
#define GET_MEMDLL(idx) ((MemDLL *)(&mem.zPool[(idx)*ATOM_SIZE]))

/*
** Unlink the chunk at mem.aPool[i] from list it is currently
** on.  It should be found on mem.aiFreelist[iLogsize].
*/

static void
memsys5Unlink(MemDLL *l){
  MemDLL *next, *prev;
  next = l->next;
  prev = l->prev;
  prev->next = next;
  next->prev = prev;
}

/*
** Link the chunk at mem.aPool[i] so that is on the iLogsize
** free list.
*/
static void
memsys5Link(MemDLL * restrict l, u32 iLogsize, MemDLL ** restrict freeList){

  l->next = freeList[iLogsize];
  l->prev = (MemDLL *)&freeList[iLogsize];
  freeList[iLogsize]->prev = l;
  freeList[iLogsize] = l;
}

/*
** Return the size of an outstanding allocation, in bytes.
** This only works for chunks that are currently checked out.
*/
static u32 memsys5Size(void *p){
  u32 iSize, i;
  i = ((u32)((u8 *)p-mem.zPool)/ATOM_SIZE);
  iSize = ATOM_SIZE * (1 << mem.aCtrl[i]);
  return iSize;
}

/*
** Return a block of memory of at least nBytes in size.
** Return NULL if unable.  Return NULL if nBytes==0.
*/
static void *zalloc_internal(u32 iFullSz, u32 iLogsize)
{
	u32 i;           /* Index of a mem.aPool[] slot */
	u32 iBin;        /* Index into mem.aiFreelist[] */
	//~ u32 iFullSz;     /* Size of allocation rounded up to power of 2 */
	//~ u32 iLogsize;    /* Log2 of iFullSz/POW2_MIN */
	MemDLL *freeNode;

	// if nByte is 0 -> return 0 to be consistent with realloc
	//~ if (nByte==0) { return 0; }


	/* Round nByte up to the next valid power of two */
	//~ for(iFullSz=ATOM_SIZE,iLogsize=0; iFullSz<nByte; iFullSz*=2,iLogsize++){}

	/* Make sure mem.aiFreelist[iLogsize] contains at least one free
	** block.  If not, then split a block of the next larger power of
	** two in order to create a new free block of size iLogsize.
	*/
	for(iBin=iLogsize; mem.aiFreelist[iBin]==&mem.sentinalNode; iBin++){}
	if( iBin>=LOGMAX ){
		return 0;
	}
	// get free node from free list
	freeNode = mem.aiFreelist[iBin];
	// unlink the node
	memsys5Unlink(freeNode);
	
	// set start to zeros
	//~ freeNode->next = 0;
	//~ freeNode->prev = 0;

	// set memory to zero using DMA
	void *memory = freeNode;
	setZeroWait();
	setZero(memory, iFullSz);
	i = ((u32)((u8 *)memory-mem.zPool)/ATOM_SIZE);
	mem.aCtrl[i] = iLogsize;

	while( iBin>iLogsize ){
		u32 newSize;
		iBin--;
		newSize = 1 << iBin;
		mem.aCtrl[i+newSize] = CTRL_FREE + iBin;
		memsys5Link(GET_MEMDLL(i+newSize), iBin, mem.aiFreelist);
	}

	/* Return a pointer to the allocated memory. */
	return memory;
}

/*
** Free an outstanding memory allocation.
*/
static void free_internal(void *pOld)
{
	u32 iLogsize;
	u32 iBlock;
	u8 *ctrlMem = mem.aCtrl;

  /* Set iBlock to the index of the block pointed to by pOld in 
  ** the array of mem.szAtom byte blocks pointed to by mem.zPool.
  */
	//~ if (pOld==0) { return; }
	iBlock = ((u32)((u8 *)pOld-mem.zPool)/ATOM_SIZE);
	iLogsize = ctrlMem[iBlock];
	//~ setZeroWait();
	//~ setZero(pOld, 1<<iLogsize);
	ctrlMem[iBlock] = CTRL_FREE + iLogsize;
	while(1) {
		u32 iBuddy;
		iBuddy = iBlock ^ (1<<iLogsize);
		if(iBuddy>=mem.nBlock) { break; }
		if(ctrlMem[iBuddy]!=(CTRL_FREE + iLogsize)) { break; }
		memsys5Unlink(GET_MEMDLL(iBuddy));
		iLogsize++;
		ctrlMem[iBlock&iBuddy] = CTRL_FREE + iLogsize;
		ctrlMem[iBlock|iBuddy] = 0;
		iBlock = iBlock&iBuddy;
	}
	memsys5Link(GET_MEMDLL(iBlock), iLogsize, mem.aiFreelist);
}

/*e*/
void free(void *pOld)/*p;*/
{
	if (pOld==0) { return; }
	asm("CPSID i");  // disable interrupts
	os_takeSpinLock(LOCK_MEMORY_ALLOC);
	free_internal(pOld);
	os_giveSpinLock(LOCK_MEMORY_ALLOC);
	asm("CPSIE i"); // enable interrupts
}

/*e*/
void *zalloc(u32 nByte)/*p;*/
{
	u32 iFullSz;     /* Size of allocation rounded up to power of 2 */
	u32 iLogsize;    /* Log2 of iFullSz/POW2_MIN */
	// if nByte is 0 -> return 0 to be consistent with realloc
	if (nByte==0) { return 0; }
	/* Round nByte up to the next valid power of two */
	for(iFullSz=ATOM_SIZE,iLogsize=0; iFullSz<nByte; iFullSz*=2,iLogsize++){}

	asm("CPSID i"); // disable interrupts
	os_takeSpinLock(LOCK_MEMORY_ALLOC);
	void *memory = zalloc_internal(iFullSz, iLogsize);
	os_giveSpinLock(LOCK_MEMORY_ALLOC);
	asm("CPSIE i"); // enable interrupts
	return memory;
}

/*
* Flexible memory allocation function.
* 1. If pPrior is 0 -> call zalloc.
* 2. If nBytes is 0 -> call free. 
* 3. If nBytes > oldSize we get a larger fresh allocation and copy data into it
* 4. If nBytes <= oldSize/2 we get a smaller allocation and copy into it.
* 5. Otherwise we return pPrior 
*/
/*e*/
void *realloc(void *pPrior, u32 nBytes)/*p;*/
{
	u32   oldSize, copyAmount;
	void *newMemory;
	if (pPrior == 0)
	{
		return zalloc(nBytes);
	}
	if (nBytes == 0)
	{
		free(pPrior);
		return 0;
	}
	oldSize = memsys5Size(pPrior);
	if (nBytes>oldSize)
	{
		copyAmount = oldSize;
	} else if (nBytes <= (oldSize/2)) {
		copyAmount = nBytes;
	} else {
		return pPrior;
	}
	// standard realloc logic
	newMemory = zalloc(nBytes);
	if (newMemory)
	{
		os_takeSpinLock(LOCK_COPY_DMA);
		dmaWordForwardCopy(pPrior, newMemory, copyAmount);
		os_giveSpinLock(LOCK_COPY_DMA);
		//~ ut_memCopy(newMemory, pPrior, copyAmount);
		free(pPrior);
	}
	return newMemory;
}

/*e*/
void *allocStack(void)/*p;*/
{
	asm("CPSID i"); // disable interrupts
	os_takeSpinLock(LOCK_MEMORY_ALLOC);
	void *stack = mem.cache;
	mem.cache = zalloc_internal(512, 4);
	os_giveSpinLock(LOCK_MEMORY_ALLOC);
	asm("CPSIE i"); // enable interrupts
	return stack;
}

/*
** Initialize the memory allocator.
*/
/*e*/
void memSysInit(void)/*p;*/
{
	s32 ii;            /* Loop counter */
	u32 iOffset;       /* An offset into mem.aCtrl[] */
	
	mem.zPool = ((u8*)__bss_end__ + 512); // start of memory
	mem.nBlock = asmDiv(END_OF_RAM - (u32)mem.zPool, 33); // heapsize / (ATOM+1)
	mem.aCtrl = &mem.zPool[mem.nBlock*ATOM_SIZE]; // start of control

	// initialize free lists to sentinal
	for(ii=0; ii<LOGMAX; ii++){
	mem.aiFreelist[ii] = &mem.sentinalNode;
	}
	// set sentinal free list to a value other than sentinal node
	mem.aiFreelist[LOGMAX] = (MemDLL*)&mem.aiFreelist[LOGMAX];
	// fill free lists with largest possible blocks of powers of 2
	iOffset = 0;
	for(ii=LOGMAX; ii>=0; ii--)
	{
		int nAlloc = (1<<ii);
		if( (iOffset+nAlloc)<=mem.nBlock ){
			mem.aCtrl[iOffset] = ii + CTRL_FREE;
			memsys5Link(GET_MEMDLL(iOffset), ii, mem.aiFreelist);
			iOffset += nAlloc;
		}
	}
	(void)allocStack();
	return;
}

/*e*/
void printMemStats(void)/*p;*/
{
	u32 totalMemory = mem.nBlock * 32;
	io_printi(totalMemory);
	io_prints(": bytes of memory under stewardship.\n");
	u32     memReady = 0;
	MemDLL *cursor   = 0;
	u32     memSize  = 32;
	for (u32 x = 0; x < LOGMAX; x++)
	{
		cursor = mem.aiFreelist[x];
		while (cursor != &mem.sentinalNode)
		{
			memReady += memSize;
			cursor = cursor->next;
		}
		memSize *= 2;
	}
	io_printi(totalMemory - memReady);
	io_prints(": bytes of memory in use.\n");
	io_printi(memReady);
	io_prints(": bytes of memory remaining.\n");
}

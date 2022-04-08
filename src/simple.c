// simple.c
#include "../localTypes.h"
#include "../gram.h"
#include "../inc/simple_i.h"
// top level compile function
// input is source code text.
// output is address to main (needs lowest bit set) to be called or nothing
// system symbol table is updated with all public functions
// compiling with functions with the same name overwrites those symbol table
// entries.

// I tried to directly encode optimized stack machine to register code.
// This turns out to be very difficult with many special cases. Most of these
// special cases are specific to the instruction set available and others are
// specific to the ABI and others still are specific to the assignment
// expression being very annoying. For the instruction set and ABI you would
// want to separate that out into a different phase, for assignment, a AST would
// be the ideal solution. So I could go with a middle ground of IR generation
// or I could back off on optimizations and generate code. For example:y= x + 5
// push x, push 5, add store -> push x add 5

// ok for some reason (maybe x86 encoding?) I had disreguarded the ability for
// the look ahead to take care of the assignment expression pain. I think for
// ARM it can take care of the issue. In addition for function calls only 2
// cases apply, direct function call or indirect and it is possible to tell the
// the difference by the symbol type. I may be able to get back on the horse
// for direct code generation.

typedef struct {
	s32 type;
	s32 regNum;
	s32 lineNumber;
} LocalVariable;

typedef struct {
	avlNode *symbols;
	s32      regNum;
} LocalScope;

typedef struct {
	s32  type;
	s32  returnType;
	u16 *startOfFunction;
	s32  argType[4];
} FunctionInfo;

typedef struct {
	List *next;
	s32   codeIndex;
	s32   value;
} RelativeLoad;

typedef struct CompilerContext {
	FunctionInfo *currentFunc;
	avlNode      *globals;
	u16          *codeBuff;
	u32           codeIndex;
	u32           codeBuffSize;
	s32           scopeIndex;
	s32           regHighWater;
	s32           returnIndex;
	s32           stackState;
	RelativeLoad *workList;           
	u8            error;
	LocalScope    scopes[8];
} CompilerContext;

static CompilerContext c;

enum {
	T_VOID = LAST,
	T_IDENT,
	T_U8,
	T_U8_R,
	T_U8_RR,
	T_S32,
	T_S32_R,
	T_S32_RR,
};

enum {
	STACK_INIT = -1,
	STACK_1,
};

static void
enterScope(void)
{
	c.scopeIndex++;
	c.scopes[c.scopeIndex].regNum = c.scopes[c.scopeIndex-1].regNum;
}

static void
leaveScope(void)
{
	avl_destroy(&c.scopes[c.scopeIndex--].symbols);
}

static void
putMachineCode(u32 code)
{
	if (c.codeIndex >= c.codeBuffSize)
	{
		c.codeBuffSize *= 2;
		c.codeBuff = realloc(c.codeBuff, c.codeBuffSize*2);
	}
	c.codeBuff[c.codeIndex++] = code;
}

static void
localDecl(NonTerminal *decl)
{
	LocalVariable *local = zalloc(sizeof(LocalVariable));
	local->type   = decl->lit.type;
	local->regNum     = c.scopes[c.scopeIndex].regNum++;
	if (local->regNum+1 > c.regHighWater) { c.regHighWater = local->regNum+1; }
	local->lineNumber = decl->lit.lineNumber;
	avlNode *node = avl_insert(
		&c.scopes[c.scopeIndex].symbols,
		decl->lit.string, decl->lit.length, local);
	if (node) { io_prints("duplicate local variable in this scope.\n"); }
}

static void
simple_binaryOp(NonTerminal *left, NonTerminal *right, s32 type)
{
	//~ io_prints("Binary Operation.\n");
	//~ if (left->lit.type == INTEGER && right->lit.type == INTEGER)
	//~ {
		//~ io_prints("constant folding.\n");
		//~ switch (type)
		//~ {
			//~ case PLUS:
			//~ left->lit.length += right->lit.length;
			//~ break;
			//~ default:;
		//~ }
		//~ return;
	//~ }
}

static u32
armBX(u32 reg)
{
	u32 code = 0x4700;
	code += reg << 3;
	return code;
}

static u32
armAdd3(u32 dest, u32 arg1, u32 arg2)
{
	u32 code = 0x1800;
	code += dest + (arg1 << 3) + (arg2 << 6);
	io_prints("add r");
	io_printi(arg1);
	io_prints(" to r");
	io_printi(arg2);
	io_prints(" save to r");
	io_printi(dest);
	io_prints("\n");
	return code;
}

static u32
armSub3(u32 dest, u32 arg1, u32 arg2)
{
	u32 code = 0x1A00;
	code += dest + (arg1 << 3) + (arg2 << 6);
	io_prints("sub r");
	io_printi(arg1);
	io_prints(" to r");
	io_printi(arg2);
	io_prints(" save to r");
	io_printi(dest);
	io_prints("\n");
	return code;
}

static u32
armPush(u32 regBits)
{
	u32 code = 0xB400;
	code += regBits;
	return code;
}

static u32
armPop(u32 regBits)
{
	u32 code = 0xBC00;
	code += regBits;
	return code;
}

static u32
armPushFuncStart(s32 numVars)
{
	u32 regBits = 0;
	for (s32 x = 0; x < numVars && x < 4; x++)
	{
		regBits <<= 1;
		regBits++;
	}
	regBits <<= 4;
	regBits += (1<<8);
	return armPush(regBits);
}

static u32
armPopFuncEnd(s32 numVars)
{
	u32 regBits = 0;
	for (s32 x = 0; x < numVars && x < 4; x++)
	{
		regBits <<= 1;
		regBits++;
	}
	regBits <<= 4;
	regBits += (1<<8);
	return armPop(regBits);
}

static u32
armSubSP(s32 numVars)
{
	if (numVars < 0) { numVars = 0; }
	u32 code = 0xB080;
	code += numVars;
	return code;
}

static u32
armAddSP(s32 numVars)
{
	if (numVars < 0) { numVars = 0; }
	u32 code = 0xB000;
	code += numVars;
	return code;
}

static u32
armMov(u32 dest, u32 src)
{
	u32 code = 0x4600;
	code += ((dest>>3)<<7) + ((dest<<29)>>29) + (src << 3);
	return code;
}

static u32
armMovImm(u32 dest, u32 val)
{
	u32 code = 0x2000;
	code += val + (dest << 8);
	return code;
}

static void
simpleReturn(void)
{
	// check if we issued a return already
	if (c.returnIndex)
	{
		// we will issue a branch to the previously generated return
	} else {
		// put in blanks for the return code
		c.returnIndex = c.codeIndex;
		putMachineCode(0);
		putMachineCode(0);
	}
}

static s32
sm_getTos(void)
{
	if (c.stackState <= 3)
	{
		return c.stackState - 1;
	} else {
		return 3;
	}
}

static s32
sm_getScratch(void)
{
	if (c.stackState <= 4)
	{
		return c.stackState - 2;
	} else {
		return 2;
	}
}

static void
sm_restoreScratch(void)
{
	if (c.stackState == 4)
	{
		// pop into r2 to restore scratch
		putMachineCode(armPop(1<<2));
		io_prints("restore r2 as scratch\n");
	}
}

static void
sm_popScratch(void)
{
	if (c.stackState <= 4)
	{
		// top of stack is within register state machine, do nothing
	} else if (c.stackState == 5) {
		// pop into r2 as the scratch, see restore scratch for second part
		putMachineCode(armPop(1<<2));
	} else {
		// pop into r2 as the scratch
		putMachineCode(armPop(1<<2));
	}
	c.stackState--;
}

static void
sm_pushTos(void)
{
	// 0 = initial state , 4 = full
	if (c.stackState <= 3)
	{
		// top of stack is within register state machine, do nothing
	} else if (c.stackState == 4) {
		// push r2 because we need a scratch register
		putMachineCode(armPush(1<<2));
		// push r3 (previous TOS) to stack
		putMachineCode(armPush(1<<3));
	} else {
		// push r3 (previous TOS) to stack
		putMachineCode(armPush(1<<3));
	}
	c.stackState++;
}

static void
sm_popTos(void)
{
	// 0 = initial state , 4 = full
	if (c.stackState <= 4)
	{
		// top of stack is within register state machine, do nothing
	} else if (c.stackState == 5) {
		// pop r3 to restore TOS
		putMachineCode(armPop(1<<3));
		// pop r2 to restore scratch pad register
		putMachineCode(armPop(1<<2));
	} else {
		// pop r3 to restore TOS
		putMachineCode(armPop(1<<3));
	}
	c.stackState--;
}

/*e*/
static void
stackAdd(void)/*i;*/
{
	s32 tos1 = sm_getTos();
	s32 scratch = sm_getScratch();
	sm_popScratch();
	s32 tos2 = sm_getTos();
	putMachineCode(armAdd3(tos2, scratch, tos1));
	sm_restoreScratch();
}

/*e*/
static void
stackSub(void)/*i;*/
{
	s32 tos1 = sm_getTos();
	s32 scratch = sm_getScratch();
	sm_popScratch();
	s32 tos2 = sm_getTos();
	putMachineCode(armSub3(tos2, scratch, tos1));
	sm_restoreScratch();
}

static void
pushVal(s32 val)
{
	sm_pushTos();
	s32 tos = sm_getTos();
	if (val <= 255 && val>=0)
	{
		// small value
		putMachineCode(armMovImm(tos, val));
	} else {
		// deal with larger const by adding a worklist item
		RelativeLoad *new = zalloc(sizeof(RelativeLoad));
		new->codeIndex = c.codeIndex;
		new->value = val;
		c.workList = list_append(new, c.workList);
	}
	// debug output
	io_prints("push value ");
	io_printi(val);
	io_prints(" to r");
	io_printi(tos);
	io_prints("\n");
}

static void
pushVar(s32 varNum)
{
	sm_pushTos();
	s32 tos = sm_getTos();
	if (varNum <= 3)
	{
		// register var
		putMachineCode(armMov(tos, varNum+4));
	} else {
		// deal with loading from the stack
	}
	// debug output
	//~ io_printsl(A.string, A.length);
	io_prints("push varNum ");
	io_printi(varNum);
	io_prints(" to r");
	io_printi(tos);
	io_prints("\n");
}

static void
storeVar(s32 varNum)
{
	s32 tos = sm_getTos();
	if (varNum <= 3)
	{
		// register var
		putMachineCode(armMov(varNum+4, tos));
	} else {
		// deal with loading from the stack
	}
	sm_popTos();
	// debug output
	io_prints("");
	//~ io_printsl(A.string, A.length);
	io_prints("store to varNum ");
	io_printi(varNum);
	io_prints(" from r");
	io_printi(tos);
	io_prints("\n");
}

#include "../gram.c"

/*e*/
void*
simpleCompile(u8 *sourceCode)/*p;*/
{
	// in addition to the above there is a global persistent context for public
	// functions so that they may be used. They are pulled in through modules?
	TokenInfo t;
	u8 *cursor;
	c.error = 0;
	t.lineNumber = 1;
	cursor = sourceCode;
	void *pParser = ParseAlloc(zalloc);
	do {
		cursor = lex(cursor, &t);
		//~ io_printi(t.type);io_txByte('\n');
		if (t.type >= 0) { Parse(pParser, t.type, &t); }
	} while(t.type != 0);
	// free parser data structure
	ParseFree(pParser, free);
	// find main function
	avlNode *mainNode = avl_find(c.globals, "main", 4);
	if (mainNode)
	{
		// a main function was created
		FunctionInfo *mainFunction = mainNode->value;
		//~ void *function = mainFunction->startOfFunction;
		// delete it so other "main" functions can be made
		avl_delete(&c.globals, "main", 4);
		// return the pointer to main
		return (void*)((u32)mainFunction->startOfFunction+1);
	} else {
		// no main created, return zero
		return 0;
	}
}

static u8 *testProgramText = "f s32\n"
"main(s32 x, s32 y)\n"
"{\n"
"return x + y;\n"
"}\n";

static u8 *testProgramText2 = "f s32\n"
"fibbo(s32 n)\n"
"{\n"
"if n > 1\n"
"{\n"
"n = fibbo(n - 1) + fibbo(n - 2);\n"
"}\n"
"return n;\n"
"}\n"
"f s32\n"
"main()\n"
"{\n"
"s32 n;\n"
"n = fibbo(20);\n"
"return n;\n"
"}\n";

/*e*/
void
testCompiler(void)/*p;*/
{
	//~ simpleCompile(testProgramText2);
	//~ s32 (*adderf)(s32 x, s32 y) = zalloc(32);
	s32 (*adderf)(s32 x, s32 y) = simpleCompile(testProgramText);
	s32 (*fibbo)(void) = simpleCompile(testProgramText2);
	//~ io_printh((s32)adderf);
	//~ u16 *cursor = (u16*)adderf;
	//~ cursor[0] = armAdd3(0, 0, 1);
	//~ cursor[1] = armBX(14);
	//~ adderf = (void*)((u32)adderf + 1);
	io_printi(adderf(5, 20)); io_prints("\n");
	io_printi(fibbo()); io_prints("\n");
	//~ free(cursor);
}




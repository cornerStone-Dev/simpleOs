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
	stackAdd();
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
getTos(s32 stackState)
{
	if (stackState < 3)
	{
		return stackState;
	} else {
		return 3;
	}
}

static s32
popScratch(void)
{
	c.stackState--;
	s32 scratchRegister;
	if (c.stackState < 3)
	{
		// no need to pop is scratch just pop
		scratchRegister = c.stackState;
	} else if (c.stackState == 3) {
		// stack state has gone beyond registers
		return 0;
	}
	return scratchRegister;
}

static void
stackPush(void)
{
	c.stackState++;
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
}

/*e*/
static void
stackAdd(void)/*i;*/
{
	s32 tos1 = getTos(c.stackState);
	s32 scratch = popScratch();
	s32 tos2 = getTos(c.stackState);
	putMachineCode(armAdd3(tos2, scratch, tos1));
	// debug
	io_prints("add register ");
	io_printi(tos1);
	io_prints(" to register ");
	io_printi(scratch);
	io_prints(" save to register ");
	io_printi(tos2);
	io_prints("\n");
}

static void
pushVal(s32 val)
{
	stackPush();
	s32 tos = getTos(c.stackState);
	if (val <= 255 && val>=0)
	{
		// small value
		putMachineCode(armMovImm(tos, val));
	} else {
		// deal with larger const
	}
	// debug output
	io_prints("push value ");
	io_printi(val);
	io_prints(" to register ");
	io_printi(tos);
	io_prints("\n");
}

static void
pushVar(s32 varNum)
{
	stackPush();
	s32 tos = getTos(c.stackState);
	if (varNum <= 3)
	{
		// register var
		putMachineCode(armMov(tos, varNum+4));
	} else {
		// deal with loading from the stack
	}
	// debug output
	io_prints("push variable");
	//~ io_printsl(A.string, A.length);
	io_prints(" from varNum ");
	io_printi(varNum);
	io_prints(" to register ");
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
		void *mainFunction = mainNode->value;
		// delete it so other "main" functions can be made
		avl_delete(&c.globals, "main", 4);
		// return the pointer to main
		return (void*)((u32)mainFunction+1);
	} else {
		// no main created, return zero
		return 0;
	}
}

static u8 *testProgramText = "f s32\n"
"add(s32 x, s32 y)\n"
"{\n"
"return x + y;\n"
"}\n";

static u8 *testProgramText2 = "f s32\n"
"fibbo(s32 n)\n"
"{\n"
"n = 2 + 2 + n;\n"
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
	simpleCompile(testProgramText2);
	s32 (*adderf)(s32 x, s32 y) = zalloc(32);
	u16 *cursor = (u16*)adderf;
	cursor[0] = armAdd3(0, 0, 1);
	cursor[1] = armBX(14);
	adderf = (void*)((u32)adderf + 1);
	io_printi(adderf(5, 20));
	free(cursor);
}




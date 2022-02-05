// simple.c
#include "../localTypes.h"
#include "../gram.h"
// top level compile function
// input is source code text.
// output is address to main (needs lowest bit set) to be called or nothing
// system symbol table is updated with all public functions
// compiling with functions with the same name overwrites those symbol table
// entries.

typedef struct {
	s32 exprType;
	s32 regNum;
	s32 lineNumber;
} LocalVariable;

typedef struct {
	avlNode *symbols;
	s32      regNum;
} LocalScope;

typedef struct {
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
	u8            error;
	LocalScope    scopes[8];
} CompilerContext;

static CompilerContext c;

enum {
	EXPR_INTLIT = LAST,
	EXPR_VOID,
	EXPR_IDENT,
	EXPR_U8,
	EXPR_U8_R,
	EXPR_U8_RR,
	EXPR_S32,
	EXPR_S32_R,
	EXPR_S32_RR,
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
	local->exprType   = decl->lit.exprType;
	local->regNum     = c.scopes[c.scopeIndex].regNum++;
	local->lineNumber = decl->lit.lineNumber;
	avlNode *node = avl_insert(
		&c.scopes[c.scopeIndex].symbols,
		decl->lit.string, decl->lit.length, local);
	if (node) { io_prints("duplicate local variable in this scope.\n"); }
}

static void
simple_binaryOp(NonTerminal *left, NonTerminal *right, s32 tokenType)
{
	io_prints("Binary Operation.\n");
	if (left->lit.exprType == INTEGER && right->lit.exprType == INTEGER)
	{
		io_prints("constant folding.\n");
		switch (tokenType)
		{
			case PLUS:
			left->lit.length += right->lit.length;
			break;
			default:;
		}
		return;
	}
}

static u32
armBX(u32 reg)
{
	u32 code = 0x4700;
	code += reg << 3;
	//~ putMachineCode(code);
	return code;
}

static u32
armAdd3(u32 dest, u32 arg1, u32 arg2)
{
	u32 code = 0x1800;
	code += dest + (arg1 << 3) + (arg2 << 6);
	//~ putMachineCode(code);
	return code;
}

static u32
armPush(u32 numVars)
{
	u32 code = 0x1800;
	//~ code += dest + (arg1 << 3) + (arg2 << 6);
	//~ putMachineCode(code);
	return code;
}

static u32
armMov(u32 numVars)
{
	u32 code = 0x1800;
	//~ code += dest + (arg1 << 3) + (arg2 << 6);
	//~ putMachineCode(code);
	return code;
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
		//~ io_printi(t.tokenType);io_txByte('\n');
		if (t.tokenType >= 0) { Parse(pParser, t.tokenType, &t); }
	} while(t.tokenType != 0);
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




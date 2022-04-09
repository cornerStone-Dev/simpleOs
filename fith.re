// fith.re
#include "../localTypes.h"
#include "../inc/fith_i.h"
// Section Types

typedef long long (*fithFunc)(u32  tosValue, u32 *expressionStackPointer);

typedef struct {
	u32   savedIndex;
	u32   blockType;
} Block;

typedef struct Work {
	struct Work *next;
	u32          workType;
	u32          codeIndex;
	u32          target;
} Work;

typedef struct FithState {
	u8       insideWord;
	u8       bi;
	u8       li;
	u8       insideParams;
	u8       notLeaf;
	u8       pad1;
	u8       pad2;
	u8       pad3;
	u32      tosValue;
	u32     *expressionStackPointer;
	u32     *expressionStack;
	u16     *codeBuff;
	u32      codeIndex;
	u32      codeBuffSize;
	avlNode *words;
	avlNode *globals;
	avlNode *locals;
	Work    *workList;
	Block    blocks[8];
} FithState;

static FithState f;

enum {
	BLOCK_NONE,
	BLOCK_WORD,
};

enum {
	WORK_NONE,
	WORK_WORDCALL,
};

#define ARM_NOP 0xBF00


/*!re2c                              // start of re2c block
	
	scm = "\\" [^\x00\n]*;
	wsp = [ \t\r\n]; // removed \v
	// integer literals
	dec = [0-9]+;
	hex = "0x" [0-9A-F]+; // a-f removed
	integer = "-"? (dec | hex);
	string_lit = ["] ([^"\x00] | ([\\] ["]))* ["];
	//string_lit_chain = string_lit ([ \n\t\r]* string_lit)+;
	//string_lit_chain = ([^"\n] | ([\\] ["]))* ("\n" | ["]);
	string_lit_chain = ([^'\n] | ([\\] [']))* "\n";
	string_lit_end = ([^'\n] | ([\\] [']))* ['];
	mangled_string_lit = ['] ([^'\x00\x03] | ([\\] [']))* "\x00";
	char_lit = [`] ([^`\x03] | ([\\] [`]))* [`];
	wordOperators = [@#$^~;:!?];
	ident = [a-zA-Z_] [a-zA-Z_0-9?!-]*;
	word = [a-zA-Z_] [a-zA-Z0-9?!_]*;
	word_definition = word "{";
	word_def_params = word "(";
	var_declaration = word "$";
	var_assign = word "=";
	var_decl_assign = word "$=";
	const_declaration = word ":=";
	word_increment = word "++";
	function_call_addr = "@" word ;
	function_definition = word ":";

	
*/                                   // end of re2c block

#define WORD_OTHER 0
#define WORD_EXIT  1
#define WORD_NEG    2

static s32 builtInWord(u8 *YYCURSOR, s32 length)
{
	u8 *YYMARKER;	
switch (length)
{
	case 1:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	case 2:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	case 3:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	"neg" { return WORD_NEG; }
	*/                                  // end of re2c block
	case 4:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	"exit" { return WORD_EXIT; }
	*/                                  // end of re2c block
	case 5:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	case 6:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	case 7:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	case 8:
	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	* { return WORD_OTHER; }
	*/                                  // end of re2c block
	default: return WORD_OTHER;
}
}

/*e*/
void picoFith(u8 *sourceCode)/*p;*/
{
	u8 *YYCURSOR = sourceCode;
	u8 *YYMARKER;
	u8 *start;

	loop:
	start = YYCURSOR;

	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;

	* { io_prints("fith: lexical error\n"); return; }
	[\x00] {
		if (f.codeIndex == 1) { return; }
		if (f.insideWord) { return; }
		// assume execute immediately
		fithFunc exec = (fithFunc)((u32)completeWord() + 1);
		long long result = exec(f.tosValue, f.expressionStackPointer);
		f.tosValue = result;
		f.expressionStackPointer = (u32*)(u32)(result >> 32);
		u32 *base = f.expressionStack;
		if (base != f.expressionStackPointer)
		{
			base++;
			while (base != f.expressionStackPointer) {
				io_prints("(");
				io_printi(*base++);
				io_prints(")");
			}
			io_prints("(");
			io_printi(f.tosValue);
			io_prints(")\n");
		}
		free(f.codeBuff);
		f.codeBuff = zalloc(32);
		f.codeIndex = 0;
		f.codeBuffSize = 16;
		putMachineCode(ARM_NOP);
		return;
	}
	wsp { goto loop; }
	scm { goto loop; }
	
	"+" { stackAdd(); goto loop; }
	"-" { stackSub(); goto loop; }
	">>" { stackRS(); goto loop; }
	"<<" { stackLS(); goto loop; }
	"*" { stackMul(); goto loop; }
	"/" { stackDiv(); goto loop; }
	"%" { stackMod(); goto loop; }
	"&" { stackAnd(); goto loop; }
	"|" { stackOr(); goto loop; }
	"^" { stackXor(); goto loop; }
	"~" { stackNot(); goto loop; }
	"&~" { stackBitClear(); goto loop; }
	
	"}" { endBlock(); goto loop; }
	"){" { endParams(); goto loop; }
	
	integer { s32 val = s2i(start); pushVal(val); goto loop; }
	
	word {
		s32 wordType = builtInWord(start, YYCURSOR - start);
		switch (wordType) {
			case WORD_EXIT:
			lineHandler = shell_inputLine; return;
			case WORD_NEG:
			negTos(); break;
		}
		speakWord(start, YYCURSOR - start);
		goto loop;
	}
	
	word_definition {
		s32 wordType = builtInWord(start, YYCURSOR - start - 1);
		if (wordType != 0)
		{
			
		} else {
			createWord(start, YYCURSOR - start - 1);
		}
		goto loop;
	}
	
	word_def_params {
		s32 wordType = builtInWord(start, YYCURSOR - start - 1);
		if (wordType != 0)
		{
			
		} else {
			createWord(start, YYCURSOR - start - 1);
			f.insideParams = 1;
		}
		goto loop;
	}
	
	var_declaration {
		u32 localNum = createVar(start, YYCURSOR - start - 1);
		setVar(localNum);
		goto loop;
	}
	
	var_assign {
		io_prints("assign\n");
		goto loop;
	}
	
	*/                                  // end of re2c block
}

/*e*/
void fith_init(void)/*p;*/
{
	f.expressionStack = zalloc(32*4);
	f.expressionStackPointer = f.expressionStack;
	f.codeBuff = zalloc(32);
	f.codeBuffSize = 16;
	putMachineCode(ARM_NOP);
	return;
}

// Section Arm Machine Code

/*e*/static u32
armPush(u32 regBits)/*i;*/
{
	u32 code = 0xB400;
	code += regBits;
	return code;
}

/*e*/static u32
armPop(u32 regBits)/*i;*/
{
	u32 code = 0xBC00;
	code += regBits;
	return code;
}

static u32
armPushFuncStart(s32 numVars)
{
	u32 regBits = 0;
	for (s32 x = 0; x < numVars; x++)
	{
		regBits <<= 1;
		regBits++;
	}
	regBits <<= 3;
	regBits += (1<<8);
	return armPush(regBits);
}

static u32
armPopFuncEnd(s32 numVars)
{
	u32 regBits = 0;
	for (s32 x = 0; x < numVars; x++)
	{
		regBits <<= 1;
		regBits++;
	}
	regBits <<= 3;
	regBits += (1<<8);
	return armPop(regBits);
}

static u32
armMovImm(u32 dest, u32 val)
{
	u32 code = 0x2000;
	code += val + (dest << 8);
	return code;
}

//~ static u32
//~ armMov(u32 dest, u32 src)
//~ {
	//~ u32 code = 0x4600;
	//~ code += ((dest>>3)<<7) + ((dest<<29)>>29) + (src << 3);
	//~ return code;
//~ }

static u32
armMov(u32 dest, u32 src)
{
	u32 code = 0x0000;
	code += dest + (src << 3);
	return code;
}

static u32
armLdrOffset(u32 dest, u32 src, u32 offset)
{
	u32 code = 0x6800;
	code += dest + (src << 3) + (offset << 6);
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
armAddImm(u32 dest, u32 val)
{
	u32 code = 0x3000;
	code += val + (dest << 8);
	return code;
}

static u32
armSubImm(u32 dest, u32 val)
{
	u32 code = 0x3800;
	code += val + (dest << 8);
	return code;
}

static u32
armSub3(u32 dest, u32 arg1, u32 arg2)
{
	u32 code = 0x1A00;
	code += dest + (arg1 << 3) + (arg2 << 6);
	return code;
}

static u32
armLslsImm(u32 dest, u32 src, u32 val)
{
	u32 code = 0x0000;
	code += dest + (src << 3) + (val << 6);
	return code;
}

static u32
armLsls(u32 dest, u32 arg1)
{
	u32 code = 0x4080;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armLsrsImm(u32 dest, u32 src, u32 val)
{
	u32 code = 0x0800;
	code += dest + (src << 3) + (val << 6);
	return code;
}

static u32
armLsrs(u32 dest, u32 arg1)
{
	u32 code = 0x40C0;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armMul(u32 dest, u32 arg1)
{
	u32 code = 0x4340;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armAnd(u32 dest, u32 arg1)
{
	u32 code = 0x4000;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armOr(u32 dest, u32 arg1)
{
	u32 code = 0x4300;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armXor(u32 dest, u32 arg1)
{
	u32 code = 0x4040;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armNot(u32 dest, u32 arg1)
{
	u32 code = 0x43C0;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armBic(u32 dest, u32 arg1)
{
	u32 code = 0x4380;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armNeg(u32 dest, u32 arg1)
{
	u32 code = 0x4240;
	code += dest + (arg1 << 3);
	return code;
}

static u32
armBX(u32 reg)
{
	u32 code = 0x4700;
	code += reg << 3;
	return code;
}

// Section Logical Instruction

/*e*/static void
pushTos(void)/*i;*/
{
	u32 code = 0xC101;
	putMachineCode(code);
}

/*e*/static void
popTos(void)/*i;*/
{
	decrementESP();
	putMachineCode(armLdrOffset(0, 1, 0));
}

/*e*/static void
popScratch(void)/*i;*/
{
	decrementESP();
	putMachineCode(armLdrOffset(2, 1, 0));
}

/*e*/static void
popLocal(u32 local)/*i;*/
{
	decrementESP();
	putMachineCode(armLdrOffset(local+3, 1, 0));
}

/*e*/static void
negTos(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 1;
		u32 src = prevCode>>3;
		putMachineCode(armNeg(0,src));
		return;
	}
	putMachineCode(armNeg(0, 0));
}

/*e*/static void
pushVal(s32 val)/*i;*/
{
	if (val <= 255 && val>=0)
	{
		// small value
		pushTos();
		putMachineCode(armMovImm(0, val));
	} else if (val < 0 && val >= -255) {
		pushTos();
		putMachineCode(armMovImm(0, -val));
		negTos();
	}
}

/*e*/static void
putMachineCode(u32 code)/*i;*/
{
	if (f.codeIndex >= f.codeBuffSize)
	{
		f.codeBuffSize *= 2;
		f.codeBuff = realloc(f.codeBuff, f.codeBuffSize*2);
	}
	f.codeBuff[f.codeIndex++] = code;
}

/*e*/static void
decrementESP(void)/*i;*/
{
	putMachineCode(armSubImm(1, 4));
}

/*e*/static void
stackAdd(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( (prevCode>>11) == 4)
	{
		// we just pushed a small constant, re-write
		f.codeIndex -= 2;
		u32 val = (prevCode<<24)>>24;
		// TODO can optimize val <= 7 and local to add with load
		putMachineCode(armAddImm(0, val));
		return;
	}
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armAdd3(0,0,src));
		return;
	}
	popScratch();
	putMachineCode(armAdd3(0,0,2));
}

/*e*/static void
stackSub(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( (prevCode>>11) == 4)
	{
		// we just pushed a small constant, re-write
		f.codeIndex -= 2;
		u32 val = (prevCode<<24)>>24;
		// TODO can optimize val <= 7 and local to add with load
		putMachineCode(armSubImm(0, val));
		return;
	}
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armSub3(0,0,src));
		return;
	}
	popScratch();
	putMachineCode(armSub3(0,2,0));
}

/*e*/static void
stackLS(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( (prevCode>>11) == 4)
	{
		// we just pushed a small constant, re-write
		f.codeIndex -= 2;
		u32 val = (prevCode<<24)>>24;
		putMachineCode(armLslsImm(0, 0, val));
		return;
	}
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armLsls(0,src));
		return;
	}
	putMachineCode(armMov(2, 0));
	popTos();
	putMachineCode(armLsls(0,2));
}

/*e*/static void
stackRS(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( (prevCode>>11) == 4)
	{
		// we just pushed a small constant, re-write
		f.codeIndex -= 2;
		u32 val = (prevCode<<24)>>24;
		putMachineCode(armLsrsImm(0, 0, val));
		return;
	}
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armLsrs(0,src));
		return;
	}
	putMachineCode(armMov(2, 0));
	popTos();
	putMachineCode(armLsrs(0,2));
}

/*e*/static void
stackMul(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armMul(0,src));
		return;
	}
	popScratch();
	putMachineCode(armMul(0, 2));
}

/*e*/static void
stackAnd(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armAnd(0,src));
		return;
	}
	popScratch();
	putMachineCode(armAnd(0, 2));
}

/*e*/static void
stackOr(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armOr(0,src));
		return;
	}
	popScratch();
	putMachineCode(armOr(0, 2));
}

/*e*/static void
stackXor(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armXor(0,src));
		return;
	}
	popScratch();
	putMachineCode(armXor(0, 2));
}

/*e*/static void
stackNot(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 1;
		u32 src = prevCode>>3;
		putMachineCode(armNot(0,src));
		return;
	}
	putMachineCode(armNot(0, 0));
}

/*e*/static void
stackCallWord(u32 target)/*i;*/
{
	Work *new = zalloc(sizeof(Work));
	new->workType = WORK_WORDCALL;
	new->codeIndex = f.codeIndex;
	putMachineCode(ARM_NOP); putMachineCode(ARM_NOP);
	new->target = target;
	f.workList = list_append(new, f.workList);
}

/*e*/static void
stackDiv(void)/*i;*/
{
	popScratch();
	stackCallWord((u32)fithDiv);
}

/*e*/static void
stackMod(void)/*i;*/
{
	popScratch();
	stackCallWord((u32)fithMod);
}

/*e*/static void
stackBitClear(void)/*i;*/
{
	u32 prevCode = f.codeBuff[f.codeIndex-1];
	if ( ((prevCode>>6) == 0x00) )
	{
		// we just pushed a local, re-write
		f.codeIndex -= 2;
		u32 src = prevCode>>3;
		putMachineCode(armBic(0,src));
		return;
	}
	putMachineCode(armMov(2, 0));
	popTos();
	putMachineCode(armBic(0, 2));
}

/*e*/static void
callWord(u32 target, u32 codeIndex)/*i;*/
{
	u32 currentAddr = (u32)&f.codeBuff[codeIndex];
	u16 *cursor = &f.codeBuff[codeIndex];
	u32 currentPC = currentAddr + 4;
	u32 jump = (target - currentPC) >> 1;
	u32 imm11 = jump << 21 >> 21;
	u32 imm10 = jump << 11 >> 22;
	u32 i2    = jump << 10 >> 31;
	u32 i1    = jump << 9 >> 31;
	u32 S     = jump << 8 >> 31;
	u32 j2    = (i2^1)^S;
	u32 j1    = (i1^1)^S;
	u32 code = 0xF000;
	code += (S<<10) + imm10;
	*cursor++ = code;
	code = 0xD000;
	code += (j2<<11) + (j1<<13) + imm11;
	*cursor = code;
}

// Section Word

/*e*/static void
createWord(u8 *word, s32 length)/*i;*/
{
	avlNode *node;
	node = avl_find(f.globals, word, length);
	if (node) { io_prints("Error: word has same name as global vairable.\n"); return; }
	if (f.insideWord) { io_prints("Error: word within a word.\n"); return; }
	node = avl_insert(&f.words, word, length, 0);
	if (node) { io_prints("Warning: redefinition of word.\n"); }
	node = avl_find(f.words, word, length);
	f.insideWord = 1;
	f.blocks[f.bi].savedIndex = (u32)node;
	f.blocks[f.bi++].blockType  = BLOCK_WORD;
}

/*e*/static void
setVar(u32 localNum)/*i;*/
{
	// initialize to TOS
	putMachineCode(armMov(localNum+3, 0));
	popTos();
}

/*e*/static u32
createVar(u8 *word, s32 length)/*i;*/
{
	avlNode *node;
	if (f.li >= 5) { io_prints("Error: Too many locals.\n"); return 5; }
	u32 localNum = f.li++;
	node = avl_insert(&f.locals, word, length, (void*)localNum);
	if (node) { io_prints("Warning: redefinition of local.\n"); }
	return localNum;
}

/*e*/static void
endBlock(void)/*i;*/
{
	if (f.bi == 0) { io_prints("Error: unmatched '}'.\n"); return; }
	f.bi--;
	switch (f.blocks[f.bi].blockType) {
		case BLOCK_WORD:
		// button up word and save it off
		;fithFunc exec = (fithFunc)((u32)completeWord());
		avlNode *node = (avlNode *)f.blocks[f.bi].savedIndex;
		node->value = exec;
		f.insideWord = 0;
		// reset code buffer for next usage
		f.codeBuff = zalloc(32);
		f.codeIndex = 0;
		f.codeBuffSize = 16;
		putMachineCode(ARM_NOP);
		break;
	}
}

/*e*/static void
endParams(void)/*i;*/
{
	if (f.li)
	{
		u32 stackOffset = f.li << 2;
		putMachineCode(armSubImm(1, stackOffset));
		stackOffset = stackOffset >> 2;
		u32 i;
		for (i = 1; i < stackOffset; i++)
		{
			putMachineCode(armLdrOffset(i+2, 1, i));
		}
		putMachineCode(armMov(i+2, 0));
		putMachineCode(armLdrOffset(0, 1, 0));
	}
	f.insideParams = 0;
}

/*e*/static void
speakWord(u8 *word, s32 length)/*i;*/
{
	avlNode *node;
	if (f.insideParams)
	{
		createVar(word, length);
		return;
	}
	node = avl_find(f.words, word, length);
	if (node)
	{
		stackCallWord((u32)node->value);
		f.notLeaf = 1;
	}
	node = avl_find(f.locals, word, length);
	if (node)
	{
		pushTos();
		putMachineCode(armMov(0, (u32)node->value + 3));
	}
}

/*e*/static u16*
completeWord(void)/*i;*/
{
	u32 index = f.codeIndex;
	putMachineCode(armPopFuncEnd(f.li));
	f.codeBuff[0] = armPushFuncStart(f.li);
	while (f.workList)
	{
		Work *todo = list_removeFirst(&f.workList);
		switch (todo->workType)
		{
			case WORK_WORDCALL:
			callWord(todo->target, todo->codeIndex);
			break;
		}
		free(todo);
	}
	u16 *start;
	if (f.li || f.notLeaf)
	{
		start = &f.codeBuff[0];
		io_printi(f.codeIndex << 1);
	} else {
		// overwrite ending
		f.codeBuff[index] = armBX(14);
		start = &f.codeBuff[1];
		io_printi((f.codeIndex-1) << 1);
	}
	// destroy all locals
	avl_freeAll(f.locals);
	f.locals = 0;
	f.li = 0;
	f.notLeaf = 0;
	io_prints(" size of word in bytes.\n");
	return start;
}
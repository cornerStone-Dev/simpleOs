// simple.c
#include "../localTypes.h"
// top level compile function
// input is source code text.
// output is address to main (needs lowest bit set) to be called or nothing
// system symbol table is updated with all public functions
// compiling with functions with the same name overwrites those symbol table
// entries.

#include "../gram.c"

/*e*/
void*
simpleCompile(u8 *sourceCode)/*p;*/
{
	Context   c;
	// in addition to the above there is a global persistent context for public
	// functions so that they may be used. They are pulled in through modules?
	TokenInfo t;
	u8 *cursor;
	c.error = 0;
	t.lineNumber = 1;
	cursor = sourceCode;
	void *pParser = ParseAlloc(zalloc, &c);
	do {
		cursor = lex(cursor, &t);
		if (t.tokenType >= 0) { Parse(pParser, t.tokenType, &t); }
	} while(t.tokenType != 0);
	ParseFree(pParser, free);
	return c.mainFunction;
}




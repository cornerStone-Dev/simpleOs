// shell.c

#include "../localTypes.h"

typedef struct ShellInfo {
	avlNode *programRoot;
} ShellInfo;

typedef void (*ShellProgram)(u8 *in);

static ShellInfo shell;

typedef struct {
	u8   *name;
	u32   length;
	void *function;
} ShellProgramInfo;

#define SHELL_INFO(A,B) {A, (sizeof A - 1), B},


enum {
	TERM_STATE_NONE,
	TERM_STATE_ESC1,
	TERM_STATE_ESC2,
};

void shell_inputLine(u8 *input)
{
	u32 length = 0;
	while ( (input[length] != ' ') && (input[length] != 0) )
	{
		length++;
	}
	avlNode *programNode =
	avl_find(
		shell.programRoot,     // pointer to tree
		input,      // pointer to string
		length   // length of string (255 max)
		);
	//~ io_printh(*(u32*)input);
	if (programNode == 0) { io_prints("No program found.\n"); return; }
	ShellProgram program = programNode->value;
	program(input);
}

/*e*/
void shell_echo(u8 *input)/*p;*/
{
	io_prints("\x1B[31m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[32m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[33m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[34m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[35m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[36m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[37m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[90m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[91m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[92m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[93m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[94m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[95m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[96m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[97m");
	io_prints(input + 5);
	io_prints("\n");
	io_prints("\x1B[0m");
}

void shell_cat(u8 *input)
{
	io_prints(input + 4);
	u32 i = 0;
	// skip white space
	while ( (input[i] < '0') || (input[i] > '9') )
	{
		if (input[i] == 0) { return; }
		i++;
	}
	// get block
	s32 blockNum = s2i(&input[i]);
	// calculate address
	u8* blockAddr = (u8*)(blockNum * 4096 + FLASH_BASE);
	// attempt to print out string at that address
	io_prints(blockAddr);
}

void shell_clear(void)
{
	io_prints("\x1B[2J");
	io_prints("\x1B[3J");
	io_prints("\x1B[0;0f");
}

void shell_fill(u8 *input)
{
	shell_clear();
	u8 string[4];
	string[0] = input[5];
	string[1] = 0;
	// fill row
	for (u32 i = 0; i < 80 * 24; i++)
	{
		io_prints(string);
	}
}

void shell_stringLen(u8 *input)
{
	io_printi(st_len(input));
	io_prints("\n");
}

/**
 * Wow I cannot update the whole screen every time as it is making me sick
 * I will have to be able to update singular locations or I am going to puke
 * I need to be able to update only a line at a time
 */


typedef struct EditInfo {
	u32  state;
	s32  currentLine;
	u32  cursor;
	u32  size;
	u32  initialized;
	u32  textSize;
	u32  rowStart;
	u32  prevDocRow;
	u32  docRow;
	u32  docColumn;
	u8  *text;
} EditInfo;

static EditInfo edit;



static void edit_deleteCharacter(void)
{
	if (edit.cursor == 0) { return; }
	edit.cursor--;
	edit.size--;
	//~ if (edit.docColumn == 1)
	//~ {
		//~ edit.docRow--;
		//~ edit.docColumn
	//~ } else {
		//~ edit.docColumn--;
	//~ }
	
	copyForward(&edit.text[edit.cursor+1],
		&edit.text[edit.cursor], edit.size - edit.cursor);
	return;
}

static void edit_insertCharacter(u32 input)
{
	edit.cursor++;
	edit.size++;
	//~ edit.docColumn++;
	if ((edit.size+1) >= edit.textSize)
	{
		edit.textSize *= 2;
		edit.text = realloc(edit.text, edit.textSize);
	}
	copyBackward(&edit.text[edit.cursor-1],
		&edit.text[edit.cursor], edit.size - edit.cursor);
	edit.text[edit.cursor-1] = input;
	return;
}

static u32 edit_getLineStart(u32 cursor)
{
	u32 start = cursor;
	while (1)
	{
		if (start == 0) { break; }
		if (edit.text[start-1] == '\n') { break; }
		start--;
	}
	return start;
}

static u32 edit_getPrintStart(u32 start, u32 *rowOut)
{
	u32 row   = 18;
	*rowOut = 18;
	// find start of current line
	while (1)
	{
		if (start == 0) { return start; }
		if (edit.text[start-1] == '\n') { break; }
		start--;
	}
	// start is start of current line to be displayed at row 18
	start--;
	while (1)
	{
		if (start == 0) { row--; break; }
		if (edit.text[start-1] == '\n') { row--; }
		if (row == 1) { break; }
		start--;
	}
	*rowOut = row;
	return start;
}

static u32 edit_getPrintEnd(u32 cursor)
{
	u32 end = cursor;
	u32 row   = 18;
	// find end
	while (1)
	{
		if (end >= edit.size) { break; }
		if (edit.text[end] == '\n') { row++; }
		if (row == 24) { break; }
		end++;
	}
	return end;
}

static void edit_upArrow(void)
{
	u32 start = edit_getLineStart(edit.cursor);
	if (start == 0) { return; }
	u32 column = edit.cursor - start + 1;
	//~ while (start < edit.cursor) { if (edit.text[start++] == 9) { column+=7; } }
	u32 upEnd = (start - 1);
	u32 upStart = edit_getLineStart(upEnd);
	u32 maxColumn = upEnd - upStart + 1;
	if (column >= maxColumn) {
		edit.cursor = upEnd;
	} else {
		edit.cursor = upEnd - (maxColumn - column);
	}
	// tabs screw things up unless they are lined up the same unfortunately
}

static void edit_downArrow(void)
{
	u32 start = edit_getLineStart(edit.cursor);
	u32 column = edit.cursor - start + 1;
	//~ while (start < edit.cursor) { if (edit.text[start++] == 9) { column+=7; } }
	u32 downStart = edit.cursor;
	// find end
	while (1)
	{
		if (downStart >= edit.size) { return; }
		if (edit.text[downStart] == '\n') { break; }
		downStart++;
	}
	downStart++;
	u32 downEnd = downStart;
	// find end
	while (1)
	{
		if (downEnd >= edit.size) { break; }
		if (edit.text[downEnd] == '\n') { break; }
		downEnd++;
	}
	u32 maxColumn = downEnd - downStart + 1;
	if (column >= maxColumn) {
		edit.cursor = downEnd;
	} else {
		edit.cursor = downEnd - (maxColumn - column);
	}
	// tabs screw things up unless they are lined up the same unfortunately
}

u32 screenCount;

static void edit_printScreen(void)
{
	// check if start of row is the same as last time
	u32 start = edit_getLineStart(edit.cursor);
	u32 column = edit.cursor - start + 1;
	u32 i = start;
	while (i < edit.cursor) { if (edit.text[i++] == 9) { column+=7; } }
	if (start == edit.rowStart)
	{
		// restart line
		io_prints("\x1B[2K");
		io_prints("\r");
		u32 end = edit.cursor;
		// find end
		while (1)
		{
			if (end >= edit.size) { break; }
			if (edit.text[end] == '\n') { break; }
			end++;
		}
		// non-destructive print
		u8 tmp = edit.text[end];
		edit.text[end] = 0;
		io_prints(&edit.text[start]);
		edit.text[end] = tmp;
		// set cursor
		io_prints(" \x08");
		io_prints("\x1B[18;");
		io_printi(column);
		io_prints("H");
		return;
	}
	// we have moved rows
	shell_clear(); // clear screen
	//~ io_prints("ns");
	//~ io_printi(screenCount++);
	edit.rowStart = start;
	// get starting row for cursor and print area
	u32 printCursorRow;
	u32 printStartCursor = edit_getPrintStart(start, &printCursorRow);
	// get end
	u32 printEndCursor = edit_getPrintEnd(edit.cursor);
	io_prints("cursorS");
	io_printi(printStartCursor);
	io_prints("cursorE");
	io_printi(printEndCursor);
	// move cursor correctly
	io_prints("\x1B[");
	io_printi(printCursorRow);
	io_prints(";1H");
	// null terminate view area
	u8 tmp = edit.text[printEndCursor];
	edit.text[printEndCursor] = 0; // null terminate
	// print from start of view area
	io_prints(&edit.text[printStartCursor]);
	edit.text[printEndCursor] = tmp; // fix up null termination
	// set cursor correctly
	io_prints(" \x08");
	io_prints("\x1B[18;");
	io_printi(column);
	io_prints("H");
	//~ u32 column = 1;
	//~ u32 row = 1;
	//~ u32 cursor = 0;
	//~ u32 count = edit.cursor;
	//~ while (count--)
	//~ {
		//~ column++;
		//~ if (edit.text[cursor++] == '\n')
		//~ {
			//~ column = 1;
			//~ row++; 
		//~ }
	//~ }
	//~ io_prints("\x1B[");
	//~ io_printi(edit.row);
	//~ io_printi(row);
	//~ io_prints(";");
	//~ io_printi(edit.column);
	//~ io_printi(column);
	//~ io_prints("H");
	return;
}

void shell_editLeave(void)
{
	shell_clear();
	io_setUartHandler(term_processCharacter);
	if (edit.text[edit.size-1] != '\n')
	{
		edit_insertCharacter('\n');
	}
	edit.text[edit.size] = 0;
	//~ simpleCompile(edit.text);
}

void shell_edit_processCharacter(u32 input)
{
	if (edit.state == TERM_STATE_NONE)
	{
		if (input == 0x13) // ^s
		{
			shell_editLeave();
			return;
		}
		if (input == 0x1B) // start of escape sequence
		{
			edit.state = TERM_STATE_ESC1;
			return;
		}
		if (input == 0x7F) // backspace input
		{
			edit_deleteCharacter();
			edit_printScreen();
			return;
		}
		if (input == 0x0D) // end of line
		{
			input = '\n';
			//~ edit.docRow++;
			//~ edit.docColumn = 0;
		}
		edit_insertCharacter(input);
		edit_printScreen();
		return;
	} else
	if (edit.state == TERM_STATE_ESC1)
	{
		if (input == 0x5B) // start of escape sequence
		{
			edit.state = TERM_STATE_ESC2;
			return;
		} else {
			edit.state = TERM_STATE_NONE;
			return;
		}
	} else
	//~ if (term.state == TERM_STATE_ESC2)
	{
		edit.state = TERM_STATE_NONE;
		if (input == 0x41) // up arrow
		{
			//~ clearLine();
			edit_upArrow();
			//~ rom_func.memcpy(&inputLine, &lines[edit.currentLine],  128);
			edit_printScreen();
			return;
		} else if (input == 0x42) { // down arrow
			//~ clearLine();
			//~ rom_func.memcpy(&inputLine, &lines[edit.currentLine],  128);
			edit_downArrow();
			edit_printScreen();
			return;
		} else if (input == 0x43) { // right arrow
			if (edit.cursor >= edit.size) { return; }
			edit.cursor++;
			//~ edit.docColumn++;
			edit_printScreen();
			//~ io_prints("\x1B\x5B\x43"); // echo arrow
			return;
		} else if (input == 0x44) { // left arrow
			if (edit.cursor == 0) { return; }
			edit.cursor--;
			edit_printScreen();
			//~ if (edit.docColumn == 1) { return; }
			//~ io_prints("\x08");
			//~ io_prints("\n left arrow\n");
			return;
		}
	}
}

void shell_edit_init(void)
{
	if (edit.initialized != 0) { edit.rowStart=-1;edit_printScreen(); return; }
	edit.text = zalloc(32);
	edit.textSize = 32;
	edit.docRow = 1;
	edit.prevDocRow = 1;
	edit.docColumn = 1;
	io_prints("\x1B[18;1H");
	edit.initialized = 1;
}

void shell_edit(u8 *input)
{
	shell_clear();
	shell_edit_init();
	io_setUartHandler(shell_edit_processCharacter);
}

void shell_persist(u8 *input)
{
	io_prints("Starting flash programming.\n");
	io_programFlash((void*)0x20000000, (u32)__bss_start__ - 0x20000000, 1);
	io_prints("RAM build written to flash.\n");
}

void shell_bootstrap(u8 *input)
{
	io_prints("Starting bootStrap programming.\n");
	os_programBootStrap(boot2Entry);
	io_prints("bootStrap written to flash.\n");
}

void printLit(u8 *input)
{
	io_prints("Loading inline lit = ");
	io_printh(asmLoadLit());
	io_prints("\n");
}

void shell_picoFith(u8 *input)
{
	fith_init();
	lineHandler = picoFith;
}

static ShellProgramInfo ShellPrograms [] = 
{
	SHELL_INFO("echo", shell_echo)
	SHELL_INFO("cat", shell_cat)
	SHELL_INFO("clear", shell_clear)
	SHELL_INFO("fill", shell_fill)
	SHELL_INFO("edit", shell_edit)
	SHELL_INFO("stringLen", shell_stringLen)
	SHELL_INFO("persist", shell_persist)
	SHELL_INFO("bootStrap", shell_bootstrap)
	SHELL_INFO("memStats", printMemStats)
	SHELL_INFO("reboot", REBOOT)
	SHELL_INFO("testp", testCompiler)
	SHELL_INFO("toggleLED", io_ledToggle)
	SHELL_INFO("lit", printLit)
	SHELL_INFO("picoFith", shell_picoFith)
};


void shell_init(void)
{	
	for (s32 i = 0; i < (sizeof ShellPrograms / sizeof ShellPrograms[0]); i++)
	{
		avl_insert(
		&shell.programRoot,   // pointer memory holding address of tree
		ShellPrograms[i].name,     // pointer to string
		ShellPrograms[i].length,  // length of string (255 max)
		ShellPrograms[i].function );   // value to be stored
	}
}

// terminal.c
#include "../localTypes.h"

#define TERM_NUM_LINES 16


enum {
	TERM_STATE_NONE,
	TERM_STATE_ESC1,
	TERM_STATE_ESC2,
};

typedef struct LineInfo {
	u32 cursor;
	u32 size;
	u8  line[120];
} LineInfo;

typedef struct TerminalInfo {
	u32 state;
	s32 currentLine;
} TerminalInfo;

static TerminalInfo term;
static LineInfo inputLine;
static LineInfo lines[TERM_NUM_LINES];

static void incrementCurrentLine(void)
{
	if ( (term.currentLine + 1) == TERM_NUM_LINES) {
		term.currentLine = 0;
	} else {
		term.currentLine++;
	}
}

static void decrementCurrentLine(void)
{
	if ( (term.currentLine - 1) == -1) {
		term.currentLine = TERM_NUM_LINES - 1;
	} else {
		term.currentLine--;
	}
}

static void clearLine(void)
{
	io_prints("\x1B[2K");
	return;
}

static void deleteCharacter(void)
{
	if (inputLine.cursor == 0) { return; }
	inputLine.cursor--;
	inputLine.size--;
	copyForward(&inputLine.line[inputLine.cursor+1],
		&inputLine.line[inputLine.cursor], inputLine.size - inputLine.cursor);
	return;
}

static void printTerminalLine(void)
{
	io_prints("\r>");
	inputLine.line[inputLine.size] = 0; // null terminate
	io_prints(inputLine.line);
	io_prints(" ");
	//~ io_prints("\x08");
	u32 moveCursorBack = inputLine.size - inputLine.cursor;
	do {
		io_prints("\x08");
	} while (moveCursorBack--);
	return;
}

static void insertCharacter(u32 input)
{
	inputLine.cursor++;
	inputLine.size++;
	copyBackward(&inputLine.line[inputLine.cursor-1],
		&inputLine.line[inputLine.cursor], inputLine.size - inputLine.cursor);
	inputLine.line[inputLine.cursor-1] = input;
	return;
}

/*e*/
void term_processCharacter(u32 input)/*p;*/
{
	if (term.state == TERM_STATE_NONE)
	{
		if (input == 0x1B) // start of escape sequence
		{
			term.state = TERM_STATE_ESC1;
			return;
		}
		if (input == 0x0D) // end of line
		{
			io_prints("\n"); // restart line
			if (inputLine.cursor == 0)
			{
				
				return;
			}
			// create process to handle input line
			inputLine.line[inputLine.size] = 0; // null terminate
			// save current line in to lines
			rom_func.memcpy(&lines[term.currentLine], &inputLine, 128);
			os_createProcess((*lineHandler), lines[term.currentLine].line, 0);
			//~ io_printh(*(u32*)lines[term.currentLine].line);
			incrementCurrentLine();
			setZeroWait();
			setZero(&lines[term.currentLine], 128);
			inputLine.cursor = 0;
			inputLine.size = 0;
			//~ os_createProcess(io_prints, "\n>", 0);
			return;
		}
		if (input == 0x7F) // backspace input
		{
			deleteCharacter();
			printTerminalLine();
			return;
		}
		insertCharacter(input);
		printTerminalLine();
		return;
	} else
	if (term.state == TERM_STATE_ESC1)
	{
		if (input == 0x5B) // start of escape sequence
		{
			term.state = TERM_STATE_ESC2;
			return;
		} else {
			term.state = TERM_STATE_NONE;
			return;
		}
	} else
	//~ if (term.state == TERM_STATE_ESC2)
	{
		term.state = TERM_STATE_NONE;
		if (input == 0x41) // up arrow
		{
			clearLine();
			decrementCurrentLine();
			rom_func.memcpy(&inputLine, &lines[term.currentLine],  128);
			printTerminalLine();
			return;
		} else if (input == 0x42) { // down arrow
			clearLine();
			rom_func.memcpy(&inputLine, &lines[term.currentLine],  128);
			incrementCurrentLine();
			printTerminalLine();
			return;
		} else if (input == 0x43) { // right arrow
			if (inputLine.cursor >= inputLine.size) { return; }
			inputLine.cursor++;
			io_prints("\x1B\x5B\x43"); // echo arrow
			return;
		} else if (input == 0x44) { // left arrow
			if (inputLine.cursor == 0) { return; }
			inputLine.cursor--;
			io_prints("\x08");
			return;
		}
	}
}

// gram.y

%include{
typedef unsigned int   size_t;
#define YYNOERRORRECOVERY 1
#define YYSTACKDEPTH 84
#define NDEBUG
#define assert(x) 
}

//~ %extra_context {CompilerContext *c}

%syntax_error {
	io_printi(yyminor->lineNumber);
	io_prints(": Syntax Error\n");
}

%token_type   {TokenInfo}
%default_type {NonTerminal}

//~ %default_destructor { (void)yypminor; (void)c; }

program ::= top_level_stmt_list.

top_level_stmt_list ::= top_level_stmt.
top_level_stmt_list ::= top_level_stmt_list top_level_stmt.

top_level_stmt ::= func_decl.
top_level_stmt ::= struct_decl.
top_level_stmt ::= stmt_var_decl.
//~ top_level_stmt ::= stmt_expr.

func_decl ::= func_ident parameter_decl compound_stmt.
{
	io_printi(c.regHighWater);
	io_prints(" Function parsed!\n");
	// fix up start of function
	c.codeBuff[0] = armPushFuncStart(c.regHighWater);
	c.codeBuff[1] = armSubSP(c.regHighWater - 4);
	// fix up return point
	c.codeBuff[c.returnIndex]   = armAddSP(c.regHighWater - 4);
	c.codeBuff[c.returnIndex+1] = armPopFuncEnd(c.regHighWater);
}

func_ident ::= F type(A) IDENT(B).
{
	// create function info struct
	FunctionInfo *f = zalloc(sizeof(FunctionInfo));
	// save off current function pointer into context
	c.currentFunc = f;
	// register function in global context
	avlNode *node = avl_insert(&c.globals, B.string, B.length, f);
	if (node) { io_prints("this function already exists!\n"); }
	// set return type
	f->returnType = A.lit.type;
	// reset reg numbers and symbols to zero in starting scope
	avl_destroy(&c.scopes[0].symbols);
	c.scopes[0].regNum = 0;
	c.regHighWater = 0;
	c.codeBuff = zalloc(32);
	c.codeBuffSize = 16;
	c.codeIndex = 2; // slots for start of the function
	c.returnIndex = 0;
	c.stackState = -1;
	//~ io_prints("func_ident parsed!\n");
}

struct_decl ::= STRUCT.

parameter_decl ::= LPAREN parameter_list RPAREN.
parameter_decl ::= LPAREN RPAREN.

parameter_list ::= var_decl(A).
{
	c.currentFunc->argType[0] = A.lit.type;
	localDecl(&A);
	A.lit.length = 1;
	putMachineCode(armMov(4, 0));
}
parameter_list ::= parameter_list(A) COMMA var_decl(B).
{
	localDecl(&B);
	putMachineCode(armMov(A.lit.length+4, A.lit.length));
	c.currentFunc->argType[A.lit.length++] = B.lit.type;
}


var_decl ::= type(A) IDENT(B).
{
	B.type = A.lit.type;
	A.tok = B;
}

type ::= type_name.
type ::= type_name(A) DOLLAR. { A.lit.type+=1; }
type ::= type_name(A) DOLLAR DOLLAR. { A.lit.type+=2; }

type_name ::= IDENT.
type_name ::= builtin_type.

//~ pointer ::= DOLLAR.
//~ pointer ::= DOLLAR DOLLAR.

builtin_type ::= U8(A). { A.type = T_U8; }
builtin_type ::= S32(A). { A.type = T_S32; }

compound_stmt ::= lblock RBLOCK. { leaveScope(); }
compound_stmt ::= lblock stmt_list RBLOCK. { leaveScope(); }

lblock ::= LBLOCK.
{
	enterScope();
}

stmt_list ::= stmt.
stmt_list ::= stmt_list stmt.

stmt ::= stmt_expr.
stmt ::= stmt_var_decl.
stmt ::= return_stmt.
stmt ::= if_stmt.

return_stmt ::= RETURN expr SEMI.
{
	simpleReturn();
}
return_stmt ::= RETURN SEMI.
{
	simpleReturn();
}

if_stmt ::= if_start compound_stmt.

if_start ::= IF expr.

stmt_expr ::= expr SEMI.

stmt_var_decl ::= var_decl ASSIGN expr SEMI.
stmt_var_decl ::= var_decl(A) SEMI.
{
	localDecl(&A);
}

expr ::= expr ASSIGN expr.
expr ::= expr(B) PLUS(C) expr(D).
{
	simple_binaryOp(&B, &D, C.type); 
}
expr ::= expr SUBT expr.
expr ::= expr GT expr.
expr ::= LPAREN expr RPAREN.
expr ::= and_expr expr. [AND]
expr ::= expr args.
{
	// code generation for function calls
	// expr is either a direct or indirect function call
	// if it is indirect we have address already generated at the bottom of the
	// stack and need to get it back to then jump. This is complicated
	// in the direct case the expr is an IDENT and needs to be looked up for
	// type information which we can work with. If we are in an expression
	// already we have stuff to do. Just realized memory access would need to be
	// deffered too when going through a struct....
}
expr ::= IDENT(A).
{
	if (yyLookahead == ASSIGN)
	{
		// do nothing as this is a store action
	} else {
		// look up var in locals
		avlNode *node = 0;
		s32 depth = c.scopeIndex;
		while (depth >= 0 && node == 0)
		{
			node = avl_find(c.scopes[depth--].symbols, A.string, A.length);
		}
		if (node != 0)
		{
			// we found the variable we are looking for as a local
			LocalVariable *v = node->value;
			A.type = v->type;
			pushVar(v->regNum);
			break;
		}
		node = avl_find(c.globals, A.string, A.length);
		if (node == 0) { io_prints("No symbol found.\n"); break; }
		// if this is a global symbol it is either a function or a global var
		// or a type? Several cases here that feed into function calls, casting,
		// or loading a global
		io_prints("Global Symbol Detected.\n");
	}
}
expr ::= STRING_LIT(A).
{
	// need a deffered code generation for a memory location
	// would use ADR instruction to memory beyond end of the function.
	// this cannot be known ahead of time and would need to be filled in after
	// the function and data have been generated.
	A.type = T_U8_R;
	// emit the 2 byte sequence for the string load
	// save off the current index to complete the fixup later
	// this would be part of a todo list to handle later?
	// interesting design I think it will fall out when ready.
	io_printsl(A.string, A.length);
	io_prints(": String parsed!\n");
}
expr ::= INTEGER(A).
{
	// modify type to expr realm
	pushVal(A.length);
}

args ::= args_start RPAREN.
{
	// no arguments in a function, push any state needed
}
args ::= args_start arg_list RPAREN.

args_start ::= LPAREN.

arg_list ::= expr.
{
	// first argument to a function, push state
	// the first argument must be in r0
	// exp can be deffered or not. Lots of cases here....
}
arg_list ::= arg_list COMMA expr.

and_expr ::= expr AND.

//~ %left RETURN.
%right ASSIGN.
%left  AND.
%nonassoc GT.
%left SUBT PLUS.
//~ %right TILDA.
//~ %left INTEGER IDENT S32.
%left LPAREN.

%token LAST.




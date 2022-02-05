// gram.y

%include{
typedef unsigned int   size_t;
#define YYNOERRORRECOVERY 1
#define YYSTACKDEPTH 84
#define NDEBUG
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
	io_prints("Function parsed!\n");
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
	f->returnType = A.lit.exprType;
	// reset reg numbers and symbols to zero in starting scope
	avl_destroy(&c.scopes[0].symbols);
	c.scopes[0].regNum = 0;
	c.codeBuff = zalloc(32);
	c.codeBuffSize = 16;
	c.codeIndex = 0;
	io_prints("func_ident parsed!\n");
}

struct_decl ::= STRUCT.

parameter_decl ::= LPAREN parameter_list RPAREN.
parameter_decl ::= LPAREN RPAREN.

parameter_list ::= var_decl(A).
{
	c.currentFunc->argType[0] = A.lit.exprType;
	localDecl(&A);
	A.lit.length = 1;
	
}
parameter_list ::= parameter_list(A) COMMA var_decl(B).
{
	localDecl(&B);
	c.currentFunc->argType[A.lit.length++] = B.lit.exprType;
}


var_decl ::= type(A) IDENT(B).
{
	B.tokenType = A.lit.exprType;
	A.tok = B;
}

type ::= type_name.
type ::= type_name(A) DOLLAR. { A.lit.exprType+=1; }
type ::= type_name(A) DOLLAR DOLLAR. { A.lit.exprType+=2; }

type_name ::= IDENT.
type_name ::= builtin_type.

//~ pointer ::= DOLLAR.
//~ pointer ::= DOLLAR DOLLAR.

builtin_type ::= U8(A). { A.tokenType = EXPR_U8; }
builtin_type ::= S32(A). { A.tokenType = EXPR_S32; }

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
return_stmt ::= RETURN SEMI.

if_stmt ::= if_start compound_stmt.

if_start ::= IF expr.

stmt_expr ::= expr SEMI.

stmt_var_decl ::= var_decl ASSIGN expr SEMI.
stmt_var_decl ::= var_decl SEMI.

expr ::= expr ASSIGN expr.
expr ::= expr(B) PLUS(C) expr(D).
{
	simple_binaryOp(&B, &D, C.tokenType); 
}
expr ::= expr(B) SUBT(C) expr(D).
{
	simple_binaryOp(&B, &D, C.tokenType); 
}
expr ::= expr GT expr.
expr ::= LPAREN expr RPAREN.
expr ::= and_expr expr. [AND]
expr ::= expr LPAREN RPAREN.
expr ::= expr LPAREN arg_list RPAREN.
expr ::= IDENT.
expr ::= STRING_LIT(A).
{
	// need a deffered code generation for a memory location
	// would use ADR instruction to memory beyond end of the function.
	// this cannot be known ahead of time and would need to be filled in after
	// the function and data have been generated.
	A.tokenType = EXPR_U8_R;
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
	//~ A.tokenType = EXPR_INTLIT;
	io_printi(A.length);
	io_prints(": Integer parsed!\n");
}

arg_list ::= expr.
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




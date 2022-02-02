// gram.y

%include{
typedef unsigned int   size_t;
#define YYNOERRORRECOVERY 1
#define YYSTACKDEPTH 84
#define NDEBUG
}

%extra_context {Context *c}

%syntax_error {
	io_printi(yyminor->lineNumber);
	io_prints(": Syntax Error\n");
}

%token_type   {TokenInfo}
%default_type {NonTerminal}

%default_destructor { (void)yypminor; (void)c; }

program ::= top_level_stmt_list.

top_level_stmt_list ::= top_level_stmt.
top_level_stmt_list ::= top_level_stmt_list top_level_stmt.

top_level_stmt ::= func_decl.
top_level_stmt ::= stmt_var_decl.
top_level_stmt ::= stmt_expr.

func_decl ::= func_ident LPAREN parameter_list RPAREN compound_stmt.

func_ident ::= F type IDENT.

parameter_list ::= var_decl.
parameter_list ::= parameter_list COMMA var_decl.

var_decl ::= type IDENT.


type ::= type_name.
type ::= type_name DOLLAR.
type ::= type_name DOLLAR DOLLAR.

type_name ::= IDENT.
type_name ::= builtin_type.

//~ pointer ::= DOLLAR.
//~ pointer ::= DOLLAR DOLLAR.

builtin_type ::= U8.
builtin_type ::= S32.

compound_stmt ::= LBLOCK RBLOCK.
compound_stmt ::= LBLOCK stmt_list RBLOCK.

stmt_list ::= stmt.
stmt_list ::= stmt_list stmt.

stmt ::= stmt_expr.
stmt ::= stmt_var_decl.
stmt ::= return_stmt.

return_stmt ::= RETURN expr SEMI.
return_stmt ::= RETURN SEMI.

stmt_expr ::= expr SEMI.

stmt_var_decl ::= var_decl ASSIGN expr SEMI.
stmt_var_decl ::= var_decl SEMI.

expr ::= expr PLUS expr.
expr ::= expr SUBT expr.
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
	io_printsl(A.string, A.length);
	io_prints(": String parsed!\n");
}
expr ::= INTEGER(A).
{
	io_printi(A.length);
	io_prints(": Integer parsed!\n");
}

arg_list ::= expr.
arg_list ::= arg_list COMMA expr.

and_expr ::= expr AND.

//~ %left RETURN.
//~ %right ASSIGN.
%left  AND.
//~ %nonassoc GT.
%left SUBT PLUS.
//~ %right TILDA.
//~ %left INTEGER IDENT S32.
%left LPAREN.




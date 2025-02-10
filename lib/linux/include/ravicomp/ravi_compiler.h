/******************************************************************************
 * Copyright (C) 2018-2024 Dibyendu Majumdar
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/
/*
A compiler for Ravi and Lua 5.3. This is work in progress.

This header file defines the public api
*/

#ifndef ravicomp_COMPILER_H
#define ravicomp_COMPILER_H

#include "ravicomp_export.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct CompilerState CompilerState;
typedef struct LexerState LexerState;
typedef struct LinearizerState LinearizerState;
typedef struct VariableType VariableType;

typedef long long lua_Integer;
typedef double lua_Number;

#ifndef C_MEMORYALLOCATOR_DEFINED
#define C_MEMORYALLOCATOR_DEFINED
/* We put all the allocator functions in one struct so that we only need to pass around
 * one pointer. Ideally the arena create/destroy functions should be separate.
 * The api below is based on the dlmalloc api.
 *
 * Note that this struct below is also defined in allocate.h/chibicc.h and all
 * definitions must be kept in sync.
 *
 * Note that this need not be threadsafe as all processing in the compiler
 * happens in a single thread; hence a simple single threaded allocator is good enough.
 */
typedef struct C_MemoryAllocator {
	void *arena;
	void *(*realloc)(void *arena, void *mem, size_t newsize);
	void *(*calloc)(void *arena, size_t n_elements, size_t elem_size);
	void (*free)(void *arena, void *p);
	void *(*create_arena)(size_t, int);
	size_t (*destroy_arena)(void *arena);
} C_MemoryAllocator;
#endif

/* Initialize the compiler state */
/* During compilation all data structures are stored in the compiler state */
RAVICOMP_EXPORT CompilerState *raviX_init_compiler(C_MemoryAllocator *allocator);
/* Destroy the compiler state, and free up all resources */
RAVICOMP_EXPORT void raviX_destroy_compiler(CompilerState *compiler);

/* ------------------------ LEXICAL ANALYZER API -------------------------------*/
/* Note: following enum was generated using utils/tokenenum.h                               */
enum TokenType {
	TOK_OFS = 256,

	TOK_and,
	TOK_break,
	TOK_do,
	TOK_else,
	TOK_elseif,
	TOK_end,
	TOK_false,
	TOK_for,
	TOK_function,
	TOK_goto,
	TOK_if,
	TOK_in,
	TOK_local,
	TOK_defer,
	TOK_C__decl,
	TOK_C__unsafe,
	TOK_C__new,
	TOK_nil,
	TOK_not,
	TOK_or,
	TOK_repeat,
	TOK_return,
	TOK_then,
	TOK_true,
	TOK_until,
	TOK_while,
	TOK_IDIV,
	TOK_CONCAT,
	TOK_DOTS,
	TOK_EQ,
	TOK_GE,
	TOK_LE,
	TOK_NE,
	TOK_SHL,
	TOK_SHR,
	TOK_DBCOLON,
	TOK_TO_INTEGER,
	TOK_TO_NUMBER,
	TOK_TO_INTARRAY,
	TOK_TO_NUMARRAY,
	TOK_TO_TABLE,
	TOK_TO_STRING,
	TOK_TO_CLOSURE,
	TOK_EOS,
	TOK_FLT,
	TOK_INT,
	TOK_NAME,
	TOK_STRING,

	FIRST_RESERVED = TOK_OFS + 1,
	LAST_RESERVED = TOK_while - TOK_OFS
};

/*
 * Lua strings can have embedded 0 bytes therefore we
 * need a string type that has a length associated with it.
 *
 * The compiler stores a single copy of each string so that StringObjects
 * can be compared by pointer equality.
 */
typedef struct StringObject {
	uint32_t len;	  /* length of the string */
	int32_t reserved; /* if is this a keyword then enum TokenType value else -1 */
	uint32_t hash;	  /* hash value of the string */
	const char *str;  /* string data, may not be 0 terminated */
} StringObject;

/*
 * Lua literals
 */
typedef union {
	lua_Number r;
	lua_Integer i;
	const StringObject *ts;
} SemInfo;

typedef struct Token {
	int token; /* Token value or character value; token values start from FIRST_RESERVED which is 257, values < 256
		      are ascii text characters */
	SemInfo seminfo; /* Literal associated with the token, only valid when token is a literal or an identifier, i.e.
			    token is > TOK_EOS */
} Token;

/*
 * Everything below should be treated as readonly; for efficiency these fields are exposed, however treat them
 * as fields managed by the lexer.
 */
typedef struct {
	int current;	 /* current character (char value as int) */
	int linenumber;	 /* current input line counter */
	int lastline;	 /* line number of the last token 'consumed' */
	Token t;	 /* current token, set after call to raviX_next() */
	Token lookahead; /* look ahead token, set after call to raviX_lookahead() */
} LexerInfo;

/* Following is a dynamic buffer implementation that is not strictly part of the
 * compiler api but is relied upon by various compiler parts. We should perhaps avoid
 * exposing it.
 *
 * The reason for exposing this is that we use it for getting the token string in one of the
 * api calls.
 */
typedef struct {
	char *buf;	 /* pointer to allocated memory, can be reallocated */
	size_t capacity; /* allocated size */
	size_t pos;	 /* current position in the buffer */
} TextBuffer;

/* all strings are interned and stored in a hash set, strings may have embedded
 * 0 bytes therefore explicit length is necessary. This function will create a new string
 * object if one doesn't already exist, else it will return the existing string object that
 * matches the input string.
 */
RAVICOMP_EXPORT const StringObject *raviX_create_string(CompilerState *compiler_state, const char *s, uint32_t len);

/* Initialize lexical analyser. Takes as input a buffer containing Lua/Ravi source and the source name. */
RAVICOMP_EXPORT LexerState *raviX_init_lexer(CompilerState *compiler_state, const char *buf, size_t buflen,
					     const char *source_name);
/* Gets the public part of the lexer data structure to allow access the current token. Note that the returned
 * value should be treated as readonly data structure.
 */
RAVICOMP_EXPORT const LexerInfo *raviX_get_lexer_info(LexerState *ls);
/* Retrieves the next token and saves it is LexState structure. If a lookahead was set then that is retrieved
 * (and reset to EOS) else the next token is retrieved.
 * LexerInfo->t gives you the current token, including any literal value.
 */
RAVICOMP_EXPORT void raviX_next(LexerState *ls);
/* Retrieves the next token and sets it as the lookahead. This means that a next call will get the lookahead.
 * Returns the token id which is either the text ascii code (if below 257) or else the enum TokenType.
 */
RAVICOMP_EXPORT int raviX_lookahead(LexerState *ls);
/* Convert a token to text format. The token will be written to current position in mb. */
RAVICOMP_EXPORT void raviX_token2str(int token, TextBuffer *mb);
/* Release all data structures used by the lexer */
RAVICOMP_EXPORT void raviX_destroy_lexer(LexerState *);

/* ---------------- PARSER API -------------------------- */

/*
 * Parse a Lua chunk (i.e. script).
 * The Lua chunk will be wrapped in an anonymous Lua function (the 'main' function), so all the code
 * in the chunk will be part of that function. Any functions defined in the chunk will become child functions
 * of the 'main' function.
 *
 * Each Lua chunk / script therefore has an anonymous 'main' function. The name 'main' is just to refer
 * to this function as it has no name in reality.
 *
 * Note that at present a new compiler state should be created when processing a Lua chunk.
 *
 * Returns 0 on success, non-zero on failure.
 */
RAVICOMP_EXPORT int raviX_parse(CompilerState *compiler_state, const char *buffer, size_t buflen,
				const char *source_name);
/* Prints out the AST to the file */
RAVICOMP_EXPORT void raviX_output_ast(CompilerState *compiler_state, FILE *fp);
void raviX_dump_ast(CompilerState *compiler_state, FILE *fp);
/* Performs type checks on the AST and annotates types of expressions nad variables where possible.
 * As a result the AST will be modified.
 *
 * Returns 0 on success, non-zero on failure.
 */
RAVICOMP_EXPORT int raviX_ast_typecheck(CompilerState *compiler_state);

/* ---------------------------- LINEARIZER API --------------------------------------- */
/* linear IR generator.
 * The goal of this component is to convert the AST to a linear IR.
 * This is work in progress, therefore the IR is not yet publicly exposed.
 */
RAVICOMP_EXPORT LinearizerState *raviX_init_linearizer(CompilerState *compiler_state);
/* Attempts to create linear IR for given AST.
 * Returns 0 on success.
 */
RAVICOMP_EXPORT int raviX_ast_linearize(LinearizerState *linearizer);

/*
 * Attempts to replace use of upvalues with constants where the upvalue
 * points to a local that was initialized with a literal and the local
 * has no other assignments.
 */
RAVICOMP_EXPORT void raviX_optimize_upvalues(LinearizerState *linearizer);

/* Prints out the content of the linear IR */
RAVICOMP_EXPORT void raviX_output_linearizer(LinearizerState *linearizer, FILE *fp);
/* Cleanup the linearizer */
RAVICOMP_EXPORT void raviX_destroy_linearizer(LinearizerState *linearizer);

/* utilies */
RAVICOMP_EXPORT const char *raviX_get_last_error(CompilerState *compiler_state);

/* ----------------------- AST WALKING API ------------------------ */

/* Binary operators */
typedef enum BinaryOperatorType {
	BINOPR_ADD,
	BINOPR_SUB,
	BINOPR_MUL,
	BINOPR_MOD,
	BINOPR_POW,
	BINOPR_DIV,
	BINOPR_IDIV,
	BINOPR_BAND,
	BINOPR_BOR,
	BINOPR_BXOR,
	BINOPR_SHL,
	BINOPR_SHR,
	BINOPR_CONCAT,
	BINOPR_EQ,
	BINOPR_LT,
	BINOPR_LE,
	BINOPR_NE,
	BINOPR_GT,
	BINOPR_GE,
	BINOPR_AND,
	BINOPR_OR,
	BINOPR_NOBINOPR
} BinaryOperatorType;

/* Gets string representation of the opcode */
RAVICOMP_EXPORT const char *raviX_get_binary_opr_str(BinaryOperatorType op);

/* Unary operators */
typedef enum UnaryOperatorType {
	UNOPR_MINUS = BINOPR_NOBINOPR + 1,
	UNOPR_BNOT,
	UNOPR_NOT,
	UNOPR_LEN,
	UNOPR_TO_INTEGER,
	UNOPR_TO_NUMBER,
	UNOPR_TO_INTARRAY,
	UNOPR_TO_NUMARRAY,
	UNOPR_TO_TABLE,
	UNOPR_TO_STRING,
	UNOPR_TO_CLOSURE,
	UNOPR_TO_TYPE,
	UNOPR_NOUNOPR
} UnaryOperatorType;

/* Gets string representation of the opcode */
RAVICOMP_EXPORT const char *raviX_get_unary_opr_str(UnaryOperatorType op);

/* Types of AST nodes created by the parser */
enum AstNodeType {
	AST_NONE, /* Will never be set on a properly initialized node */
	STMT_RETURN,
	STMT_GOTO,
	STMT_LABEL,
	STMT_DO,
	STMT_LOCAL,
	STMT_FUNCTION,
	STMT_IF,
	STMT_TEST_THEN,
	STMT_WHILE,
	STMT_FOR_IN,
	STMT_FOR_NUM,
	STMT_REPEAT,
	STMT_EXPR, /* Also used for assignment statements */
	STMT_EMBEDDED_C,
	EXPR_LITERAL,
	EXPR_SYMBOL,
	EXPR_Y_INDEX,		   /* [] operator */
	EXPR_FIELD_SELECTOR,	   /* table field access - '.' or ':' operator */
	EXPR_TABLE_ELEMENT_ASSIGN, /* table element assignment in table constructor */
	EXPR_SUFFIXED,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_FUNCTION,	    /* function literal */
	EXPR_TABLE_LITERAL, /* table constructor */
	EXPR_FUNCTION_CALL,
	EXPR_CONCAT,
	EXPR_BUILTIN /* Special Node for built-in functions */
};

typedef struct Statement Statement;
typedef struct ReturnStatement ReturnStatement;
typedef struct LabelStatement LabelStatement;
typedef struct GotoStatement GotoStatement;
typedef struct LocalStatement LocalStatement;
typedef struct ExpressionStatement ExpressionStatement;
typedef struct FunctionStatement FunctionStatement;
typedef struct DoStatement DoStatement;
typedef struct TestThenStatement TestThenStatement;
typedef struct IfStatement IfStatement;
typedef struct WhileOrRepeatStatement WhileOrRepeatStatement;
typedef struct ForStatement ForStatement;
typedef struct EmbeddedCStatement EmbeddedCStatement;

typedef struct Expression Expression;
typedef struct LiteralExpression LiteralExpression;
typedef struct SymbolExpression SymbolExpression;
typedef struct IndexExpression IndexExpression;
typedef struct UnaryExpression UnaryExpression;
typedef struct BinaryExpression BinaryExpression;
typedef struct FunctionExpression FunctionExpression;
typedef struct TableElementAssignmentExpression TableElementAssignmentExpression;
typedef struct TableLiteralExpression TableLiteralExpression;
typedef struct SuffixedExpression SuffixedExpression;
typedef struct FunctionCallExpression FunctionCallExpression;
typedef struct StringConcatenationExpression StringConcatenationExpression;
typedef struct BuiltinExpression BuiltinExpression;

typedef struct Scope Scope;

/* Types of symbols */
enum SymbolType {
	SYM_LOCAL,   /* LuaVariableSymbol */
	SYM_UPVALUE, /* LuaUpvalueSymbol */
	SYM_GLOBAL,  /* LuaVariableSymbol, Global symbols are never added to a scope so they are always looked up */
	SYM_LABEL,   /* LuaLabelSymbol */
	SYM_ENV	     /* Special symbol type for _ENV */
};
typedef struct LuaSymbol LuaSymbol;
typedef struct LuaUpvalueSymbol LuaUpvalueSymbol;
typedef struct LuaVariableSymbol LuaVariableSymbol;
typedef struct LuaLabelSymbol LuaLabelSymbol;

/* ----------------- Scope ------------------------ */
/* return the function that owns this scope */
RAVICOMP_EXPORT const FunctionExpression *raviX_scope_owning_function(const Scope *scope);
/* return the parent scope - NULL if no parent */
RAVICOMP_EXPORT const Scope *raviX_scope_parent_scope(const Scope *scope);
/* evaluate for each symbol defined in this scope */
RAVICOMP_EXPORT void raviX_scope_foreach_symbol(const Scope *scope, void *userdata,
						void (*callback)(void *userdata, const LuaSymbol *symbol));
/* does the scope contain up-values that need closing */
RAVICOMP_EXPORT bool raviX_scope_needs_closing(const Scope *scope);

/* ----------------- Symbols ------------------------ */

RAVICOMP_EXPORT enum SymbolType raviX_symbol_type(const LuaSymbol *symbol);
/* symbol downcast */
RAVICOMP_EXPORT const LuaVariableSymbol *raviX_symbol_variable(const LuaSymbol *symbol);
RAVICOMP_EXPORT const LuaUpvalueSymbol *raviX_symbol_upvalue(const LuaSymbol *symbol);
RAVICOMP_EXPORT const LuaLabelSymbol *raviX_symbol_label(const LuaSymbol *symbol);

/* variable symbol - local and global variables */
RAVICOMP_EXPORT const StringObject *raviX_variable_symbol_name(const LuaVariableSymbol *lua_local_symbol);
RAVICOMP_EXPORT const VariableType *raviX_variable_symbol_type(const LuaVariableSymbol *lua_local_symbol);
// NULL if global
RAVICOMP_EXPORT const Scope *raviX_variable_symbol_scope(const LuaVariableSymbol *lua_local_symbol);
RAVICOMP_EXPORT bool raviX_variable_symbol_is_function_parameter(const LuaVariableSymbol *lua_local_symbol);
RAVICOMP_EXPORT bool raviX_variable_symbol_has_escaped(const LuaVariableSymbol *lua_local_symbol);

/* label symbol */
RAVICOMP_EXPORT const StringObject *raviX_label_name(const LuaLabelSymbol *symbol);
RAVICOMP_EXPORT const Scope *raviX_label_scope(const LuaLabelSymbol *symbol);

/* upvalue symbol */
RAVICOMP_EXPORT const VariableType *raviX_upvalue_symbol_type(const LuaUpvalueSymbol *symbol);
RAVICOMP_EXPORT const LuaVariableSymbol *raviX_upvalue_target_variable(const LuaUpvalueSymbol *symbol);
RAVICOMP_EXPORT const FunctionExpression *raviX_upvalue_target_function(const LuaUpvalueSymbol *symbol);
RAVICOMP_EXPORT unsigned raviX_upvalue_index(const LuaUpvalueSymbol *symbol);

/* -------------------------- main ---------------------------- */

/* As described before each parsed Lua script or chunk is wrapped in an anonymous 'main'
 * function hence the AST root is this function.
 */
RAVICOMP_EXPORT const FunctionExpression *raviX_ast_get_main_function(const CompilerState *compiler_state);

/* -------------------------- Statements --------------------------- */

/* Convert a statement to the correct type */
RAVICOMP_EXPORT enum AstNodeType raviX_statement_type(const Statement *statement);
RAVICOMP_EXPORT const ReturnStatement *raviX_return_statement(const Statement *stmt);
RAVICOMP_EXPORT const LabelStatement *raviX_label_statement(const Statement *stmt);
RAVICOMP_EXPORT const GotoStatement *raviX_goto_statement(const Statement *stmt);
RAVICOMP_EXPORT const LocalStatement *raviX_local_statement(const Statement *stmt);
RAVICOMP_EXPORT const ExpressionStatement *raviX_expression_statement(const Statement *stmt);
RAVICOMP_EXPORT const FunctionStatement *raviX_function_statement(const Statement *stmt);
RAVICOMP_EXPORT const DoStatement *raviX_do_statement(const Statement *stmt);
RAVICOMP_EXPORT const TestThenStatement *raviX_test_then_statement(const Statement *stmt);
RAVICOMP_EXPORT const IfStatement *raviX_if_statement(const Statement *stmt);
RAVICOMP_EXPORT const WhileOrRepeatStatement *raviX_while_or_repeat_statement(const Statement *stmt);
RAVICOMP_EXPORT const ForStatement *raviX_for_statement(const Statement *stmt);
RAVICOMP_EXPORT const EmbeddedCStatement *raviX_embedded_C_statement(const Statement *stmt);

/* return statement walking */
RAVICOMP_EXPORT void raviX_return_statement_foreach_expression(const ReturnStatement *statement, void *userdata,
							       void (*callback)(void *, const Expression *expr));

/* label statement walking */
RAVICOMP_EXPORT const StringObject *raviX_label_statement_label_name(const LabelStatement *statement);
RAVICOMP_EXPORT const Scope *raviX_label_statement_label_scope(const LabelStatement *statement);

/* goto statement walking */
RAVICOMP_EXPORT const StringObject *raviX_goto_statement_label_name(const GotoStatement *statement);
RAVICOMP_EXPORT const Scope *raviX_goto_statement_scope(const GotoStatement *statement);
RAVICOMP_EXPORT bool raviX_goto_statement_is_break(const GotoStatement *statement);

/* local statement walking */
/* A local statement has a list of symbols and list of expressions */
RAVICOMP_EXPORT void raviX_local_statement_foreach_expression(const LocalStatement *statement, void *userdata,
							      void (*callback)(void *, const Expression *expr));
RAVICOMP_EXPORT void raviX_local_statement_foreach_symbol(const LocalStatement *statement, void *userdata,
							  void (*callback)(void *, const LuaVariableSymbol *expr));

/* expression or assignment statement walking */
/* here we have expressions to the left and right of equal sign */
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_lhs_expression(const ExpressionStatement *statement, void *userdata,
						  void (*callback)(void *, const Expression *expr));
RAVICOMP_EXPORT void
raviX_expression_statement_foreach_rhs_expression(const ExpressionStatement *statement, void *userdata,
						  void (*callback)(void *, const Expression *expr));

/* function statement walking */
RAVICOMP_EXPORT const SymbolExpression *raviX_function_statement_name(const FunctionStatement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_is_method(const FunctionStatement *statement);
RAVICOMP_EXPORT const IndexExpression *raviX_function_statement_method_name(const FunctionStatement *statement);
RAVICOMP_EXPORT bool raviX_function_statement_has_selectors(const FunctionStatement *statement);
RAVICOMP_EXPORT void raviX_function_statement_foreach_selector(const FunctionStatement *statement, void *userdata,
							       void (*callback)(void *, const IndexExpression *expr));
RAVICOMP_EXPORT const FunctionExpression *raviX_function_ast(const FunctionStatement *statement);

/* do statement walking */
RAVICOMP_EXPORT const Scope *raviX_do_statement_scope(const DoStatement *statement);
RAVICOMP_EXPORT void raviX_do_statement_foreach_statement(const DoStatement *statement, void *userdata,
							  void (*callback)(void *userdata, const Statement *statement));
/* if statement walking */
/* Lua if statements are a mix of select/case and if/else statements in
 * other languages. The AST represents the initial if condition block and all subsequent
 * elseif blocks as TestThenStatements. The final else block is treated as an optional
 * else block.
 */
RAVICOMP_EXPORT void raviX_if_statement_foreach_test_then_statement(const IfStatement *statement, void *userdata,
								    void (*callback)(void *,
										     const TestThenStatement *stmt));
RAVICOMP_EXPORT const Scope *raviX_if_then_statement_else_scope(const IfStatement *statement);
RAVICOMP_EXPORT void raviX_if_statement_foreach_else_statement(const IfStatement *statement, void *userdata,
							       void (*callback)(void *userdata,
										const Statement *statement));
RAVICOMP_EXPORT const Scope *raviX_test_then_statement_scope(const TestThenStatement *statement);
RAVICOMP_EXPORT void raviX_test_then_statement_foreach_statement(const TestThenStatement *statement, void *userdata,
								 void (*callback)(void *userdata,
										  const Statement *statement));
RAVICOMP_EXPORT const Expression *raviX_test_then_statement_condition(const TestThenStatement *statement);

/* while or repeat statement walking */
RAVICOMP_EXPORT const Expression *raviX_while_or_repeat_statement_condition(const WhileOrRepeatStatement *statement);
RAVICOMP_EXPORT const Scope *raviX_while_or_repeat_statement_scope(const WhileOrRepeatStatement *statement);
RAVICOMP_EXPORT void
raviX_while_or_repeat_statement_foreach_statement(const WhileOrRepeatStatement *statement, void *userdata,
						  void (*callback)(void *userdata, const Statement *statement));

/* for statement walking */
RAVICOMP_EXPORT const Scope *raviX_for_statement_scope(const ForStatement *statement);
RAVICOMP_EXPORT void raviX_for_statement_foreach_symbol(const ForStatement *statement, void *userdata,
							void (*callback)(void *, const LuaVariableSymbol *expr));
RAVICOMP_EXPORT void raviX_for_statement_foreach_expression(const ForStatement *statement, void *userdata,
							    void (*callback)(void *, const Expression *expr));
RAVICOMP_EXPORT const Scope *raviX_for_statement_body_scope(const ForStatement *statement);
RAVICOMP_EXPORT void raviX_for_statement_body_foreach_statement(const ForStatement *statement, void *userdata,
								void (*callback)(void *userdata,
										 const Statement *statement));

RAVICOMP_EXPORT void raviX_embedded_C_statement_foreach_symbol(const EmbeddedCStatement *statement, void *userdata,
							       void (*callback)(void *, const LuaVariableSymbol *expr));
RAVICOMP_EXPORT const StringObject *raviX_embedded_C_statement_C_source(const EmbeddedCStatement *statement);

/* -------------------------------- expressions ------------------------------- */

/* Convert an expression to the correct type */
RAVICOMP_EXPORT enum AstNodeType raviX_expression_type(const Expression *expression);
RAVICOMP_EXPORT const LiteralExpression *raviX_literal_expression(const Expression *expr);
RAVICOMP_EXPORT const SymbolExpression *raviX_symbol_expression(const Expression *expr);
RAVICOMP_EXPORT const IndexExpression *raviX_index_expression(const Expression *expr);
RAVICOMP_EXPORT const UnaryExpression *raviX_unary_expression(const Expression *expr);
RAVICOMP_EXPORT const BinaryExpression *raviX_binary_expression(const Expression *expr);
RAVICOMP_EXPORT const FunctionExpression *raviX_function_expression(const Expression *expr);
RAVICOMP_EXPORT const TableElementAssignmentExpression *
raviX_table_element_assignment_expression(const Expression *expr);
RAVICOMP_EXPORT const TableLiteralExpression *raviX_table_literal_expression(const Expression *expr);
RAVICOMP_EXPORT const SuffixedExpression *raviX_suffixed_expression(const Expression *expr);
RAVICOMP_EXPORT const FunctionCallExpression *raviX_function_call_expression(const Expression *expr);
RAVICOMP_EXPORT const StringConcatenationExpression *raviX_string_concatenation_expression(const Expression *expr);
RAVICOMP_EXPORT const BuiltinExpression *raviX_builtin_expression(const Expression *expr);

/* literal expression */
/* Note: '...' value has type RAVI_TVARARGS and no associated SemInfo. */
RAVICOMP_EXPORT const VariableType *raviX_literal_expression_type(const LiteralExpression *expression);
RAVICOMP_EXPORT const SemInfo *raviX_literal_expression_literal(const LiteralExpression *expression);

/* symbol expression */
RAVICOMP_EXPORT const VariableType *raviX_symbol_expression_type(const SymbolExpression *expression);
RAVICOMP_EXPORT const LuaSymbol *raviX_symbol_expression_symbol(const SymbolExpression *expression);

/* index expression */
RAVICOMP_EXPORT const VariableType *raviX_index_expression_type(const IndexExpression *expression);
RAVICOMP_EXPORT const Expression *raviX_index_expression_expression(const IndexExpression *expression);

/* unary expression */
RAVICOMP_EXPORT const VariableType *raviX_unary_expression_type(const UnaryExpression *expression);
RAVICOMP_EXPORT const Expression *raviX_unary_expression_expression(const UnaryExpression *expression);
RAVICOMP_EXPORT UnaryOperatorType raviX_unary_expression_operator(const UnaryExpression *expression);

/* binary expression */
RAVICOMP_EXPORT const VariableType *raviX_binary_expression_type(const BinaryExpression *expression);
RAVICOMP_EXPORT const Expression *raviX_binary_expression_left_expression(const BinaryExpression *expression);
RAVICOMP_EXPORT const Expression *raviX_binary_expression_right_expression(const BinaryExpression *expression);
RAVICOMP_EXPORT BinaryOperatorType raviX_binary_expression_operator(const BinaryExpression *expression);

/* function expression */
RAVICOMP_EXPORT const VariableType *raviX_function_type(const FunctionExpression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_vararg(const FunctionExpression *function_expression);
RAVICOMP_EXPORT bool raviX_function_is_method(const FunctionExpression *function_expression);
RAVICOMP_EXPORT const FunctionExpression *raviX_function_parent(const FunctionExpression *function_expression);
RAVICOMP_EXPORT void raviX_function_foreach_child(const FunctionExpression *function_expression, void *userdata,
						  void (*callback)(void *userdata,
								   const FunctionExpression *function_expression));
RAVICOMP_EXPORT const Scope *raviX_function_scope(const FunctionExpression *function_expression);
RAVICOMP_EXPORT void raviX_function_foreach_statement(const FunctionExpression *function_expression, void *userdata,
						      void (*callback)(void *userdata, const Statement *statement));
RAVICOMP_EXPORT void raviX_function_foreach_argument(const FunctionExpression *function_expression, void *userdata,
						     void (*callback)(void *userdata, const LuaVariableSymbol *symbol));
RAVICOMP_EXPORT void raviX_function_foreach_local(const FunctionExpression *function_expression, void *userdata,
						  void (*callback)(void *userdata,
								   const LuaVariableSymbol *lua_local_symbol));
RAVICOMP_EXPORT void raviX_function_foreach_upvalue(const FunctionExpression *function_expression, void *userdata,
						    void (*callback)(void *userdata, const LuaUpvalueSymbol *symbol));

/* table element assignment expression */
RAVICOMP_EXPORT const VariableType *
raviX_table_element_assignment_expression_type(const TableElementAssignmentExpression *expression);
RAVICOMP_EXPORT const Expression *
raviX_table_element_assignment_expression_key(const TableElementAssignmentExpression *expression);
RAVICOMP_EXPORT const Expression *
raviX_table_element_assignment_expression_value(const TableElementAssignmentExpression *expression);

/* table_literal_expression */
RAVICOMP_EXPORT const VariableType *raviX_table_literal_expression_type(const TableLiteralExpression *expression);
RAVICOMP_EXPORT void
raviX_table_literal_expression_foreach_element(const TableLiteralExpression *expression, void *userdata,
					       void (*callback)(void *, const TableElementAssignmentExpression *expr));

/* suffixed_expression */
RAVICOMP_EXPORT const VariableType *raviX_suffixed_expression_type(const SuffixedExpression *expression);
RAVICOMP_EXPORT const Expression *raviX_suffixed_expression_primary(const SuffixedExpression *expression);
RAVICOMP_EXPORT void raviX_suffixed_expression_foreach_suffix(const SuffixedExpression *expression, void *userdata,
							      void (*callback)(void *, const Expression *expr));

/* function call expression */
RAVICOMP_EXPORT const VariableType *raviX_function_call_expression_type(const FunctionCallExpression *expression);
// can return NULL
RAVICOMP_EXPORT const StringObject *
raviX_function_call_expression_method_name(const FunctionCallExpression *expression);
RAVICOMP_EXPORT void raviX_function_call_expression_foreach_argument(const FunctionCallExpression *expression,
								     void *userdata,
								     void (*callback)(void *, const Expression *expr));

/* string concatenation expression */
RAVICOMP_EXPORT void
raviX_string_concatenation_expression_foreach_argument(const StringConcatenationExpression *expression, void *userdata,
						       void (*callback)(void *, const Expression *expr));

/* builtin expression */
RAVICOMP_EXPORT int raviX_builtin_expression_token(const BuiltinExpression *expression);
RAVICOMP_EXPORT void raviX_builtin_expression_foreach_argument(const BuiltinExpression *expression, void *userdata,
							       void (*callback)(void *, const Expression *expr));

/* Utilities */
#ifdef __GNUC__
#define FORMAT_ATTR(pos) __attribute__((__format__(__printf__, pos, pos + 1)))
#else
#define FORMAT_ATTR(pos)
#endif

RAVICOMP_EXPORT void raviX_buffer_init(TextBuffer *mb, size_t initial_size);
RAVICOMP_EXPORT void raviX_buffer_resize(TextBuffer *mb, size_t new_size);
RAVICOMP_EXPORT void raviX_buffer_reserve(TextBuffer *mb, size_t n);
RAVICOMP_EXPORT void raviX_buffer_free(TextBuffer *mb);
static inline char *raviX_buffer_data(const TextBuffer *mb) { return mb->buf; }
static inline size_t raviX_buffer_size(const TextBuffer *mb) { return mb->capacity; }
static inline size_t raviX_buffer_len(const TextBuffer *mb) { return mb->pos; }
static inline void raviX_buffer_reset(TextBuffer *mb) { mb->pos = 0; }

/* following convert input to string before adding */
RAVICOMP_EXPORT void raviX_buffer_add_string(TextBuffer *mb, const char *str);
RAVICOMP_EXPORT void raviX_buffer_add_bytes(TextBuffer *mb, const char *str, size_t len);
RAVICOMP_EXPORT void raviX_buffer_add_fstring(TextBuffer *mb, const char *str, ...) FORMAT_ATTR(2);

/* strncpy() replacement with guaranteed 0 termination */
RAVICOMP_EXPORT void raviX_string_copy(char *buf, const char *src, size_t buflen);

#endif

#ifndef MY_COMPILER_H
#define MY_COMPILER_H

#define FOREACH_TOKENKIND(MACRO)                                               \
  MACRO(TOK_BRACE_OPEN)                                                        \
  MACRO(TOK_BRACE_CLOSE)                                                       \
  MACRO(TOK_PAREN_OPEN)                                                        \
  MACRO(TOK_PAREN_CLOSE)                                                       \
  MACRO(TOK_SEMICOLON)                                                         \
  MACRO(TOK_COLON)                                                             \
  MACRO(TOK_QUESTIONMARK)                                                      \
  MACRO(TOK_KEYWORD_FOR)                                                       \
  MACRO(TOK_KEYWORD_BREAK)                                                     \
  MACRO(TOK_KEYWORD_IF)                                                        \
  MACRO(TOK_KEYWORD_ELSE)                                                      \
  MACRO(TOK_KEYWORD_INT)                                                       \
  MACRO(TOK_KEYWORD_CHAR)                                                      \
  MACRO(TOK_KEYWORD_RETURN)                                                    \
  MACRO(TOK_IDENTIFIER)                                                        \
  MACRO(TOK_LITERAL_INT)                                                       \
  MACRO(TOK_MINUS)                                                             \
  MACRO(TOK_BITCOMPLEMENT)                                                     \
  MACRO(TOK_LOGICAL_NOT)                                                       \
  MACRO(TOK_PLUS)                                                              \
  MACRO(TOK_MUL)                                                               \
  MACRO(TOK_DIV)                                                               \
  MACRO(TOK_ASSIGN)                                                            \
  MACRO(TOK_LOGICAL_AND)                                                       \
  MACRO(TOK_LOGICAL_OR)                                                        \
  MACRO(TOK_LOGICAL_EQUAL)                                                     \
  MACRO(TOK_LOGICAL_NOT_EQUAL)                                                 \
  MACRO(TOK_LOGICAL_LESS)                                                      \
  MACRO(TOK_LOGICAL_LESS_EQUAL)                                                \
  MACRO(TOK_LOGICAL_GREATER)                                                   \
  MACRO(TOK_LOGICAL_GREATER_EQUAL)                                             \
  MACRO(TOK_UNKNOWN)                                                           \
  MACRO(TOK_EOF)

#define FOREACH_EXPR_KIND(MACRO)                                               \
  MACRO(EXPR_CONST)                                                            \
  MACRO(EXPR_UNOP)                                                             \
  MACRO(EXPR_BINOP)                                                            \
  MACRO(EXPR_ASSIGN)                                                           \
  MACRO(EXPR_VAR)

#define FOREACH_BLOCK_ITEM_KIND(MACRO)                                         \
  MACRO(BLOCK_ITEM_RETURN)                                                     \
  MACRO(BLOCK_ITEM_EXPR)                                                       \
  MACRO(BLOCK_ITEM_FORLOOP)                                                    \
  MACRO(BLOCK_ITEM_IFELSE)                                                     \
  MACRO(BLOCK_ITEM_COMPOUND)                                                   \
  MACRO(BLOCK_ITEM_DECL)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum { FOREACH_TOKENKIND(GENERATE_ENUM) } TokenKind;

#include <stdio.h>

typedef struct String {
  char *data;
  unsigned len;
  unsigned cap;
} String;

int string_create(String *buf, unsigned cap);
void string_destroy(String *buf);
int string_push(String *buf, char c);
unsigned string_size(String *buf);

typedef struct {
  FILE *file;
  char peek;
} CharPeekable;

typedef struct {
  unsigned start;
  unsigned len;
} StrSlice;

typedef struct {
  TokenKind kind;
  unsigned line;
  unsigned col;
  union {
    int      literalint;
    StrSlice string;
  };
} Token;


typedef struct {
  Token *data;
  unsigned len;
  unsigned cap;
  unsigned i;
} TokenPeekable;


typedef struct Lexer {
  TokenPeekable tokens;
  String charbuf;
} Lexer;


int tokenpeekable_expect(TokenPeekable *tokens, TokenKind expected,
                         const char *print);

int tokenpeekable_print(TokenPeekable *buf, String *str);

int  lex_init(Lexer *lexer);
void lex_quit(Lexer *lexer);
void lex_file(Lexer *lexer, FILE *file);

typedef struct VarMapEntry {
  int prev;
  StrSlice view;
  int stack_offset;
  int scopelevel;
} VarMapEntry;

typedef struct VarMap {
  VarMapEntry *data;
  int cap;
  int len;
} VarMap;

int  varmap_scope_size(VarMap *map, int startId);
int  varmap_init(VarMap *map);
void varmap_quit(VarMap *map);
int  varmap_new_id(VarMap *map);
VarMapEntry *varmap_get(VarMap *map, int id);
int  varmap_find_var(VarMap *map, int startId, StrSlice toFind, int varscope, String *charbuf);
void varmap_print(VarMap *vars, String *buffer);

int  kind_size(TokenKind tok);




Token peekn(Lexer *lexer, unsigned ahead);
Token peek(Lexer *lexer);
Token next(Lexer *lexer);

struct AstExpr;

typedef struct AstAssign {
  StrSlice name;
  struct AstExpr *expr;
} AstAssign;

typedef struct AstBinop {
  TokenKind kind;
  struct AstExpr *left;
  struct AstExpr *right;
} AstBinop;

typedef struct {
  TokenKind kind;
  struct AstExpr *expr;
} AstUnOp;

typedef struct AstExpr {
  enum { FOREACH_EXPR_KIND(GENERATE_ENUM) } kind;
  union {
    int intliteral;
    AstUnOp unop;
    AstBinop binop;
    AstAssign assign;
    StrSlice var;
  };
} AstExpr;

typedef struct AstDeclare {
  Token kind;
  StrSlice name;
  AstExpr *option;
} AstDeclare;

typedef struct AstIfElse {
  AstExpr *condition;
  struct AstBlockItem *truestmt;
  struct AstBlockItem *elsestmt;
} AstIfElse;

typedef struct {
  struct AstBlockItem *data;
  int len;
  int cap;
} AstBlockItemList;

typedef struct AstCompound {
  AstBlockItemList blockitems;
} AstCompound;

typedef struct AstForLoop {
  struct AstBlockItem *init;
  AstExpr *condition;
  AstExpr *postExpr;
  struct AstBlockItem *body;
} AstForLoop;

typedef struct AstBlockItem {
  enum { FOREACH_BLOCK_ITEM_KIND(GENERATE_ENUM) } kind;
  union {
    AstExpr *expr;
    AstDeclare decl;
    struct AstIfElse ifelse;
    AstCompound compound;
    AstForLoop forloop;
  };
} AstBlockItem;

typedef struct {
  StrSlice name;
  AstBlockItemList stmts;
} AstFunDef;

typedef struct {
  String charbuf;
  AstFunDef *fundef;
} Ast;

Ast *parse_file(FILE *file);

void gencode_file(FILE *file, Ast *ast, VarMap *vars, int lastVarId,
                  int scopelevel);

void print_expr(AstExpr *expr, String *buffer, int indent);
void print_fundef(AstFunDef *fundef, String *buffer, int indent);

void free_expr(AstExpr *expr);
void free_stmt(AstBlockItem *stmt);
void free_stmt_list(AstBlockItemList list);

#endif

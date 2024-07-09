
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
  MACRO(TOK_KEYWORD_STRUCT)                                                    \
  MACRO(TOK_KEYWORD_BREAK)                                                     \
  MACRO(TOK_KEYWORD_IF)                                                        \
  MACRO(TOK_KEYWORD_ELSE)                                                      \
  MACRO(TOK_KEYWORD_FOR)                                                       \
  MACRO(TOK_KEYWORD_INT)                                                       \
  MACRO(TOK_KEYWORD_CHAR)                                                      \
  MACRO(TOK_KEYWORD_RETURN)                                                    \
  MACRO(TOK_IDENTIFIER)                                                        \
  MACRO(TOK_TYPE_IDENTIFIER)                                                   \
  MACRO(TOK_NORMAL_IDENTIFIER)                                                 \
  MACRO(TOK_LITERAL_INT)                                                       \
  MACRO(TOK_MINUS)                                                             \
  MACRO(TOK_BITCOMPLEMENT)                                                     \
  MACRO(TOK_LOGICAL_NOT)                                                       \
  MACRO(TOK_PLUS)                                                              \
  MACRO(TOK_MUL)                                                               \
  MACRO(TOK_DIV)                                                               \
  MACRO(TOK_DOT)                                                               \
  MACRO(TOK_ASSIGN)                                                            \
  MACRO(TOK_SINGLE_AMPERSAND)                                                  \
  MACRO(TOK_LOGICAL_AND)                                                       \
  MACRO(TOK_LOGICAL_OR)                                                        \
  MACRO(TOK_LOGICAL_EQUAL)                                                     \
  MACRO(TOK_LOGICAL_NOT_EQUAL)                                                 \
  MACRO(TOK_LOGICAL_LESS)                                                      \
  MACRO(TOK_LOGICAL_LESS_EQUAL)                                                \
  MACRO(TOK_LOGICAL_GREATER)                                                   \
  MACRO(TOK_LOGICAL_GREATER_EQUAL)                                             \
  MACRO(TOK_COMMA)                                                             \
  MACRO(TOK_UNKNOWN)                                                           \
  MACRO(TOK_EOF)

#define FOREACH_EXPR_KIND(MACRO)                                               \
  MACRO(EXPR_CONST)                                                            \
  MACRO(EXPR_UNOP)                                                             \
  MACRO(EXPR_BINOP)                                                            \
  MACRO(EXPR_ASSIGN)                                                           \
  MACRO(EXPR_VAR)

#define FOREACH_BLOCK_ITEM_KIND(MACRO)          \
  MACRO(BLOCK_ITEM_RETURN)                      \
    MACRO(BLOCK_ITEM_EXPR)                      \
    MACRO(BLOCK_ITEM_FORLOOP)                   \
    MACRO(BLOCK_ITEM_IFELSE)                    \
    MACRO(BLOCK_ITEM_COMPOUND)                  \
    MACRO(BLOCK_ITEM_DECL)

#define FOREACH_AST_KIND(MACRO)                                                \
  MACRO(AST_LITERAL)                                                           \
  MACRO(AST_STRUCT)                                                            \
  MACRO(AST_DECL)                                                              \
  MACRO(AST_MEMBER_ACCESS)                                                     \
  MACRO(AST_VAR)                                                               \
  MACRO(AST_PROGRAM)                                                           \
  MACRO(AST_FUNC_PROTO)                                                        \
  MACRO(AST_FUNC_DEF)                                                          \
  MACRO(AST_FUNC_CALL)                                                         \
  MACRO(AST_BLOCK)                                                             \
  MACRO(AST_IF_ELSE)                                                           \
  MACRO(AST_RETURN)                                                            \
  MACRO(AST_BREAK)                                                             \
  MACRO(AST_BINOP)                                                             \
  MACRO(AST_UNOP)                                                              \
  MACRO(AST_FORLOOP)                                                           \
  MACRO(AST_ERROR)

#define FOREACH_TYPE_KIND(MACRO)                                               \
  MACRO(TYPE_UNRESOLVED)                                                       \
  MACRO(TYPE_ERROR)                                                            \
  MACRO(TYPE_INT)                                                              \
  MACRO(TYPE_STRUCT)                                                           \
  MACRO(TYPE_FUNC)                                                             \
  MACRO(TYPE_CHAR)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum { FOREACH_TOKENKIND(GENERATE_ENUM) } TokenKind;

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "str.h"
#include "lex.h"
#include "ast.h"
#include "table.h"
#include "dep.h"


typedef struct FileManager {
  String *filenames;
  String *content;
  unsigned len;
  unsigned cap;
} FileManager;

int    filemanager_init(FileManager *manager);
void   filemanager_quit(FileManager *manager);
int    filemanager_get_id(FileManager *manager, const String *filename);
int    filemanager_load_file(FileManager *manager, const String *filename);
String filemanager_get_content(FileManager *manager, int file_id);
String filemanager_get_filename(FileManager *manager, int file_id);



typedef struct Scope {
  struct Scope *parent;
  PtrBucket vars;
} Scope;

int   scope_init(Scope *scope);
int   scope_quit(Scope *scope);
void *scope_get(Scope *scope, uint64_t key);
int   scope_put(Scope *scope, uint64_t key, void *data);


typedef enum {
  // parameters
  // to functions
  RDI,
  RSI,
  RDX,
  RCX,
  R8,
  R9,
  // preserved
  RBX,
  RSP,
  RBP,
  R12,
  R13,
  R14,
  // return registers
  RAX,
} AssemblyRegisterType;

typedef struct AssemblyVarInfo {
  uint64_t name;
  unsigned size;
  bool isOnStack;
  bool isValid;
  union {
    AssemblyRegisterType registerType;
    int stackOffset;
  };
} AssemblyVarInfo;

typedef struct AssemblyVarTable {
  AssemblyVarInfo *data;
  int cap;
  int len;

} AssemblyVarTable;

int vartable_init(AssemblyVarTable *table);
int vartable_quit(AssemblyVarTable *table);
int vartable_checkpoint_get(AssemblyVarTable *table);
int vartable_checkpoint_set(AssemblyVarTable *table, int checkpoint);
int vartable_add(AssemblyVarTable *table, AssemblyVarInfo info);
AssemblyVarInfo vartable_get(AssemblyVarTable *table, uint64_t name);
AssemblyVarInfo vartable_last_on_stack(AssemblyVarTable *table);

typedef struct Parser {
  Lexer lexer;
  StringInterner pool;

  AstNode *root;

  PtrBucket struct_definitions;
  PtrBucket function_definitions;

  // needed to order to calculate
  // struct sizes out of order
  DepGraph struct_dependencies;

  // needed to resolve which variable name
  // relates to what variable declaration
  Scope scope;

  // maps names to register names
  // or stack value offsets for
  // simple assembly code generation
  AssemblyVarTable assembly_variables;

  bool hasErrors;

} Parser;

int parser_init(Parser *parser);
void parser_parse(Parser *parser, String content, int fileid,
                  FileManager *manager);
void parser_analyze(Parser *parser);
void parser_print(Parser *parser);
void parser_dump_assembly(Parser *parser, FILE *file);
void parser_quit(Parser *parser);

String parser_token_content(Parser *parser, Token tok);
String parser_token_filename(Parser *parser, Token tok);
int parser_analyze_types(Parser *parser);

int size_of_type(Parser *parser, Token kind);
int offset_align(int offset, int multiple);
int var_member_offset(Parser *parser, AstNode *node, int *last_member_size);

// Regular text
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define BWHT "\e[1;37m"

//Regular underline text
#define UBLK "\e[4;30m"
#define URED "\e[4;31m"
#define UGRN "\e[4;32m"
#define UYEL "\e[4;33m"
#define UBLU "\e[4;34m"
#define UMAG "\e[4;35m"
#define UCYN "\e[4;36m"
#define UWHT "\e[4;37m"

//Regular background
#define BLKB "\e[40m"
#define REDB "\e[41m"
#define GRNB "\e[42m"
#define YELB "\e[43m"
#define BLUB "\e[44m"
#define MAGB "\e[45m"
#define CYNB "\e[46m"
#define WHTB "\e[47m"

//High intensty background
#define BLKHB "\e[0;100m"
#define REDHB "\e[0;101m"
#define GRNHB "\e[0;102m"
#define YELHB "\e[0;103m"
#define BLUHB "\e[0;104m"
#define MAGHB "\e[0;105m"
#define CYNHB "\e[0;106m"
#define WHTHB "\e[0;107m"

//High intensty text
#define HBLK "\e[0;90m"
#define HRED "\e[0;91m"
#define HGRN "\e[0;92m"
#define HYEL "\e[0;93m"
#define HBLU "\e[0;94m"
#define HMAG "\e[0;95m"
#define HCYN "\e[0;96m"
#define HWHT "\e[0;97m"

//Bold high intensity text
#define BHBLK "\e[1;90m"
#define BHRED "\e[1;91m"
#define BHGRN "\e[1;92m"
#define BHYEL "\e[1;93m"
#define BHBLU "\e[1;94m"
#define BHMAG "\e[1;95m"
#define BHCYN "\e[1;96m"
#define BHWHT "\e[1;97m"

//Reset
#define reset "\e[0m"
#define CRESET "\e[0m"
#define COLOR_RESET "\e[0m"

#endif

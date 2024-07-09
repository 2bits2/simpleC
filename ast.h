#ifndef MY_AST_H
#define MY_AST_H


typedef enum ErrorKind {
  ERR_UNINITIALIZED_NODE,
  ERR_UNEXPECTED_TOKEN,
  ERR_INVALID_AST
} ErrorKind;

typedef struct UnexpectedTokenError {
  Token actual;
  const char *triedToParse;
} UnexpectedTokenError;

typedef struct InvalidAstError {
  const char *description;
  struct AstNode *node;
  Token context_token;
} InvalidAstError;

typedef struct Error {
  ErrorKind kind;
  union {
    UnexpectedTokenError unexpectedToken;
    InvalidAstError invalid;
  };
} Error;


typedef enum AstNodeKind {
  FOREACH_AST_KIND(GENERATE_ENUM)
} AstNodeKind;

typedef struct AstIfElse {
  struct AstNode *condition;
  struct AstNode *ifblock;
  struct AstNode *elseblock;
} AstIfElse;

typedef struct AstBreak {
} AstBreak;

typedef struct AstDecl {
  Token kind;
  Token name;
  int num_pointers;

  int size;
  int assembly_base_offset;
  struct AstNode *expr;
} AstDecl;

typedef struct AstBinOp {
  Token op;
  struct AstNode *left;
  struct AstNode *right;
} AstBinOp;

typedef struct AstUnOp {
  Token op;
  struct AstNode *expr;
} AstUnOp;

typedef struct AstForLoop {
  struct AstNode *init;
  struct AstNode *condition;
  struct AstNode *step;
  struct AstNode *stmt;
} AstForLoop;


typedef struct AstMemberAccess {
  Token name;
  struct AstNode *next;
} AstMemberAccess;

typedef struct AstVar {
  struct AstNode *declaration;
  Token name;
  struct AstNode *member_access;
} AstVar;

typedef struct AstNodeList {
  struct AstNode **nodes;
  int len;
  int cap;
} AstNodeList;


typedef struct AstFuncCall {
  Token name;
  AstNodeList params;
} AstFuncCall;

typedef struct AstNodeStruct {
  Token name;
  AstNodeList members;
  bool members_all_defined;
  int size;
} AstNodeStruct;

typedef struct AstFuncPrototype {
  Token retType;
  Token name;
  AstNodeList params;
} AstFuncPrototype;


typedef struct AstFuncDef {
  struct AstNode *prototype;
  struct AstNode *block;
} AstFuncDef;

typedef struct AstReturn {
  struct AstNode *expr;
} AstReturn;

typedef struct AstProgram {
  AstNodeList items;
} AstProgram;

typedef enum TypeKind {
  FOREACH_TYPE_KIND(GENERATE_ENUM)
} TypeKind;

typedef struct Type {
  TypeKind kind;
  struct AstNode *definition;
  int num_pointers;
  const char *error;
} Type;

typedef struct AstNode {
  AstNodeKind kind;
  Type *type;
  union {
    Token literal;
    AstVar var;
    AstDecl decl;
    Error error;
    AstNodeStruct structure;
    AstBinOp binop;
    AstUnOp  unop;
    AstFuncPrototype funcproto;
    AstNodeList block;
    AstFuncDef func;
    AstFuncCall funccall;
    AstProgram program;
    AstReturn ret;
    AstBreak brk;
    AstForLoop forloop;
    AstIfElse ifelse;
    AstMemberAccess member;
  };
} AstNode;


#endif

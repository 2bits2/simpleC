#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#define FOREACH_TOKENKIND(MACRO)                                               \
  MACRO(TOK_BRACE_OPEN)                                                        \
  MACRO(TOK_BRACE_CLOSE)                                                       \
  MACRO(TOK_PAREN_OPEN)                                                        \
  MACRO(TOK_PAREN_CLOSE)                                                       \
  MACRO(TOK_SEMICOLON)                                                         \
  MACRO(TOK_KEYWORD_INT)                                                       \
  MACRO(TOK_KEYWORD_RETURN)                                                    \
  MACRO(TOK_IDENTIFIER)                                                        \
  MACRO(TOK_LITERAL_INT)                                                       \
  MACRO(TOK_MINUS)                                                             \
  MACRO(TOK_BITCOMPLEMENT)                                                     \
  MACRO(TOK_LOGICAL_NOT)                                                       \
  MACRO(TOK_PLUS)                                                              \
  MACRO(TOK_MUL)                                                               \
  MACRO(TOK_DIV)                                                               \
  MACRO(TOK_UNKNOWN)                                                           \
  MACRO(TOK_EOF)

#define FOREACH_EXPR_KIND(MACRO)                                               \
  MACRO(EXPR_CONST)                                                            \
  MACRO(EXPR_UNOP)                                                             \
  MACRO(EXPR_BINOP)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum { FOREACH_TOKENKIND(GENERATE_ENUM) } TokenKind;

typedef struct {
  char *data;
  unsigned len;
  unsigned cap;
} CharBuffer;


int charbuffer_create(CharBuffer *buf, unsigned cap){
  buf->len = 0;
  buf->cap = cap;
  buf->data = malloc(sizeof(char) * buf->cap);
  if(!buf->data){
    return -1;
  }
  return 0;
}

void charbuffer_destroy(CharBuffer *buf){
  free(buf->data);
  buf->len = 0;
  buf->cap = 0;
}

int charbuffer_push(CharBuffer *buf, char c){
  if(buf->len >= buf->cap){
    unsigned newcap = buf->cap * 2;
    char *newdata = realloc(buf->data, newcap);
    if(!newdata){
      fprintf(stderr, "couldnt realloc charbuffer \n");
      return -1;
    }
    buf->data = newdata;
    buf->cap = newcap;
  }
  buf->data[buf->len++] = c;
  return 0;
}

unsigned charbuffer_pos(CharBuffer *buf){
  return buf->len;
}


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
} TokenBuffer;


int tokenbuffer_print(TokenBuffer *buf, CharBuffer *str){
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  for(int i=0; i<buf->len; i++){
    Token tok = buf->data[i];

    printf("%d:%d  %s  ", tok.line, tok.col, names[tok.kind]);

    if(tok.kind == TOK_IDENTIFIER){
      printf(" -> %.*s ", tok.string.len, str->data + tok.string.start);
    }
    printf("\n");
  }

  return 0;
}


int tokenbuffer_create(TokenBuffer *buf, unsigned cap){
  buf->i = 0;
  buf->len = 0;
  buf->cap = cap;
  buf->data = malloc(sizeof(Token) * buf->cap);
  if(!buf->data){
    return -1;
  }
  return 0;
}

Token tokenbuffer_peek(TokenBuffer *buf){
  if(buf->i >= buf->len){
    Token result = {
      .kind = TOK_EOF
    };
    return result;
  }
  return buf->data[buf->i];
}

Token tokenbuffer_next(TokenBuffer *buf){
  Token tok = tokenbuffer_peek(buf);
  if(tok.kind == TOK_EOF){
    return tok;
  }
  buf->i += 1;
  return tok;
}



void tokenbuffer_destroy(TokenBuffer *buf){
  free(buf->data);
  buf->cap = 0;
  buf->len = 0;
}

int tokenbuffer_push(TokenBuffer *buf, Token tok){
  if(buf->len >= buf->cap){
    unsigned newcap = buf->cap * 2;
    Token *newdata = realloc(buf->data, sizeof(Token) * newcap);
    if(!newdata){
      return -1;
    }
    buf->cap = newcap;
    buf->data = newdata;
  }

  buf->data[buf->len] = tok;
  buf->len++;
  return 0;
}


#define LEX_BUFFER_MAX 1024
typedef struct {
  char data[LEX_BUFFER_MAX];
  unsigned cap;
  unsigned i;
  FILE *file;
  char peek;
} LexBuffer;

int lexbuf_readnextchunk(LexBuffer *const lexbuf){
  lexbuf->i = 0;
  return lexbuf->cap = fread(lexbuf->data, sizeof(char), LEX_BUFFER_MAX, lexbuf->file);
}

int lexbuf_create(LexBuffer *const lexbuf, const char *filename){
  FILE *file = fopen(filename, "r");
  if(!file){
    fprintf(stderr, "couldnt open file %s for lexing \n", filename);
    return -1;
  }

  lexbuf->file = file;
  lexbuf->peek = 0;
  lexbuf_readnextchunk(lexbuf);
  return 0;
}


int lexbuf_destroy(LexBuffer *lexbuf){
  fclose(lexbuf->file);
  lexbuf->cap = 0;
  lexbuf->i = 0;
  lexbuf->peek = 0;
  return 0;
}


char lex_next(LexBuffer * const lexbuf){
  if(lexbuf->peek){
    char c = lexbuf->peek;
    lexbuf->peek = 0;
    return c;
  }

  if(lexbuf->i >= lexbuf->cap ){
    if(!lexbuf_readnextchunk(lexbuf)){
      return 0;
    }
    return lexbuf->data[lexbuf->i];
  }
  return lexbuf->data[lexbuf->i++];
}

char lex_peek(LexBuffer *const lexbuf){
  if(lexbuf->peek){
    return lexbuf->peek;
  }
  lexbuf->peek = lex_next(lexbuf);
  return lexbuf->peek;
}


void lex(TokenBuffer * const tokens, LexBuffer *lexer, CharBuffer *charbuf){

  Token tok = {.col = 0, .line=1};

  int lastlen = 0;

  while(1) {
    char c = lex_next(lexer);
    if(!c){break;}

    switch (c) {
    case '\n':
      tok.col = 0;
      tok.line++;
      lastlen = 0;
      continue;
    case ' ':
      tok.col++;
      lastlen = 0;
      continue;
    case '{':
      tok.kind = TOK_BRACE_OPEN;
      lastlen = 1;
      break;
    case '}':
      tok.kind = TOK_BRACE_CLOSE;
      lastlen = 1;
      break;
    case '+':
      tok.kind = TOK_PLUS;
      lastlen = 1;
      break;
    case '*':
      tok.kind = TOK_MUL;
      lastlen = 1;
      break;
    case '/':
      tok.kind = TOK_DIV;
      lastlen = 1;
      break;
    case '(':
      tok.kind = TOK_PAREN_OPEN;
      lastlen = 1;
      break;
    case ')':
      tok.kind = TOK_PAREN_CLOSE;
      lastlen = 1;
      break;
    case ';':
      tok.kind = TOK_SEMICOLON;
      lastlen = 1;
      break;
    case '!':
      tok.kind = TOK_LOGICAL_NOT;
      lastlen = 1;
      break;
    case '~':
      tok.kind = TOK_BITCOMPLEMENT;
      lastlen = 1;
      break;
    case '-':
      tok.kind = TOK_MINUS;
      lastlen = 1;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {

      int result = c - '0';
      int len = 1;
      while(1){
        char peek = lex_peek(lexer);
        if(!isdigit(peek)){
          break;
        }
        lex_next(lexer);
        len++;
        result *= 10;
        result += peek - '0';
      }

      tok.kind       = TOK_LITERAL_INT;
      tok.literalint = result;
      lastlen = len;
      break;
    }

    default:

      if(isalpha(c)){
        StrSlice slice = {};
        slice.start = charbuffer_pos(charbuf);
        slice.len = 1;
        charbuffer_push(charbuf, c);

        while(1){
          char peek = lex_peek(lexer);
          if(!isalnum(peek)){
            break;
          }
          lex_next(lexer);
          slice.len++;
          charbuffer_push(charbuf, peek);
        }


        tok.kind = TOK_IDENTIFIER;
        tok.string = slice;
        lastlen = slice.len;

        if(slice.len == 3 && memcmp(charbuf->data + slice.start, "int", 3) == 0){
          charbuf->len = slice.start;
          tok.kind = TOK_KEYWORD_INT;
        } else if(slice.len == 6 && memcmp(charbuf->data + slice.start, "return", 6) == 0){
          charbuf->len = slice.start;
          tok.kind = TOK_KEYWORD_RETURN;
        }



      } else {
        tok.kind = TOK_UNKNOWN;
        lastlen = 1;
      }
    }

    tokenbuffer_push(tokens, tok);
    tok.col += lastlen;
  }
}

struct AstExpr;

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
  enum {FOREACH_EXPR_KIND(GENERATE_ENUM)} kind;
  union {
    int intliteral;
    AstUnOp unop;
    AstBinop binop;
  };
} AstExpr;

typedef struct {
  AstExpr *expr;
} AstReturnStmt;

typedef struct {
  StrSlice name;
  AstReturnStmt *stmt;
} AstFunDef;

void print_expr(AstExpr *expr, CharBuffer *buffer, int indent) {
  if(expr == NULL){
    printf("%*sexpression NULL \n", indent, "");
    return;
  }
  static char *expr_names[] = {FOREACH_EXPR_KIND(GENERATE_STRING)};
  static char *tok_names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  switch (expr->kind) {
  case EXPR_CONST:
    printf("%*sconst %d \n", indent, "", expr->intliteral);
    break;
  case EXPR_BINOP:
    printf("%*sexpression %s \n", indent, "", tok_names[expr->binop.kind]);
    print_expr(expr->binop.left, buffer, indent+2);
    print_expr(expr->binop.right, buffer, indent+2);
    break;
  case EXPR_UNOP:
    printf("%*sexpression %s \n", indent, "", tok_names[expr->unop.kind]);
    print_expr(expr->unop.expr, buffer, indent+2);
    //printf("%*unop %d \n", indent, "", expr->intliteral);
    break;
  }
}

void print_fundef(AstFunDef *fundef, CharBuffer *buffer, int indent) {
  if(fundef == NULL){
    printf("%*sfundef NULL \n", indent, "");
    return;
  }
  printf("%*sfunction %.*s \n", indent, "", fundef->name.len, buffer->data + fundef->name.start);
  print_expr(fundef->stmt->expr, buffer, indent+2);
}


int tokenbuffer_expect(TokenBuffer *tokens, TokenKind expected, const char *print){
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};
  Token tok = tokenbuffer_peek(tokens);
  if(tok.kind != expected){
    if(print){
      fprintf(
          stderr,
          "%d:%d expected %s but found tokenkind %s "
          "instead.\n",
          tok.line, tok.col, print, names[tok.kind]);
    }
    return 0;
  }
  return 1;
}

AstExpr *parse_expr(TokenBuffer *tokens, CharBuffer *charbuf);

AstExpr *parse_factor(TokenBuffer *tokens, CharBuffer *charbuf) {
  Token tok = tokenbuffer_peek(tokens);
  switch (tok.kind) {
  case TOK_BITCOMPLEMENT:
  case TOK_LOGICAL_NOT:
  case TOK_MINUS: {
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_UNOP;
    expr->unop.kind = tok.kind;

    tokenbuffer_next(tokens);
    expr->unop.expr = parse_factor(tokens, charbuf);
    return expr;
  }
  case TOK_LITERAL_INT: {
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_CONST;
    expr->intliteral = tok.literalint;
    tokenbuffer_next(tokens);
    return expr;
    break;
  }
  case TOK_PAREN_OPEN: {
    tokenbuffer_next(tokens);
    AstExpr *expr = parse_expr(tokens, charbuf);
    if(expr == NULL){
      fprintf(stderr, "%d:%d expected expression after (", tok.line, tok.col);
      return NULL;
    }
    tok = tokenbuffer_next(tokens);
    if(tok.kind != TOK_PAREN_CLOSE){
      fprintf(stderr, "%d:%d expected closing paren ) after expression\n", tok.line, tok.col );
      // TODO: free expr
    }
    return expr;
    break;
  }

  default:
    return NULL;
  }
}

AstExpr *parse_term(TokenBuffer *tokens, CharBuffer *charbuf) {
  AstExpr *leftFactor = parse_factor(tokens, charbuf);
  if (leftFactor == NULL) {
    return NULL;
  }

  Token tok = tokenbuffer_peek(tokens);
  while (tok.kind == TOK_MUL || tok.kind == TOK_DIV) {
    tokenbuffer_next(tokens);
    AstExpr *rightFactor = parse_factor(tokens, charbuf);
    if (rightFactor == NULL) {
      fprintf(stderr, "%d:%d expected right factor after * or / \n", tok.line,
              tok.col);
      // TODO: free leftFactor
      return NULL;
    }

    AstExpr *factor = malloc(sizeof(AstExpr));
    factor->kind = EXPR_BINOP;
    factor->binop.kind = tok.kind;
    factor->binop.left = leftFactor;
    factor->binop.right = rightFactor;
    leftFactor = factor;

    tok = tokenbuffer_peek(tokens);
  }

  return leftFactor;
}

AstExpr *parse_expr(TokenBuffer *tokens, CharBuffer *charbuf) {
  AstExpr *leftTerm = parse_term(tokens, charbuf);
  if (leftTerm == NULL) {
    return leftTerm;
  }
  Token tok = tokenbuffer_peek(tokens);
  while (tok.kind == TOK_PLUS || tok.kind == TOK_MINUS) {
    tokenbuffer_next(tokens);

    AstExpr *rightTerm = parse_term(tokens, charbuf);
    if (rightTerm == NULL) {
      fprintf(stderr, "%d:%d expected right factor after * or / \n", tok.line,
              tok.col);
      // TODO: free rightTerm
      return NULL;
    }

    AstExpr *term = malloc(sizeof(AstExpr));
    term->kind = EXPR_BINOP;
    term->binop.kind = tok.kind;
    term->binop.left = leftTerm;
    term->binop.right = rightTerm;
    leftTerm = term;
    tok = tokenbuffer_peek(tokens);
  }
  return leftTerm;
}

AstReturnStmt *parse_return_stmt(TokenBuffer *tokens, CharBuffer *charbuf) {

  if (!tokenbuffer_expect(tokens, TOK_KEYWORD_RETURN, "return keyword")) {
    return NULL;
  }

  Token ret = tokenbuffer_peek(tokens);

  tokenbuffer_next(tokens);

  AstExpr *expr = parse_expr(tokens, charbuf);
  if (!expr) {
    fprintf(stderr, "%d:%d: expected valid expression after return keyword\n",
            ret.line, ret.col);
    return NULL;
  }

  if (!tokenbuffer_expect(tokens, TOK_SEMICOLON, "semicolon ; ")) {
    free(expr);
    return NULL;
  }
  tokenbuffer_next(tokens);
  //Token inttok = tokenbuffer_peek(tokens);

  AstReturnStmt *stmt = malloc(sizeof(AstReturnStmt));
  stmt->expr = expr;
  return stmt;
}

AstFunDef *parse_fun(TokenBuffer *tokens, CharBuffer *charbuf) {
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  if (!tokenbuffer_expect(tokens, TOK_KEYWORD_INT, "int keyword")) {
    return NULL;
  }
  tokenbuffer_next(tokens);

  if (!tokenbuffer_expect(tokens, TOK_IDENTIFIER, "identifier")) {
    return NULL;
  }
  Token identtok = tokenbuffer_peek(tokens);
  tokenbuffer_next(tokens);

  if (!tokenbuffer_expect(tokens, TOK_PAREN_OPEN, "open paren (")) {
    return NULL;
  }
  tokenbuffer_next(tokens);

  if (!tokenbuffer_expect(tokens, TOK_PAREN_CLOSE, "close paren )")) {
    return NULL;
  }
  tokenbuffer_next(tokens);

  if (!tokenbuffer_expect(tokens, TOK_BRACE_OPEN, "brace open {")) {
    return NULL;
  }
  tokenbuffer_next(tokens);

  AstReturnStmt *retstmt = parse_return_stmt(tokens, charbuf);
  if (!retstmt) {
    return NULL;
  }
  //tokenbuffer_next(tokens);

  if (!tokenbuffer_expect(tokens, TOK_BRACE_CLOSE, "brace close }")) {
    free(retstmt);
    return NULL;
  }

  AstFunDef *fundef = malloc(sizeof(AstFunDef));
  fundef->name = identtok.string;
  fundef->stmt = retstmt;
  return fundef;
}

void gencode_expr(FILE *file, AstExpr *expr, CharBuffer *buffer) {

  switch (expr->kind) {
  case EXPR_BINOP:
    gencode_expr(file, expr->binop.left, buffer);
    fprintf(file, "push %%eax\n");
    gencode_expr(file, expr->binop.right, buffer);
    fprintf(file, "mov %%eax, %%ecx\n");
    fprintf(file, "pop %%eax\n");

    switch (expr->binop.kind) {
    case TOK_PLUS:
      fprintf(file, "addl %%ecx, %%eax\n");
      break;
    case TOK_MINUS:
      fprintf(file, "subl %%ecx, %%eax\n");
      break;
    case TOK_MUL:
      fprintf(file, "imul %%ecx, %%eax\n");
      break;
    case TOK_DIV:
      fprintf(file, "cdq\n");
      fprintf(file, "idivl %%ecx, %%eax\n");
      break;
    default:
      fprintf(stderr, "couldnt generate code for expression \n");
      print_expr(expr, buffer, 0);
    }

    break;
  case EXPR_CONST:
    fprintf(file, "movl $%d, %%eax\n", expr->intliteral);
    break;
  case EXPR_UNOP:
    gencode_expr(file, expr->unop.expr, buffer);
    switch (expr->unop.kind) {
    case TOK_MINUS:
      fprintf(file, "neg %%eax\n");
      break;
    case TOK_BITCOMPLEMENT:
      fprintf(file, "not %%eax\n");
      break;
    case TOK_LOGICAL_NOT:
      fprintf(file, "cmpl $0, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "sete %%al\n");
      break;
    default:
      fprintf(stderr, "error cant generate code expr %d \n", expr->kind);
    }
  }
}

void gencode_retstmt(FILE *file, AstReturnStmt *retstmt, CharBuffer *buffer) {
  // fprintf(file, "movl $%d, %%eax\n", retstmt->intliteral);
  gencode_expr(file, retstmt->expr, buffer);
  fprintf(file, "ret\n");
}

void gencode_fundef(FILE *file, AstFunDef *fundef, CharBuffer *buffer) {
  fprintf(file, ".globl %.*s\n", fundef->name.len,
          buffer->data + fundef->name.start);
  fprintf(file, "%.*s:\n", fundef->name.len, buffer->data + fundef->name.start);
  gencode_retstmt(file, fundef->stmt, buffer);
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "wrong number of arguments. need a c file as input\n");
    exit(1);
  }

  const char *filename = argv[1];
  TokenBuffer tokens;
  CharBuffer string;
  LexBuffer lexbuf;
  int status = 0;

  status = tokenbuffer_create(&tokens, 100);
  if (status < 0) {
    fprintf(stderr, "couldnt allocate token buffer\n");
  }

  status = charbuffer_create(&string, 100);
  if (status < 0) {
    fprintf(stderr, "couldnt create char buffer\n");
  }

  status = lexbuf_create(&lexbuf, filename);
  if (status < 0) {
    fprintf(stderr, "couldnt lex filename %s\n", filename);
  }

  lex(&tokens, &lexbuf, &string);
  tokenbuffer_print(&tokens, &string);

  AstFunDef *fundef = parse_fun(&tokens, &string);

  print_fundef(fundef, &string, 0);

  FILE *genfile = fopen("myassembly.s", "w");
  if (fundef) {
    printf("succesfully parsed function \n");
    printf("name: %.*s \n", fundef->name.len, string.data + fundef->name.start);
    // printf("return value %d\n", fundef->stmt->expr.);
    gencode_fundef(genfile, fundef, &string);

    char commandbuffer[250];
    int len = 1;
    while (filename[len]) {
      if (filename[len] == '.') {
        break;
      }
      len++;
    }
    // system(" gcc -m32 myassembly.s -o program ");
  }

  fclose(genfile);

  lexbuf_destroy(&lexbuf);
  charbuffer_destroy(&string);
  tokenbuffer_destroy(&tokens);
}

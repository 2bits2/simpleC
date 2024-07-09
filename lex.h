#ifndef MY_LEXER_H
#define MY_LEXER_H

///////// lexing ////////////////
typedef struct {
  TokenKind kind;
  unsigned line;
  unsigned col;
  StrSlice string;
  int file_id;
} Token;

typedef struct {
  Token *data;
  unsigned len;
  unsigned cap;
} TokenArray;

typedef struct Lexer {
  TokenArray tokens;
  int i;
} Lexer;

int tokens_init(TokenArray *tokens);
int tokens_quit(TokenArray *tokens);
int tokens_add_from_string(TokenArray *tokens, String content, int fileid);
int tokens_internalize(TokenArray *tokens, String content,
                       StringInterner *interner);
Lexer tokens_to_lex(Lexer *lexer);

int   lex_init(Lexer *lexer);
int   lex_quit(Lexer *lexer);
Token lex_peekn(Lexer *lexer, unsigned ahead);
Token lex_peek(Lexer *lexer);
Token lex_next(Lexer *lexer);
int   lex(Lexer *lexer, String input, int fileid);

int   lex_checkpoint(Lexer *lexer);
int   lex_restore(Lexer *lexer, int checkpoint);


#endif

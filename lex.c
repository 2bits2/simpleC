#include "compiler.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int tokenpeekable_print(TokenPeekable *buf, String *str);
int tokenpeekable_create(TokenPeekable *buf, unsigned cap);
Token tokenpeekable_peek(TokenPeekable *buf);
Token tokenpeekable_next(TokenPeekable *buf);
void tokenpeekable_destroy(TokenPeekable *buf);
int tokenpeekable_push(TokenPeekable *buf, Token tok);

int charpeekable_create(CharPeekable *const lexbuf, const char *filename);
int charpeekable_destroy(CharPeekable *lexbuf);

int string_create(String *buf, unsigned cap){
  buf->len = 0;
  buf->cap = cap;
  buf->data = malloc(sizeof(char) * buf->cap);
  if(!buf->data){
    return -1;
  }
  return 0;
}

void string_destroy(String *buf){
  free(buf->data);
  buf->len = 0;
  buf->cap = 0;
}

int string_push(String *buf, char c){
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

unsigned string_size(String *buf){
  return buf->len;
}


int tokenpeekable_print(TokenPeekable *buf, String *str){
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


int tokenpeekable_create(TokenPeekable *buf, unsigned cap){
  buf->i = 0;
  buf->len = 0;
  buf->cap = cap;
  buf->data = malloc(sizeof(Token) * buf->cap);
  if(!buf->data){
    return -1;
  }
  return 0;
}

Token tokenpeekable_peekn(TokenPeekable *buf, unsigned i) {
  if (buf->i + i >= buf->len) {
    Token result = {.kind = TOK_EOF};
    return result;
  }
  return buf->data[buf->i + i];
}

Token tokenpeekable_peek(TokenPeekable *buf){
  if(buf->i >= buf->len){
    Token result = {
      .kind = TOK_EOF
    };
    return result;
  }
  return buf->data[buf->i];
}






Token tokenpeekable_next(TokenPeekable *buf){
  Token tok = tokenpeekable_peek(buf);
  if(tok.kind == TOK_EOF){
    return tok;
  }
  buf->i += 1;
  return tok;
}



void tokenpeekable_destroy(TokenPeekable *buf){
  free(buf->data);
  buf->cap = 0;
  buf->len = 0;
}

int tokenpeekable_push(TokenPeekable *buf, Token tok){
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

int charpeekable_create(CharPeekable *const charpeekable, const char *filename){
  FILE *file = fopen(filename, "r");
  if(!file){
    fprintf(stderr, "couldnt open file %s for lexing \n", filename);
    return -1;
  }

  charpeekable->file = file;
  charpeekable->peek = 0;
  return 0;
}


int charpeekable_destroy(CharPeekable *charpeekable){
  fclose(charpeekable->file);
  charpeekable->peek = 0;
  return 0;
}


char lex_next(CharPeekable * const charpeekable){
  if(charpeekable->peek){
    char c = charpeekable->peek;
    charpeekable->peek = 0;
    return c;
  }
  char c = fgetc(charpeekable->file);
  if(c == EOF){return 0;}
  return c;
}

char lex_peek(CharPeekable *const charpeekable){
  if(charpeekable->peek){
    return charpeekable->peek;
  }
  charpeekable->peek = lex_next(charpeekable);
  return charpeekable->peek;
}






void lex(Lexer *lexer, CharPeekable *charpeekable){

  Token tok = {.col = 0, .line=1};

  int lastlen = 0;

  while (1) {
    char c = lex_next(charpeekable);
    if (!c) {
      break;
    }

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
    case '=': {
      tok.kind = TOK_ASSIGN;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if (peek == '=') {
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_EQUAL;
        lastlen++;
      }
      break;
    }
    case '<': {
      tok.kind = TOK_LOGICAL_LESS;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if(peek == '='){
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_LESS_EQUAL;
        lastlen++;
      }
      break;
    }
    case '&': {
      tok.kind = TOK_UNKNOWN;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if (peek == '&') {
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_AND;
        lastlen++;
      }
      break;
    }
    case '|': {
      tok.kind = TOK_UNKNOWN;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if (peek == '|') {
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_OR;
        lastlen++;
      }
      break;
    }
    case '>': {
      tok.kind = TOK_LOGICAL_GREATER;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if(peek == '='){
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_GREATER_EQUAL;
        lastlen++;
      }
      break;
    }
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
    case ':':
      tok.kind = TOK_COLON;
      lastlen = 1;
      break;
    case '!': {
      tok.kind = TOK_LOGICAL_NOT;
      lastlen = 1;
      char peek = lex_peek(charpeekable);
      if(peek == '='){
        lex_next(charpeekable);
        tok.kind = TOK_LOGICAL_NOT_EQUAL;
        lastlen++;
      }
      break;
    }
    case '~':
      tok.kind = TOK_BITCOMPLEMENT;
      lastlen = 1;
      break;
    case '-':
      tok.kind = TOK_MINUS;
      lastlen = 1;
      break;
    case '?':
      tok.kind = TOK_QUESTIONMARK;
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
        char peek = lex_peek(charpeekable);
        if(!isdigit(peek)){
          break;
        }
        lex_next(charpeekable);
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
        slice.start = string_size(&lexer->charbuf);
        slice.len = 1;
        string_push(&lexer->charbuf, c);

        while(1){
          char peek = lex_peek(charpeekable);
          if(!isalnum(peek)){
            break;
          }
          lex_next(charpeekable);
          slice.len++;
          string_push(&lexer->charbuf, peek);
        }


        tok.kind = TOK_IDENTIFIER;
        tok.string = slice;
        lastlen = slice.len;

        if(slice.len == 3){
          if(memcmp(lexer->charbuf.data + slice.start, "int", 3) == 0){
            lexer->charbuf.len = slice.start;
            tok.kind = TOK_KEYWORD_INT;
          } else if(memcmp(lexer->charbuf.data + slice.start, "for", 3) == 0){
            lexer->charbuf.len = slice.start;
            tok.kind = TOK_KEYWORD_FOR;
          }
        }
        else if(slice.len == 4){
          if (memcmp(lexer->charbuf.data + slice.start, "char", 4) == 0) {
            lexer->charbuf.len = slice.start;
            tok.kind = TOK_KEYWORD_CHAR;
          }
        }
        else if(slice.len == 5){
          if(memcmp(lexer->charbuf.data + slice.start, "break", 5) == 0){
            lexer->charbuf.len = slice.start;
            tok.kind = TOK_KEYWORD_FOR;
          }
        }
        else if(slice.len == 6 && memcmp(lexer->charbuf.data + slice.start, "return", 6) == 0){
          lexer->charbuf.len = slice.start;
          tok.kind = TOK_KEYWORD_RETURN;
        } else if(slice.len == 2 && memcmp(lexer->charbuf.data + slice.start, "if", 2) == 0){
          lexer->charbuf.len = slice.start;
          tok.kind = TOK_KEYWORD_IF;
        } else if(slice.len == 4 && memcmp(lexer->charbuf.data + slice.start, "else", 4) == 0){
          lexer->charbuf.len = slice.start;
          tok.kind = TOK_KEYWORD_ELSE;
        }



      } else {
        tok.kind = TOK_UNKNOWN;
        lastlen = 1;
      }
    }

    tokenpeekable_push(&lexer->tokens, tok);
    tok.col += lastlen;
  }
}

Token peek(Lexer *lexer) {
  return tokenpeekable_peek(&lexer->tokens);
}


Token peekn(Lexer *lexer, unsigned i){
  return tokenpeekable_peekn(&lexer->tokens, i);
}

Token next(Lexer *lexer) { return tokenpeekable_next(&lexer->tokens); }


void lex_file(Lexer *lexer, FILE *file) {
  CharPeekable charpeekable = {.file = file, .peek = 0};
  lex(lexer, &charpeekable);
}

int lex_init(Lexer *lexer) {
  int status = 0;
  status = tokenpeekable_create(&lexer->tokens, 100);
  if (status < 0) {
    fprintf(stderr, "couldnt allocate token buffer\n");
    return status;
  }
  status = string_create(&lexer->charbuf, 100);
  if (status < 0) {
    fprintf(stderr, "couldnt create char buffer\n");
    return status;
  }
  return 0;
}

void lex_quit(Lexer *lexer){
  string_destroy(&lexer->charbuf);
  tokenpeekable_destroy(&lexer->tokens);
}

int tokenpeekable_expect(TokenPeekable *tokens, TokenKind expected, const char *print){
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};
  Token tok = tokenpeekable_peek(tokens);
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

#include "compiler.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////////// tokens //////////////////////////////////////
int tokens_init(TokenArray *arr) {
  arr->len = 0;
  arr->cap = 64;
  arr->data = malloc(sizeof(*arr->data) * arr->cap);
  if (!arr->data) {
    fprintf(stderr, "couldn't initialize token array with cap %d\n", arr->cap);
    return -1;
  }
  return 0;
}

String token_string(FileManager *manager, Token tok){
  String result = filemanager_get_content(manager, tok.file_id);
  if(result.data){
    result = str_from_slice(result, tok.string);
  }
  return result;
}

void token_print(FileManager *manager, Token tok, FILE *file){
  static const char *kind_names[] = { FOREACH_TOKENKIND(GENERATE_STRING)};
  String content = token_string(manager, tok);
  String filename = filemanager_get_filename(manager, tok.file_id);
  fprintf(file, "%.*s:%d:%d: %.*s ->  %s",
          filename.len, filename.data,
          tok.line, tok.col,
          content.len, content.data,
          kind_names[tok.kind]
          );
}


int tokens_print(TokenArray *buf, String str){
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};
  for(int i=0; i<buf->len; i++){
    Token tok = buf->data[i];
    printf("%d:%d  %s  ", tok.line, tok.col, names[tok.kind]);
    printf(" -> %.*s ", tok.string.len, str.data + tok.string.start);
    printf("\n");
  }

  return 0;
}

int tokens_internalize(TokenArray *tokens, String content,
                       StringInterner *interner){
  for(int i=0; i<tokens->len; i++){
    String original = str_from_slice(content, tokens->data[i].string);
    tokens->data[i].string = str_interner_put(interner, original);
  }
  return 0;
}

int tokens_push(TokenArray *buf, Token tok){
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

int tokens_quit(TokenArray *arr){
  free(arr->data);
  arr->cap = 0;
  arr->len = 0;
  return 0;
}

Lexer tokens_to_lexer(TokenArray arr){
  Lexer result =  {.tokens = arr, .i=0 };
  return result;
}


int tokens_add_from_string(TokenArray *tokens, String content, int fileid){
  Token tok = {.col = 0, .line = 1};
  int lastlinepos = 0;
  for (int i = 0; i < content.len; i++) {
    char c = content.data[i];
    switch (c) {
    case '\n':
      tok.line++;
      tok.col = 0;
      lastlinepos = i;
      continue;
    case ' ':
      tok.col++;
      continue;
    case '{':
      tok.kind = TOK_BRACE_OPEN;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;
    case '}':
      tok.kind = TOK_BRACE_CLOSE;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '.':
      tok.kind = TOK_DOT;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '(':
      tok.kind = TOK_PAREN_OPEN;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case ')':
      tok.kind = TOK_PAREN_CLOSE;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case ',':
      tok.kind = TOK_COMMA;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '&':
      tok.kind = TOK_SINGLE_AMPERSAND;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '/':
      tok.kind = TOK_DIV;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '-':
      tok.kind = TOK_MINUS;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '+':
      tok.kind = TOK_PLUS;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '*':
      tok.kind = TOK_MUL;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
      break;

    case '>':
      tok.kind = TOK_LOGICAL_GREATER;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;

      if(i+1 < content.len){
        if(content.data[i+1] == '='){
          tok.kind = TOK_LOGICAL_GREATER_EQUAL;
          tok.string.len++;
        }
      }
      tokens_push(tokens, tok);
      i += tok.string.len - 1;
      break;

    case '<':
      tok.kind = TOK_LOGICAL_LESS;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;

      if (i + 1 < content.len) {
        if (content.data[i + 1] == '=') {
          tok.kind = TOK_LOGICAL_LESS_EQUAL;
          tok.string.len = 2;
        }
      }
      tokens_push(tokens, tok);
      i += tok.string.len - 1;
      break;

    case '=':
      tok.kind = TOK_ASSIGN;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;

      if (i + 1 < content.len) {
        if (content.data[i + 1] == '=') {
          tok.kind = TOK_LOGICAL_EQUAL;
          tok.string.len++;
        }
      }

      tokens_push(tokens, tok);
      i += tok.string.len - 1;
      break;
    case ';':
      tok.kind = TOK_SEMICOLON;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      tok.string.len = 1;
      tokens_push(tokens, tok);
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
      tok.kind = TOK_LITERAL_INT;
      tok.file_id = fileid;
      tok.col = i - lastlinepos;
      tok.string.start = i;
      int len = 1;
      while (i + len < content.len &&
             isdigit(content.data[i + len])) {
        len++;
      }
      tok.string.len = len;
      tokens_push(tokens, tok);
      i += len - 1;
    } break;

    default: {
      if (isalpha(c)) {
        int len = 1;
        while (i + len < content.len &&
               isalnum(content.data[i + len])) {
          len++;
        }
        tok.kind = TOK_IDENTIFIER;
        tok.file_id = fileid;
        tok.col = i - lastlinepos;
        tok.string.start = i;
        tok.string.len = len;

        if(tok.string.len == 2){
          if(strncmp(content.data + tok.string.start, "if", 2) == 0){
            tok.kind = TOK_KEYWORD_IF;
          }
        }
        else if(tok.string.len == 3){
          if(strncmp(content.data + tok.string.start, "int", 3) == 0){
            tok.kind = TOK_KEYWORD_INT;
          } else if(strncmp(content.data + tok.string.start, "for", 3) == 0){
            tok.kind = TOK_KEYWORD_FOR;
          }
        } else if(tok.string.len == 4) {
          if (strncmp(content.data + tok.string.start, "else", 4) == 0) {
            tok.kind = TOK_KEYWORD_ELSE;
          }
          else if(strncmp(content.data + tok.string.start, "char", 4) == 0){
            tok.kind = TOK_KEYWORD_CHAR;
          }
        } else if(tok.string.len == 5){
          if (strncmp(content.data + tok.string.start, "break", 5) == 0) {
            tok.kind = TOK_KEYWORD_BREAK;
          }
        }
        else if(tok.string.len == 6){
          if(strncmp(content.data + tok.string.start, "struct", 6) == 0) {
            tok.kind = TOK_KEYWORD_STRUCT;
          } else if(strncmp(content.data + tok.string.start, "return", 6) == 0){
            tok.kind = TOK_KEYWORD_RETURN;
          }
        }
        tokens_push(tokens, tok);
        i += len-1;
      } else {
        tok.kind = TOK_UNKNOWN;
        tok.file_id = fileid;
        tok.col = i - lastlinepos;
        tok.string.start = i;
        tok.string.len = 1;
        tokens_push(tokens, tok);
      }
    }
    }
  }
  return 0;
}

///////////////// lexer ////////////////////////////////
int lex_init(Lexer *lexer) {
  lexer->i = 0;
  return tokens_init(&lexer->tokens);
}

int lex(Lexer *lexer, String input, int fileid){
  return tokens_add_from_string(&lexer->tokens, input, fileid);
}

Token lex_peek(Lexer *lexer) { return lex_peekn(lexer, 0); }

Token lex_peekn(Lexer *lexer, unsigned ahead) {
  Token tok = {.kind = TOK_EOF};
  if (lexer->i + ahead >= lexer->tokens.len) {
    return tok;
  }
  return lexer->tokens.data[lexer->i + ahead];
}

Token lex_next(Lexer *lexer) {
  lexer->i++;
  return lex_peek(lexer);
}

int lex_checkpoint(Lexer *lexer) { return lexer->i; }

int lex_restore(Lexer *lexer, int checkpoint) {
  int move = lexer->i - checkpoint;
  lexer->i = checkpoint;
  return move;
}

int lex_quit(Lexer *lexer) {
  lexer->i = 0;
  return tokens_quit(&lexer->tokens);
}

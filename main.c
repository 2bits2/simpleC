#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "compiler.h"

const char *test1 = {
          #include "test/test1.c"
};

int kind_size(TokenKind kind){
  switch (kind) {
  case TOK_KEYWORD_CHAR: return 1;
  case TOK_KEYWORD_INT:  return 4;
  default:
    return 0;
  }
}


int compile(FILE *file) {
  int status = 0;
  Ast *ast = parse_file(file);

  FILE *genfile = fopen("myassembly.s", "w");
  if (ast->fundef) {
    printf("succesfully parsed function ");
    printf("name: %.*s \n", ast->fundef->name.len,
           ast->charbuf.data + ast->fundef->name.start);

    print_fundef(ast->fundef, &ast->charbuf, 0);

    VarMap vars = {};
    varmap_init(&vars);

    gencode_file(genfile, ast, &vars, -1, 0);

    varmap_print(&vars, &ast->charbuf);
    varmap_quit(&vars);

  }
  fclose(genfile);

  if(ast->fundef == NULL){
    return -1;
  }

  return 0;
}

int test(const char *input){
  FILE *t1 = fmemopen((void*)input, strlen(input), "r");
  int status = compile(t1);
  fclose(t1);
  return status;
}









int main(int argc, char *argv[]) {
  test(test1);


  /* if (argc != 2) { */
  /*   fprintf(stderr, "wrong number of arguments. need a c file as input\n"); */
  /*   exit(1); */
  /* } */
  /* const char *filename = argv[1]; */
  /* FILE *file = fopen(filename, "r"); */
  /* if (!file) { */
  /*   fprintf(stderr, "couldnt open file %s for lexing \n", filename); */
  /*   exit(1); */
  /* } */
  /* compile(file); */
  /* fclose(file); */
}

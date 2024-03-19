#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

AstExpr *parse_additive_expr(Lexer *lexer);
AstExpr *parse_relational_expr(Lexer *lexer);
AstExpr *parse_equality_expr(Lexer *lexer);
AstExpr *parse_logical_and_expr(Lexer *lexer);
AstExpr *parse_logical_or_expr(Lexer *lexer);
AstFunDef *parse_fun(Lexer *lexer);




void stmt_list_create(AstBlockItemList *list, int cap){
  list->cap = cap;
  list->len = 0;
  list->data = malloc(sizeof(*list->data) * cap);
  if(!list->data){
    fprintf(stderr, "couldnt allocate %d list for stmt list \n", cap);
    exit(4);
  }
}

void stmt_list_add(AstBlockItemList *list, AstBlockItem stmt){
  if(list->len >= list->cap){
    int newcap = list->cap * 2;
    AstBlockItem *newdata = realloc(list->data, sizeof(AstBlockItem) * newcap);
    if(!newdata){
      fprintf(stderr, "couldnt reallocate %d new data for stmt list\n", newcap);
      exit(5);
    }

    list->data = newdata;
    list->cap = newcap;
  }

  list->data[list->len] = stmt;
  list->len++;
}

AstExpr *parse_expr(Lexer *lexer) {
  Token ident = peekn(lexer, 0);
  Token assign = peekn(lexer, 1);

  if (ident.kind == TOK_IDENTIFIER && assign.kind == TOK_ASSIGN) {
    next(lexer);
    next(lexer);

    AstExpr *rightval = parse_expr(lexer);
    if (rightval == NULL) {
      fprintf(stderr,
              "%d:%d expected valid expression after assignment %.*s = ... \n",
              assign.line, assign.col, ident.string.len,
              lexer->charbuf.data + ident.string.start);
      free_expr(rightval);
      free(rightval);
      return NULL;
    }

    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_ASSIGN;
    expr->assign.name = ident.string;
    expr->assign.expr = rightval;
    return expr;
  }

  return parse_logical_or_expr(lexer);
}


AstExpr *parse_logical_or_expr(Lexer *lexer) {
  AstExpr *leftExpr = parse_logical_and_expr(lexer);
  if (leftExpr == NULL) {
    return NULL;
  }

  Token tok = peek(lexer);
  while (tok.kind == TOK_LOGICAL_OR) {
    next(lexer);
    AstExpr *rightExpr = parse_logical_and_expr(lexer);
    if (rightExpr == NULL) {
      fprintf(stderr, "%d:%d expected right expression after || \n", tok.line,
              tok.col);
      free_expr(leftExpr);
      free_expr(rightExpr);
      free(leftExpr);
      free(rightExpr);
      return NULL;
    }
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_BINOP;
    expr->binop.kind = tok.kind;
    expr->binop.left = leftExpr;
    expr->binop.right = rightExpr;
    leftExpr = expr;

    tok = peek(lexer);
  }
  return leftExpr;
}

AstExpr *parse_logical_and_expr(Lexer *lexer) {
  AstExpr *leftExpr = parse_equality_expr(lexer);
  if (leftExpr == NULL) {
    return NULL;
  }

  Token tok = peek(lexer);
  while (tok.kind == TOK_LOGICAL_AND) {
    next(lexer);
    AstExpr *rightExpr = parse_equality_expr(lexer);
    if (rightExpr == NULL) {
      fprintf(stderr, "%d:%d expected right expression after && \n", tok.line,
              tok.col);
      free_expr(leftExpr);
      free_expr(rightExpr);
      free(leftExpr);
      free(rightExpr);
      return NULL;
    }
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_BINOP;
    expr->binop.kind = tok.kind;
    expr->binop.left = leftExpr;
    expr->binop.right = rightExpr;
    leftExpr = expr;

    tok = peek(lexer);
  }
  return leftExpr;
}

AstExpr *parse_equality_expr(Lexer *lexer){
  AstExpr *leftExpr = parse_relational_expr(lexer);
  if(leftExpr == NULL){
    return NULL;
  }

  Token tok = peek(lexer);
  while (tok.kind == TOK_LOGICAL_EQUAL || tok.kind == TOK_LOGICAL_NOT_EQUAL) {
    next(lexer);
    AstExpr *rightExpr = parse_relational_expr(lexer);
    if (rightExpr == NULL) {
      fprintf(stderr, "%d:%d expected right expression after != or == \n",
              tok.line, tok.col);
      free_expr(leftExpr);
      free_expr(rightExpr);
      free(leftExpr);
      free(rightExpr);
      return NULL;
    }
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_BINOP;
    expr->binop.kind = tok.kind;
    expr->binop.left = leftExpr;
    expr->binop.right = rightExpr;
    leftExpr = expr;

    tok = peek(lexer);
  }

  return leftExpr;
}


AstExpr *parse_relational_expr(Lexer *lexer) {
  AstExpr *leftAdditiveExpr = parse_additive_expr(lexer);
  if (leftAdditiveExpr == NULL) {
    return NULL;
  }

  Token tok = peek(lexer);
  while (tok.kind == TOK_LOGICAL_LESS || tok.kind == TOK_LOGICAL_LESS_EQUAL ||
         tok.kind == TOK_LOGICAL_GREATER || tok.kind == TOK_LOGICAL_GREATER_EQUAL
         ){

    next(lexer);
    AstExpr *rightAdditiveExpr = parse_additive_expr(lexer);
    if (rightAdditiveExpr == NULL) {
      fprintf(stderr, "%d:%d expected right expression after < <= > or >= \n",
              tok.line, tok.col);
      free_expr(leftAdditiveExpr);
      free_expr(rightAdditiveExpr);
      free(leftAdditiveExpr);
      free(rightAdditiveExpr);
      return NULL;
    }

    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_BINOP;
    expr->binop.kind = tok.kind;
    expr->binop.left = leftAdditiveExpr;
    expr->binop.right = rightAdditiveExpr;
    leftAdditiveExpr = expr;

    tok = peek(lexer);
  }

  return leftAdditiveExpr;
}

AstExpr *parse_factor(Lexer *lexer) {
  Token tok = peek(lexer);
  switch (tok.kind) {
  case TOK_BITCOMPLEMENT:
  case TOK_LOGICAL_NOT:
  case TOK_MINUS: {
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_UNOP;
    expr->unop.kind = tok.kind;
    next(lexer);
    expr->unop.expr = parse_factor(lexer);
    return expr;
  }
  case TOK_IDENTIFIER: {
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_VAR;
    expr->var = tok.string;
    next(lexer);
    return expr;
    break;
  }
  case TOK_LITERAL_INT: {
    AstExpr *expr = malloc(sizeof(AstExpr));
    expr->kind = EXPR_CONST;
    expr->intliteral = tok.literalint;
    next(lexer);
    return expr;
    break;
  }
  case TOK_PAREN_OPEN: {
    next(lexer);
    AstExpr *expr = parse_expr(lexer);
    if(expr == NULL){
      fprintf(stderr, "%d:%d expected expression after (\n", tok.line, tok.col);
      return NULL;
    }
    tok = peek(lexer);
    if (tok.kind != TOK_PAREN_CLOSE) {
      fprintf(stderr,
              "%d:%d expected closing paren ) after expression (code %d )\n",
              tok.line, tok.col, tok.kind);
      free_expr(expr);
      free(expr);
      return NULL;
    }
    next(lexer);
    return expr;
    break;
  }
  default:
    return NULL;
  }
  return NULL;
}


AstExpr *parse_term(Lexer *lexer) {
  AstExpr *leftFactor = parse_factor(lexer);
  if (leftFactor == NULL) {
    return NULL;
  }
  Token tok = peek(lexer);
  while (tok.kind == TOK_MUL || tok.kind == TOK_DIV) {
    next(lexer);
    AstExpr *rightFactor = parse_factor(lexer);
    if (rightFactor == NULL) {
      fprintf(stderr, "%d:%d expected right factor after * or / \n", tok.line,
              tok.col);
      free_expr(rightFactor);
      free_expr(leftFactor);
      free(leftFactor);
      free(rightFactor);
      return NULL;
    }
    AstExpr *factor = malloc(sizeof(AstExpr));
    factor->kind = EXPR_BINOP;
    factor->binop.kind = tok.kind;
    factor->binop.left = leftFactor;
    factor->binop.right = rightFactor;
    leftFactor = factor;
    tok = peek(lexer);
  }
  return leftFactor;
}

AstExpr *parse_additive_expr(Lexer *lexer) {
  AstExpr *leftTerm = parse_term(lexer);
  if (leftTerm == NULL) {
    return leftTerm;
  }
  Token tok = peek(lexer);
  while (tok.kind == TOK_PLUS || tok.kind == TOK_MINUS) {
    next(lexer);

    AstExpr *rightTerm = parse_term(lexer);
    if (rightTerm == NULL) {
      fprintf(stderr, "%d:%d expected right factor after + or - \n", tok.line,
              tok.col);
      free_expr(rightTerm);
      free_expr(leftTerm);
      free(rightTerm);
      free(leftTerm);
      return NULL;
    }

    AstExpr *term = malloc(sizeof(AstExpr));
    term->kind = EXPR_BINOP;
    term->binop.kind = tok.kind;
    term->binop.left = leftTerm;
    term->binop.right = rightTerm;
    leftTerm = term;
    tok = peek(lexer);
  }
  return leftTerm;
}

AstBlockItem *parse_declaration(Lexer *lexer) {
  Token tok = peek(lexer);
  if (tok.kind == TOK_KEYWORD_INT || tok.kind == TOK_KEYWORD_CHAR) {

    Token kind = tok;
    next(lexer);

    Token id = peek(lexer);
    if (id.kind != TOK_IDENTIFIER) {
      fprintf(stderr, "%d:%d expected identifier after int keyword\n", id.line,
              id.col);
      return NULL;
    }

    next(lexer);
    Token eq = peek(lexer);
    AstExpr *optional = NULL;
    if (eq.kind == TOK_ASSIGN) {
      next(lexer);
      optional = parse_expr(lexer);
    }

    AstBlockItem *stmt = malloc(sizeof(AstBlockItem));
    stmt->kind = BLOCK_ITEM_DECL;
    stmt->decl.name = id.string;
    stmt->decl.option = optional;
    stmt->decl.kind = kind;

    Token tok = peek(lexer);
    if (tok.kind != TOK_SEMICOLON) {
      free_stmt(stmt);
      free(stmt);
      free(optional);
      fprintf(stderr, "%d:%d expected ; after declaration \n", tok.line, tok.col);
      return NULL;
    }
    next(lexer);

    return stmt;
  }
  return NULL;
}

AstBlockItem *parse_block_item(Lexer *lexer);


AstBlockItem *parse_for_loop(Lexer *lexer) {
  Token tok = peek(lexer);
  if(tok.kind != TOK_KEYWORD_FOR){
    fprintf(stderr, "%d:%d expected keyword for for parsing\n", tok.line, tok.col);
    return NULL;
  }

  next(lexer);
  tok = peek(lexer);

  if(tok.kind != TOK_PAREN_OPEN){
    fprintf(stderr, "%d:%d expected open paren ( after for keyword\n", tok.line, tok.col);
    return NULL;
  }

  next(lexer);
  tok = peek(lexer);


  AstBlockItem *forloop = malloc(sizeof(AstBlockItem));
  forloop->kind = BLOCK_ITEM_FORLOOP;

  if (tok.kind == TOK_SEMICOLON) {
    forloop->forloop.init = NULL;
    next(lexer);
  } else {
    forloop->forloop.init = parse_block_item(lexer);
  }

  tok = peek(lexer);

  if(tok.kind == TOK_SEMICOLON){
    forloop->forloop.condition = NULL;
    next(lexer);
  } else {
    forloop->forloop.condition = parse_expr(lexer);
    tok = peek(lexer);
    if (tok.kind != TOK_SEMICOLON) {
      fprintf(stderr, "%d:%d expected second semicolon ; inside for(...) \n",
              tok.line, tok.col);
      free_stmt(forloop);
      free(forloop);
      return NULL;
    }
    next(lexer);
  }

  tok = peek(lexer);

  if (tok.kind == TOK_PAREN_CLOSE) {
    forloop->forloop.postExpr = NULL;
    next(lexer);
  } else {
    forloop->forloop.postExpr = parse_expr(lexer);
    tok = peek(lexer);
    if (tok.kind != TOK_PAREN_CLOSE) {
      fprintf(stderr, "%d:%d expected closing paren ) after for \n ", tok.line,
              tok.col);
      free_stmt(forloop);
      free(forloop);
      return NULL;
    }
    next(lexer);
  }

  AstBlockItem *body = parse_block_item(lexer);
  if(body == NULL){
    fprintf(stderr, "%d:%d expected statement body after for \n", tok.line, tok.col);
    free_stmt(forloop);
    free(forloop);
    return NULL;
  }

  forloop->forloop.body = body;
  return forloop;
}

AstBlockItemList parse_block_item_list(Lexer *lexer);



AstBlockItem *parse_return_stmt(Lexer *lexer) {
  Token tok = peek(lexer);
  if(tok.kind != TOK_KEYWORD_RETURN){ return NULL;}

  next(lexer);
  AstExpr *expr = parse_expr(lexer);
  if (expr == NULL) {
    fprintf(stderr, "%d:%d: expected valid expression after return keyword\n",
            tok.line, tok.col);
    return NULL;
  }

  AstBlockItem *stmt = malloc(sizeof(AstBlockItem));
  stmt->kind = BLOCK_ITEM_RETURN;
  stmt->expr = expr;

  tok = peek(lexer);
  if (tok.kind != TOK_SEMICOLON) {
    free_stmt(stmt);
    free(stmt);
    free(expr);
    fprintf(stderr, "%d:%d expected ; after statement \n", tok.line, tok.col);
    return NULL;
  }
  next(lexer);
  return stmt;
}
AstBlockItem *parse_stmt(Lexer *lexer);

AstBlockItem *parse_ifelse(Lexer *lexer) {
  Token tok = peek(lexer);
  if(tok.kind != TOK_KEYWORD_IF){
    return NULL;
  }

  next(lexer);
  tok = peek(lexer);
  if (tok.kind != TOK_PAREN_OPEN) {
    fprintf(stderr, "%d:%d expected open paren ( after if keyword\n", tok.line,
            tok.col);
    return NULL;
  }

  next(lexer);

  AstExpr *condition_expr = parse_expr(lexer);
  if (condition_expr == NULL) {
    fprintf(stderr, "%d:%d expected expression after if \n", tok.line, tok.col);
    return NULL;
  }

  Token closeParen = peek(lexer);
  if (closeParen.kind != TOK_PAREN_CLOSE) {
    fprintf(stderr, "%d:%d expected close paren ) after if expression \n",
            tok.line, tok.col);

    free_expr(condition_expr);
    free(condition_expr);

    return NULL;
  }
  next(lexer);

  AstBlockItem *true_stmt = parse_stmt(lexer);
  if (true_stmt == NULL) {
    free_expr(condition_expr);
    free(condition_expr);
    fprintf(stderr, "%d:%d expected statement after if \n", closeParen.line,
            closeParen.col);
    return NULL;
  }

  Token elsetok = peek(lexer);
  if (elsetok.kind != TOK_KEYWORD_ELSE) {
    AstBlockItem *ifelse = malloc(sizeof(AstBlockItem));
    ifelse->kind = BLOCK_ITEM_IFELSE;
    ifelse->ifelse.condition = condition_expr;
    ifelse->ifelse.truestmt = true_stmt;
    ifelse->ifelse.elsestmt = NULL;
    return ifelse;
  }

  next(lexer);

  AstBlockItem *else_stmt = parse_stmt(lexer);
  if (else_stmt == NULL) {
    free_expr(condition_expr);
    free(condition_expr);

    free_stmt(true_stmt);
    free(true_stmt);
    fprintf(stderr, "%d:%d expected statement after else \n", elsetok.line,
            elsetok.col);
    return NULL;
  }

  AstBlockItem *ifelse = malloc(sizeof(AstBlockItem));
  ifelse->kind = BLOCK_ITEM_IFELSE;
  ifelse->ifelse.condition = condition_expr;
  ifelse->ifelse.truestmt = true_stmt;
  ifelse->ifelse.elsestmt = else_stmt;
  return ifelse;
}


AstBlockItem *parse_compound_statement(Lexer *lexer){
  Token tok = peek(lexer);
  if(tok.kind != TOK_BRACE_OPEN){
    return NULL;
  }

  next(lexer);
  AstBlockItem *item = malloc(sizeof(AstBlockItem));
  item->kind = BLOCK_ITEM_COMPOUND;
  item->compound.blockitems = parse_block_item_list(lexer);

  tok = peek(lexer);
  if (tok.kind != TOK_BRACE_CLOSE) {
    fprintf(stderr, "%d:%d compound statement misses closing brace }\n",
            tok.line, tok.col);
    free_stmt(item);
    free(item);
    return NULL;
  }
  next(lexer);
  return item;
}

AstBlockItem *parse_single_statement(Lexer *lexer){
  Token tok = peek(lexer);
  AstExpr *singleExpr = parse_expr(lexer);
  if (singleExpr == NULL) {
    fprintf(stderr, "%d:%d couldnt parse statement as a single epr\n", tok.line,
            tok.col);
    return NULL;
  }

  AstBlockItem *stmt = malloc(sizeof(AstBlockItem));
  stmt->kind = BLOCK_ITEM_EXPR;
  stmt->expr = singleExpr;

  tok = peek(lexer);
  if (tok.kind != TOK_SEMICOLON) {
    free_stmt(stmt);
    free(stmt);
    fprintf(stderr,
            "%d:%d expected ; after expression to make it a statement \n",
            tok.line, tok.col);
    return NULL;
  }
  next(lexer);
  return stmt;
}

AstBlockItem *parse_stmt(Lexer *lexer) {
  Token tok = peek(lexer);
  switch (tok.kind) {
  case TOK_KEYWORD_FOR:
    return parse_for_loop(lexer);
  case TOK_KEYWORD_RETURN:
    return parse_return_stmt(lexer);
  case TOK_KEYWORD_IF:
    return parse_ifelse(lexer);
  case TOK_BRACE_OPEN:
    return parse_compound_statement(lexer);
  default:
    return parse_single_statement(lexer);
  }
  return NULL;
}

AstBlockItem *parse_block_item(Lexer *lexer) {
  AstBlockItem *item = parse_declaration(lexer);
  if (item != NULL) {
    return item;
  }
  return parse_stmt(lexer);
}


AstBlockItemList parse_block_item_list(Lexer *lexer) {
  AstBlockItemList list;
  stmt_list_create(&list, 8);
  while (1) {
    int i = lexer->tokens.i;
    Token tok = peek(lexer);
    if (tok.kind == TOK_BRACE_CLOSE) {
      break;
    }
    AstBlockItem *block_item = parse_block_item(lexer);
    if (block_item == NULL) {
      break;
    }
    stmt_list_add(&list, *block_item);
    free(block_item);
  }
  return list;
}

AstFunDef *parse_fun(Lexer *lexer) {
  static const char *names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  if (!tokenpeekable_expect(&lexer->tokens, TOK_KEYWORD_INT, "int keyword")) {
    return NULL;
  }
  next(lexer);

  if (!tokenpeekable_expect(&lexer->tokens, TOK_IDENTIFIER, "identifier")) {
    return NULL;
  }
  Token identtok = peek(lexer);
  next(lexer);

  if (!tokenpeekable_expect(&lexer->tokens, TOK_PAREN_OPEN, "open paren (")) {
    return NULL;
  }
  next(lexer);

  if (!tokenpeekable_expect(&lexer->tokens, TOK_PAREN_CLOSE, "close paren )")) {
    return NULL;
  }
  next(lexer);

  if (!tokenpeekable_expect(&lexer->tokens, TOK_BRACE_OPEN, "brace open {")) {
    return NULL;
  }
  next(lexer);

  AstBlockItemList list = parse_block_item_list(lexer);

  if (!tokenpeekable_expect(&lexer->tokens, TOK_BRACE_CLOSE, "brace close }")) {
    free_stmt_list(list);
    return NULL;
  }

  if (list.len == 0 || list.data[list.len - 1].kind != BLOCK_ITEM_RETURN) {
    fprintf(stderr, "warning: function %.*s is missing a return statement \n",
            identtok.string.len, lexer->charbuf.data + identtok.string.start);

    AstBlockItem stmt;
    stmt.kind = BLOCK_ITEM_RETURN;
    stmt.expr = malloc(sizeof(AstExpr));
    stmt.expr->kind = EXPR_CONST;
    stmt.expr->intliteral = 0;
    stmt_list_add(&list, stmt);
  }

  AstFunDef *fundef = malloc(sizeof(AstFunDef));
  fundef->name = identtok.string;
  fundef->stmts = list;
  return fundef;
}

Ast *parse_file(FILE *file) {
  Lexer lexer = {};
  lex_init(&lexer);
  lex_file(&lexer, file);
  Ast *ast = malloc(sizeof(Ast));
  ast->charbuf = lexer.charbuf;
  ast->fundef = parse_fun(&lexer);
  return ast;
}

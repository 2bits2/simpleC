#include "compiler.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void     astnode_free(AstNode *node);
AstNode *parse_expr(Parser *parser);

String parser_token_content(Parser *parser, Token tok);

void astnode_print(Parser *parser, FILE *file, AstNode *node, int indent);

int tok_is_maybe_type(Token tok) {
  switch (tok.kind) {
  case TOK_KEYWORD_CHAR:
  case TOK_KEYWORD_INT:
  case TOK_IDENTIFIER:
    return 1;
  default:
    return 0;
  }
}

int astnodelist_init(AstNodeList *list) {
  list->len = 0;
  list->cap = 10;
  list->nodes = malloc(sizeof(AstNode) * list->cap);
  if (list->nodes == NULL) {
    fprintf(stderr, "couldnt initialize ast node list \n");
    return -1;
  }
  return 1;
}

void astnodelist_free_nodes(AstNodeList *list) {
  for (int i = 0; i < list->len; i++) {
    astnode_free(list->nodes[i]);
  }
  list->nodes = NULL;
  list->cap = 0;
  list->len = 0;
}

int astnodelist_push(AstNodeList *list, AstNode *node) {
  if (list->len >= list->cap) {
    int newcap = list->cap * 2;
    AstNode **newdata = realloc(list->nodes, sizeof(AstNode *) * newcap);
    if (!newdata) {
      fprintf(stderr, "couldnt reallocated nodelist \n");
      return -1;
    }
    list->nodes = newdata;
    list->cap = newcap;
  }

  list->nodes[list->len] = node;
  list->len++;
  return 0;
}

AstNode *astnode_new() {
  AstNode *node = malloc(sizeof(AstNode));
  node->kind = AST_ERROR;
  node->error.kind = ERR_UNINITIALIZED_NODE;
  node->type = NULL;
  return node;
}

AstNode *astnode_structure_new() {
  AstNode *node = malloc(sizeof(AstNode));
  node->kind = AST_STRUCT;
  node->type = NULL;
  astnodelist_init(&node->structure.members);
  node->structure.members_all_defined = false;
  node->structure.size = -1;
  return node;
}

void astnode_unexpected_token(AstNode *node, Token actual,
                              const char *triedToParse) {
  node->kind = AST_ERROR;
  node->error.kind = ERR_UNEXPECTED_TOKEN;
  node->error.unexpectedToken.actual = actual;
  node->error.unexpectedToken.triedToParse = triedToParse;
}

void astnode_invalid_ast(AstNode *node, AstNode *invalid,
                         const char *description, Token context_token) {
  node->kind = AST_ERROR;
  node->error.kind = ERR_INVALID_AST;
  node->error.invalid.node = invalid;
  node->error.invalid.description = description;
  node->error.invalid.context_token = context_token;
}

void astnode_free(AstNode *node) {
  if (node == NULL) {
    return;
  }
  switch (node->kind) {
  case AST_FORLOOP: {
    astnode_free(node->forloop.init);
    astnode_free(node->forloop.condition);
    astnode_free(node->forloop.step);
    astnode_free(node->forloop.stmt);
    free(node);
    break;
  }
  case AST_BREAK: {
    free(node);
    break;
  }
  case AST_RETURN: {
    astnode_free(node->ret.expr);
    break;
  }
  case AST_IF_ELSE: {
    astnode_free(node->ifelse.condition);
    astnode_free(node->ifelse.elseblock);
    astnode_free(node->ifelse.ifblock);
    break;
  }
  case AST_FUNC_CALL: {
    astnodelist_free_nodes(&node->funccall.params);
    free(node);
    break;
  }
  case AST_FUNC_DEF: {
    astnode_free(node->func.prototype);
    astnode_free(node->func.block);
    free(node);
    break;
  }
  case AST_FUNC_PROTO: {
    astnodelist_free_nodes(&node->funcproto.params);
    free(node);
    break;
  }
  case AST_PROGRAM: {
    astnodelist_free_nodes(&node->program.items);
    free(node);
    break;
  }
  case AST_BLOCK: {
    astnodelist_free_nodes(&node->block);
    free(node);
    break;
  }
  case AST_LITERAL:
    free(node);
    break;
  case AST_VAR: {
    AstNode *member = node->var.member_access;
    while(member ){
      AstNode *next = member->member.next;
      free(member);
      member = next;
    }
    free(node);
    break;
  }
  case AST_DECL:
    astnode_free(node->decl.expr);
    free(node);
    break;
  case AST_ERROR: {
    switch (node->error.kind) {
    case ERR_UNINITIALIZED_NODE:
    case ERR_UNEXPECTED_TOKEN:
      free(node);
      break;
    case ERR_INVALID_AST:
      astnode_free(node->error.invalid.node);
      free(node);
      break;
    }
    break;
  }
  case AST_STRUCT:
    astnodelist_free_nodes(&node->structure.members);
    free(node);
    break;
  case AST_BINOP:
    astnode_free(node->binop.left);
    astnode_free(node->binop.right);
    free(node);
    break;
  case AST_UNOP:
    astnode_free(node->unop.expr);
    free(node);
    break;
  case AST_MEMBER_ACCESS:
    break;
  }
}

AstNode *parse_member_access(Parser *parser){
  AstNode *last = astnode_new();
  AstNode *node = last;
  AstNode *prev = NULL;
  Token tok = lex_peek(&parser->lexer);
  if(tok.kind != TOK_DOT){
    astnode_unexpected_token(node, tok, " dot . for member access");
    return node;
  }
  while(tok.kind == TOK_DOT){
    Token identifier = lex_next(&parser->lexer);
    if(identifier.kind != TOK_IDENTIFIER){
      astnode_unexpected_token(node, identifier, " identifier after dot for member access");
      return node;
    }
    last->kind = AST_MEMBER_ACCESS;
    last->member.name = identifier;
    last->member.next = astnode_new();
    prev = last;
    last = last->member.next;
    tok = lex_next(&parser->lexer);
  }
  if(prev != NULL){
    astnode_free(last);
    prev->member.next = NULL;
  }
  return node;
}

AstNode *parse_var(Parser *parser) {
  AstNode *var = astnode_new();
  Token identifier = lex_peek(&parser->lexer);
  if (identifier.kind != TOK_IDENTIFIER) {
    astnode_unexpected_token(var, identifier,
                             "expected identifier as variable name");
    return var;
  }

  var->kind = AST_VAR;
  var->var.name = identifier;
  var->var.member_access = NULL;

  AstNode *decl =
      scope_get(&parser->scope, str_slice_to_uint64(identifier.string));

  var->var.declaration = decl;
  if (decl == NULL) {
    astnode_unexpected_token(var, identifier, "previously declared variable");
    return var;
  }

  Token tok = lex_next(&parser->lexer);
  if (tok.kind == TOK_DOT) {
    var->var.member_access = parse_member_access(parser);
  }

  /* lex_next(&parser->lexer); */
  return var;
}

AstNode *parse_funccall(Parser *parser) {
  AstNode *node = astnode_new();
  node->kind = AST_FUNC_CALL;
  Token tok = lex_peek(&parser->lexer);

  if (tok.kind != TOK_IDENTIFIER) {
    astnode_unexpected_token(node, tok, "name of function call");
    return node;
  }

  node->funccall.name = tok;

  tok = lex_next(&parser->lexer);

  if (tok.kind != TOK_PAREN_OPEN) {
    astnode_unexpected_token(node, tok,
                             "starting ( for function call arguments ");
    return node;
  }

  tok = lex_next(&parser->lexer);

  astnodelist_init(&node->funccall.params);

  while (tok.kind != TOK_EOF && tok.kind != TOK_PAREN_CLOSE) {

    AstNode *expr = parse_expr(parser);
    astnodelist_push(&node->funccall.params, expr);

    tok = lex_peek(&parser->lexer);
    if (tok.kind == TOK_COMMA) {
      tok = lex_next(&parser->lexer);
    } else if (tok.kind != TOK_PAREN_CLOSE) {

      AstNode *unexpected = astnode_new();
      astnode_unexpected_token(unexpected, tok,
                               "comma , in function argument list");
      astnodelist_push(&node->funccall.params, unexpected);
      while (tok.kind != TOK_SEMICOLON && tok.kind != TOK_EOF &&
             tok.kind != TOK_PAREN_CLOSE) {
        tok = lex_next(&parser->lexer);
      }
      return unexpected;
      break;
    }
  }

  lex_next(&parser->lexer);
  return node;
}

AstNode *parse_factor(Parser *parser) {
  Token tok = lex_peek(&parser->lexer);
  AstNode *node = astnode_new();
  switch (tok.kind) {

  case TOK_LITERAL_INT:
    node->kind = AST_LITERAL;
    node->literal = tok;
    lex_next(&parser->lexer);
    break;

  case TOK_MUL:
    node->kind = AST_UNOP;
    node->unop.op = tok;
    lex_next(&parser->lexer);
    node->unop.expr = parse_factor(parser);
    break;

  case TOK_IDENTIFIER: {

    Token ahead = lex_peekn(&parser->lexer, 1);
    if (ahead.kind == TOK_PAREN_OPEN) {
      return parse_funccall(parser);
    }

    AstNode *var = parse_var(parser);
    if (var->kind != AST_ERROR) {
      free(node);
      return var;
    }
    astnode_invalid_ast(node, var,
                        "tried to parse variable as primary expression", tok);
    return node;
  } break;

  case TOK_SINGLE_AMPERSAND: {
    lex_next(&parser->lexer);
    AstNode *expr = parse_factor(parser);
    if (expr->kind == AST_ERROR) {
      astnode_invalid_ast(
          node, expr, "tried to parse expression after unary operator & ", tok);
      return node;
    }
    node->kind = AST_UNOP;
    node->unop.expr = expr;
    node->unop.op = tok;
    break;
  }
  case TOK_MINUS: {
    lex_next(&parser->lexer);
    AstNode *expr = parse_factor(parser);
    if (expr->kind == AST_ERROR) {
      astnode_invalid_ast(
          node, expr, "after unary operator - is something unexpected", tok);
      return node;
    }
    node->kind = AST_UNOP;
    node->unop.expr = expr;
    node->unop.op = tok;
    break;
  }
  case TOK_PAREN_OPEN: {
    lex_next(&parser->lexer);
    AstNode *expr = parse_expr(parser);
    if (expr->kind == AST_ERROR) {
      astnode_invalid_ast(node, expr,
                          "tried to parse expression inside parens (...)", tok);
      return node;
    }

    tok = lex_peek(&parser->lexer);
    if (tok.kind != TOK_PAREN_CLOSE) {
      astnode_unexpected_token(node, tok, "closing paren ) of expression");
      return node;
    }
    lex_next(&parser->lexer);
    free(node);
    return expr;
  }
  default:
    astnode_unexpected_token(node, tok, "factor");
    lex_next(&parser->lexer);
    break;
  }
  return node;
}

int op_precedence(Token tok) {
  switch (tok.kind) {

  case TOK_LOGICAL_EQUAL:
  case TOK_LOGICAL_LESS:
  case TOK_LOGICAL_GREATER_EQUAL:
  case TOK_LOGICAL_LESS_EQUAL:
  case TOK_LOGICAL_GREATER:
    return 2;

  case TOK_MUL:
    return 10;
  case TOK_DIV:
    return 10;
  case TOK_PLUS:
    return 5;
  case TOK_MINUS:
    return 5;
  case TOK_ASSIGN:
    return 1;
  default:
    return 0;
  }
}

AstNode *parse_expr_1(Parser *parser, AstNode *left, int min_precedence) {
  Token lookahead = lex_peek(&parser->lexer);

  while (op_precedence(lookahead) >= min_precedence) {
    Token op = lookahead;
    lex_next(&parser->lexer);

    AstNode *right = parse_factor(parser);
    if (right->kind == AST_ERROR) {
      astnode_invalid_ast(left, right, "I expected a primary expression", op);
      return left;
    }

    lookahead = lex_peek(&parser->lexer);

    while (op_precedence(lookahead) > op_precedence(op)) {
      right = parse_expr_1(parser, right, op_precedence(op) + 1);

      if (right->kind == AST_ERROR) {
        astnode_invalid_ast(left, right, "wanted any right expression",
                            lookahead);
        return left;
      }

      lookahead = lex_peek(&parser->lexer);
    }

    AstNode *binop = astnode_new();
    binop->kind = AST_BINOP;
    binop->binop.left = left;
    binop->binop.right = right;
    binop->binop.op = op;
    left = binop;
  }
  return left;
}

AstNode *parse_expr(Parser *parser) {
  AstNode *expr = parse_factor(parser);
  return parse_expr_1(parser, expr, 1);
}

int size_of_type(Parser *parser, Token kind) {
  int size = -1;
  if (kind.kind == TOK_KEYWORD_INT) {
    size = 4;
  } else if (kind.kind == TOK_KEYWORD_CHAR) {
    size = 1;
  } else if (kind.kind == TOK_IDENTIFIER) {
    uint64_t key = str_slice_to_uint64(kind.string);
    AstNode *def = ptr_bucket_get(&parser->struct_definitions, key);
    if (def != NULL) {
      assert(def->kind == AST_STRUCT);
      size = def->structure.size;
    }
  }
  return size;
}

int push_decl_to_scope(Parser *parser, AstNode *decl) {
  assert(decl->kind == AST_DECL);
  uint64_t key = str_slice_to_uint64(decl->decl.name.string);
  return scope_put(&parser->scope, key, decl);
}

AstNode *parse_decl(Parser *parser, int parseSemicolon) {
  Token tok = lex_peek(&parser->lexer);
  AstNode *node = astnode_new();

  if (!tok_is_maybe_type(tok)) {
    astnode_unexpected_token(
        node, tok,
        "variable declaration (starting with name of type like int or char)");
    return node;
  }

  Token kind = tok;

  tok = lex_next(&parser->lexer);

  int num_pointers = 0;
  while (tok.kind == TOK_MUL) {
    num_pointers++;
    tok = lex_next(&parser->lexer);
  }

  if (tok.kind != TOK_IDENTIFIER) {
    astnode_unexpected_token(node, tok, "variable name for declaration");
    lex_next(&parser->lexer);
    return node;
  }

  Token name = tok;

  tok = lex_next(&parser->lexer);

  if (parseSemicolon && tok.kind == TOK_SEMICOLON) {
    lex_next(&parser->lexer);
    node->kind = AST_DECL;
    node->decl.kind = kind;
    node->decl.name = name;
    node->decl.num_pointers = num_pointers;
    node->decl.size = size_of_type(parser, kind);
    if (num_pointers > 0) {
      node->decl.size = 8;
    }
    node->decl.expr = NULL;

    push_decl_to_scope(parser, node);
    return node;
  }

  if (!parseSemicolon &&
      (tok.kind == TOK_PAREN_CLOSE || tok.kind == TOK_COMMA)) {
    node->kind = AST_DECL;
    node->decl.kind = kind;
    node->decl.name = name;
    node->decl.expr = NULL;
    node->decl.size = size_of_type(parser, kind);
    node->decl.num_pointers = num_pointers;
    if (num_pointers > 0) {
      node->decl.size = 8;
    }

    push_decl_to_scope(parser, node);
    return node;
  }

  if (tok.kind != TOK_ASSIGN) {
    astnode_unexpected_token(node, tok,
                             "assignment (=) after declaration name ");
    return node;
  }

  lex_next(&parser->lexer);

  AstNode *expr = parse_expr(parser);
  if (expr->kind == AST_ERROR) {
    astnode_invalid_ast(node, expr, "expression after = seems to be invalid",
                        tok);
    return node;
  }

  tok = lex_peek(&parser->lexer);

  if (parseSemicolon) {
    if (tok.kind != TOK_SEMICOLON) {
      astnode_unexpected_token(node, tok,
                               "semicolon ; of declaration statement");
      tok = lex_next(&parser->lexer);
      return node;
    } else {
      tok = lex_next(&parser->lexer);
    }
  }

  node->kind = AST_DECL;
  node->decl.kind = kind;
  node->decl.name = name;
  node->decl.expr = expr;
  node->decl.num_pointers = num_pointers;
  node->decl.size = size_of_type(parser, kind);
  node->decl.assembly_base_offset = 0;
  if (num_pointers > 0) {
    node->decl.size = 8;
  }

  push_decl_to_scope(parser, node);
  return node;
}

AstNode *parse_stmt_block(Parser *parser);

AstNode *parse_return(Parser *parser) {
  AstNode *node = astnode_new();
  Token tok = lex_peek(&parser->lexer);
  if (tok.kind != TOK_KEYWORD_RETURN) {
    astnode_unexpected_token(node, tok, "return keyword");
    return node;
  }
  tok = lex_next(&parser->lexer);

  if (tok.kind == TOK_SEMICOLON) {
    lex_next(&parser->lexer);
    node->kind = AST_RETURN;
    node->ret.expr = NULL;
    return node;
  }

  AstNode *expr = parse_expr(parser);
  if (expr->kind == AST_ERROR) {
    astnode_invalid_ast(node, expr, "expected expression after return", tok);
    return node;
  }

  tok = lex_peek(&parser->lexer);
  if (tok.kind != TOK_SEMICOLON) {
    astnode_unexpected_token(node, tok, "semicolon after return stmt");
    return node;
  }
  lex_next(&parser->lexer);

  node->kind = AST_RETURN;
  node->ret.expr = expr;
  return node;
}
AstNode *parse_for(Parser *parser);
AstNode *parse_ifelse(Parser *parser);

AstNode *parse_stmt(Parser *parser, int consumeSemicolon) {

  Token tok = lex_peek(&parser->lexer);

  if (consumeSemicolon) {
    while (tok.kind == TOK_SEMICOLON) {
      tok = lex_next(&parser->lexer);
    }
  }

  AstNode *node = astnode_new();
  Token ahead = lex_peekn(&parser->lexer, 1);

  if (tok.kind == TOK_KEYWORD_RETURN) {
    AstNode *ret = parse_return(parser);
    if (ret->kind == AST_ERROR) {
      astnode_invalid_ast(node, ret, "expected statement", tok);
      return node;
    }
    free(node);
    return ret;
  }

  if (tok.kind == TOK_KEYWORD_BREAK) {
    node->kind = AST_BREAK;
    tok = lex_next(&parser->lexer);
    if (consumeSemicolon) {
      if (tok.kind != TOK_SEMICOLON) {
        astnode_unexpected_token(node, tok, "semicolon ; after break");
        return node;
      }
      lex_next(&parser->lexer);
    }
    return node;
  }

  if (tok.kind == TOK_KEYWORD_IF) {
    AstNode *ifelse = parse_ifelse(parser);
    if (ifelse->kind == AST_ERROR) {
      astnode_invalid_ast(node, ifelse, "expected if statement", tok);
      return node;
    }
    free(node);
    return ifelse;
  }

  if (tok.kind == TOK_KEYWORD_FOR) {
    AstNode *fornode = parse_for(parser);
    if (fornode->kind == AST_ERROR) {
      astnode_invalid_ast(node, fornode, "expected for loop", tok);
      return node;
    }
    free(node);
    return fornode;
  }

  if (tok.kind == TOK_MUL) {
    astnode_unexpected_token(node, tok,
                             "dereferenced assignment but "
                             "its not supported yet");
    return node;
  }

  // block
  if (tok.kind == TOK_BRACE_OPEN) {
    AstNode *block = parse_stmt_block(parser);
    if (block->kind == AST_ERROR) {
      astnode_invalid_ast(node, block, "expected statement block because of {",
                          tok);
      return node;
    }
    free(node);
    return block;
  }

  // declaration
  if ((tok.kind == TOK_IDENTIFIER || tok.kind == TOK_KEYWORD_CHAR ||
       tok.kind == TOK_KEYWORD_INT) &&
      ahead.kind == TOK_IDENTIFIER) {
    AstNode *decl = parse_decl(parser, consumeSemicolon);
    if (decl->kind == AST_ERROR) {
      astnode_invalid_ast(node, decl, "expected variable declaration", tok);
      return node;
    }
    free(node);
    return decl;
  }

  // assignment
  /* if (tok.kind == TOK_IDENTIFIER && ahead.kind == TOK_ASSIGN) { */

  /*   AstNode *var = parse_var(parser); */
  /*   if (var->kind == AST_ERROR) { */
  /*     astnode_invalid_ast(node, var, "expected varable in assignment statement", */
  /*                         tok); */
  /*     return node; */
  /*   } */

  /*   lex_next(&parser->lexer); */
  /*   AstNode *expr = parse_expr(parser); */

  /*   if (consumeSemicolon) { */
  /*     Token semicolon = lex_peek(&parser->lexer); */
  /*     if (semicolon.kind != TOK_SEMICOLON) { */
  /*       astnode_unexpected_token(node, semicolon, "semicolon of statement"); */
  /*       lex_next(&parser->lexer); */
  /*       return node; */
  /*     } */
  /*     lex_next(&parser->lexer); */
  /*   } */

  /*   node->kind = AST_ASSIGN; */
  /*   node->assign.var = var; */
  /*   node->assign.expr = expr; */
  /*   return node; */
  /* } */

  // expression
  AstNode *expr = parse_expr(parser);
  if (expr->kind == AST_ERROR) {
    astnode_invalid_ast(node, expr, "expected an expression statement", tok);
    lex_next(&parser->lexer);
    return node;
  }

  if (consumeSemicolon) {
    Token semicolon = lex_peek(&parser->lexer);
    if (semicolon.kind != TOK_SEMICOLON) {
      astnode_unexpected_token(node, semicolon, "semicolon of statement");
      return node;
    }
  }

  lex_next(&parser->lexer);
  free(node);
  return expr;
}

AstNode *parse_ifelse(Parser *parser) {
  Token tok = lex_peek(&parser->lexer);
  AstNode *node = astnode_new();

  if (tok.kind != TOK_KEYWORD_IF) {
    astnode_unexpected_token(node, tok, "if keyword");
    return node;
  }

  tok = lex_next(&parser->lexer);

  if (tok.kind != TOK_PAREN_OPEN) {
    astnode_unexpected_token(node, tok, "open paren ( after if keyword");
    return node;
  }

  tok = lex_next(&parser->lexer);

  AstNode *condition = parse_expr(parser);
  if (condition->kind == AST_ERROR) {
    astnode_invalid_ast(node, condition,
                        "expected expression as condition for if branch", tok);
    return node;
  }

  tok = lex_peek(&parser->lexer);

  if (tok.kind != TOK_PAREN_CLOSE) {
    astnode_unexpected_token(node, tok, "close paren ) after if expression");
    astnode_free(condition);
    return node;
  }

  tok = lex_next(&parser->lexer);

  AstNode *stmt = parse_stmt(parser, 1);
  if (stmt->kind == AST_ERROR) {
    astnode_invalid_ast(node, stmt, "expected valid compound statement for if",
                        tok);
    astnode_free(condition);
    return node;
  }

  tok = lex_peek(&parser->lexer);

  if (tok.kind != TOK_KEYWORD_ELSE) {
    node->kind = AST_IF_ELSE;
    node->ifelse.condition = condition;
    node->ifelse.ifblock = stmt;
    node->ifelse.elseblock = NULL;
    return node;
  }

  tok = lex_next(&parser->lexer);

  AstNode *elsestmt = parse_stmt(parser, 1);
  if (elsestmt->kind == AST_ERROR) {
    astnode_invalid_ast(node, elsestmt, "expected correct else statement", tok);
    astnode_free(condition);
    astnode_free(stmt);
    return node;
  }

  node->kind = AST_IF_ELSE;
  node->ifelse.condition = condition;
  node->ifelse.ifblock = stmt;
  node->ifelse.elseblock = elsestmt;
  return node;
}

AstNode *parse_for(Parser *parser) {
  Token tok = lex_peek(&parser->lexer);
  AstNode *node = astnode_new();

  if (tok.kind != TOK_KEYWORD_FOR) {
    astnode_unexpected_token(node, tok, "for keyword");
    return node;
  }

  tok = lex_next(&parser->lexer);

  if (tok.kind != TOK_PAREN_OPEN) {
    astnode_unexpected_token(node, tok, "open paren ( after for");
    return node;
  }

  tok = lex_next(&parser->lexer);

  AstNode *init = NULL;
  if (tok.kind != TOK_SEMICOLON) {
    init = parse_stmt(parser, 1);
    if (init->kind == AST_ERROR) {
      astnode_invalid_ast(node, init, "expected initial statement in for loop",
                          tok);
      return node;
    }
  } else {
    lex_next(&parser->lexer);
  }

  tok = lex_peek(&parser->lexer);

  AstNode *condition = NULL;
  if (tok.kind != TOK_SEMICOLON) {
    condition = parse_expr(parser);
    if (condition->kind == AST_ERROR) {
      astnode_free(init);
      astnode_invalid_ast(node, condition, "expected condition in for loop",
                          tok);
    }

    tok = lex_peek(&parser->lexer);

    if (tok.kind != TOK_SEMICOLON) {
      astnode_free(init);
      astnode_free(condition);
      astnode_unexpected_token(node, tok,
                               "second semicolon ; for loop condition");
      return node;
    }

    lex_next(&parser->lexer);

  } else {
    lex_next(&parser->lexer);
  }

  tok = lex_peek(&parser->lexer);

  AstNode *step = NULL;
  if (tok.kind != TOK_PAREN_CLOSE) {
    step = parse_stmt(parser, 0);
    if (step->kind == AST_ERROR) {
      astnode_free(init);
      astnode_free(condition);
      astnode_invalid_ast(node, step, "expected step in for loop", tok);
      return node;
    }
  }

  tok = lex_peek(&parser->lexer);

  if (tok.kind != TOK_PAREN_CLOSE) {
    astnode_free(init);
    astnode_free(condition);
    astnode_free(step);
    astnode_unexpected_token(node, tok, " closing paren ) in for loop");
    return node;
  }
  lex_next(&parser->lexer);

  AstNode *stmt = NULL;

  stmt = parse_stmt(parser, 1);
  node->kind = AST_FORLOOP;
  node->forloop.init = init;
  node->forloop.condition = condition;
  node->forloop.step = step;
  node->forloop.stmt = stmt;
  return node;
}

AstNode *parse_func_proto(Parser *parser) {
  AstNode *node = astnode_new();
  Token rettype = lex_peek(&parser->lexer);
  if (rettype.kind != TOK_IDENTIFIER && rettype.kind != TOK_KEYWORD_INT &&
      rettype.kind != TOK_KEYWORD_CHAR) {
    astnode_unexpected_token(node, rettype, "return type for function ");
    lex_next(&parser->lexer);
    return node;
  }

  Token funcname = lex_next(&parser->lexer);
  if (funcname.kind != TOK_IDENTIFIER) {
    astnode_unexpected_token(node, rettype, "identifier as function name");
    lex_next(&parser->lexer);
    return node;
  }

  Token openParen = lex_next(&parser->lexer);
  if (openParen.kind != TOK_PAREN_OPEN) {
    astnode_unexpected_token(node, openParen,
                             "( open paren for function parameter list");
    lex_next(&parser->lexer);
    return node;
  }
  node->kind = AST_FUNC_PROTO;
  node->funcproto.name = funcname;
  node->funcproto.retType = rettype;

  astnodelist_init(&node->funcproto.params);

  Token tok = lex_next(&parser->lexer);
  while (tok.kind != TOK_EOF && tok.kind != TOK_PAREN_CLOSE) {
    AstNode *decl = parse_decl(parser, 0);
    tok = lex_peek(&parser->lexer);
    if (tok.kind == TOK_COMMA) {
      astnodelist_push(&node->funcproto.params, decl);
      tok = lex_next(&parser->lexer);
      continue;
    } else if (tok.kind == TOK_PAREN_CLOSE) {
      astnodelist_push(&node->funcproto.params, decl);
      break;
    } else {
      astnodelist_free_nodes(&node->funcproto.params);
      astnode_unexpected_token(node, tok,
                               ", or ) after function parameter list");
      return node;
    }
  }
  if (tok.kind != TOK_PAREN_CLOSE) {
    AstNode *err = astnode_new();
    astnode_invalid_ast(err, node,
                        "expected closing paren ) "
                        "for function parameter list",
                        tok);
    return err;
  }
  lex_next(&parser->lexer);
  return node;
}

AstNode *parse_stmt_block(Parser *parser) {
  AstNode *node = astnode_new();
  node->kind = AST_BLOCK;

  Token brace_open_tok = lex_peek(&parser->lexer);
  if (brace_open_tok.kind != TOK_BRACE_OPEN) {
    astnode_unexpected_token(node, brace_open_tok,
                             "open brace { for statement block");
    return node;
  }

  Token tok = lex_next(&parser->lexer);

  Scope newscope;
  scope_init(&newscope);
  newscope.parent = &parser->scope;

  astnodelist_init(&node->block);

  while (tok.kind != TOK_BRACE_CLOSE && tok.kind != TOK_EOF) {
    AstNode *stmt = parse_stmt(parser, 1);
    astnodelist_push(&node->block, stmt);

    if (stmt->kind == AST_ERROR) {
      while (tok.kind != TOK_SEMICOLON && tok.kind != TOK_EOF) {
        tok = lex_next(&parser->lexer);
      }
      tok = lex_next(&parser->lexer);
    }

    tok = lex_peek(&parser->lexer);
  }

  scope_quit(&newscope);

  if (tok.kind != TOK_BRACE_CLOSE) {
    AstNode *errnode = astnode_new();
    astnode_invalid_ast(errnode, node,
                        "expected corresponding closing brace } of "
                        "statement block",
                        brace_open_tok);
    return errnode;
  }
  lex_next(&parser->lexer);
  return node;
}

void parse_struct_decl_stmts(Parser *parser, AstNode *structure) {
  AstNode *stmt = NULL;
  static const char *token_names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  Token tok = lex_peek(&parser->lexer);

  int size = 0;
  bool hasErrors = false;

  bool all_members_defined = true;

  uint64_t struct_name_key =
      str_slice_to_uint64(structure->structure.name.string);

  while (tok.kind != TOK_EOF && tok.kind != TOK_BRACE_CLOSE) {

    stmt = parse_decl(parser, 1);
    tok = lex_peek(&parser->lexer);
    if (stmt->kind == AST_ERROR) {
      while (tok.kind != TOK_SEMICOLON && tok.kind != TOK_BRACE_CLOSE &&
             tok.kind != TOK_EOF) {
        printf("%s", token_names[tok.kind]);
        tok = lex_next(&parser->lexer);
      }
      if (tok.kind == TOK_SEMICOLON) {
        tok = lex_next(&parser->lexer);
      }
      hasErrors = true;
      all_members_defined = false;
    }

    int size_of_member;
    if (stmt->decl.num_pointers > 0) {
      size_of_member = 8;
      stmt->decl.size = size_of_member;
    } else {
      size_of_member = size_of_type(parser, stmt->decl.kind);
      stmt->decl.size = size_of_member;
    }

    if (size_of_member < 0) {
      uint64_t key = str_slice_to_uint64(stmt->decl.kind.string);
      IntTuple dependency = {.first = struct_name_key, .second = key};
      depgraph_add(&parser->struct_dependencies, dependency);
      all_members_defined = false;
    } else {
      size = offset_align(size, size_of_member);
      size += size_of_member;
    }

    astnodelist_push(&structure->structure.members, stmt);
  }

  size = offset_align(size, 4);

  if (all_members_defined) {
    structure->structure.members_all_defined = true;
    structure->structure.size = size;
    IntTuple dependency = {.first = struct_name_key, .second = 0};
    depgraph_add(&parser->struct_dependencies, dependency);
  }
}

AstNode *parse_struct(Parser *parser) {
  AstNode *node = astnode_structure_new();
  Token tok = lex_peek(&parser->lexer);
  node->kind = AST_STRUCT;

  if (tok.kind != TOK_KEYWORD_STRUCT) {
    astnode_unexpected_token(node, tok, "struct");
    return node;
  }

  tok = lex_next(&parser->lexer);

  if (tok.kind != TOK_IDENTIFIER) {
    astnode_unexpected_token(node, tok, "name of struct");
    return node;
  }

  node->structure.name = tok;

  tok = lex_next(&parser->lexer);

  if (tok.kind != TOK_BRACE_OPEN) {
    astnode_unexpected_token(node, tok, "brace open { of struct ");
    return node;
  }

  tok = lex_next(&parser->lexer);

  parse_struct_decl_stmts(parser, node);

  if (node->kind == AST_ERROR) {
    astnode_invalid_ast(node, node, "expected struct member variables", tok);
    return node;
  }

  tok = lex_peek(&parser->lexer);
  if (tok.kind != TOK_BRACE_CLOSE) {
    astnode_unexpected_token(node, tok, " close brace } of struct ");
  }

  tok = lex_next(&parser->lexer);

  if (tok.kind == TOK_SEMICOLON) {
    lex_next(&parser->lexer);
  }

  uint64_t structkey = str_slice_to_uint64(node->structure.name.string);

  AstNode *previousdef = ptr_bucket_get(&parser->struct_definitions, structkey);

  if (previousdef != NULL) {
    fprintf(stderr, "previous struct def\n");
    astnode_print(parser, stderr, previousdef, 0);
  }

  ptr_bucket_put(&parser->struct_definitions, structkey, node);

  return node;
}

AstNode *parse_any(Parser *parser) {
  Token first = lex_peekn(&parser->lexer, 0);

  // skip empty statements
  // (also maybe caused by previous errors)
  while (first.kind == TOK_SEMICOLON) {
    first = lex_next(&parser->lexer);
  }

  Token second = lex_peekn(&parser->lexer, 1);
  Token third = lex_peekn(&parser->lexer, 2);
  Token fourth = lex_peekn(&parser->lexer, 3);

  // parse struct
  if (first.kind == TOK_KEYWORD_STRUCT) {
    return parse_struct(parser);
  }

  if (tok_is_maybe_type(first)) {

    // parse function
    if ((second.kind == TOK_IDENTIFIER || third.kind == TOK_IDENTIFIER) &&
        (third.kind == TOK_PAREN_OPEN || fourth.kind == TOK_PAREN_OPEN)) {


      Scope newscope;
      scope_init(&newscope);
      newscope.parent = &parser->scope;

      AstNode *proto = parse_func_proto(parser);
      if (proto->kind == AST_ERROR) {

        first = lex_next(&parser->lexer);
        while (first.kind != TOK_EOF && first.kind != TOK_SEMICOLON &&
               first.kind != TOK_BRACE_CLOSE && first.kind != TOK_BRACE_OPEN) {

          first = lex_next(&parser->lexer);
        }
      }

      Token tok = lex_peek(&parser->lexer);
      if (tok.kind != TOK_BRACE_OPEN) {
        return proto;
      }

      AstNode *function = astnode_new();
      function->kind = AST_FUNC_DEF;
      function->func.prototype = proto;
      function->func.block = parse_stmt_block(parser);
      scope_quit(&newscope);
      ptr_bucket_put(&parser->function_definitions, str_slice_to_uint64(proto->funcproto.name.string), function);

      return function;
    }

    return parse_decl(parser, 1);
  }

  AstNode *node = astnode_new();
  astnode_unexpected_token(node, first, "didn't know what to parse here");

  first = lex_next(&parser->lexer);
  while (first.kind != TOK_SEMICOLON && first.kind != TOK_PAREN_CLOSE &&
         first.kind != TOK_EOF) {
    first = lex_next(&parser->lexer);
  }
  return node;
}

AstNode *parse_program(Parser *parser) {
  Token tok = lex_peek(&parser->lexer);
  AstNode *node = astnode_new();
  node->kind = AST_PROGRAM;
  astnodelist_init(&node->program.items);
  while (tok.kind != TOK_EOF) {
    AstNode *item = parse_any(parser);
    astnodelist_push(&node->program.items, item);
    tok = lex_peek(&parser->lexer);
  }
  return node;
}

void astnode_print(Parser *parser, FILE *file, AstNode *node, int indent) {
  if (node == NULL) {
    fprintf(file, "%*snull\n", indent, "");
    return;
  }

  static const char *token_names[] = {FOREACH_TOKENKIND(GENERATE_STRING)};

  switch (node->kind) {
  case AST_BREAK:
    fprintf(file, "%*sbreak\n", indent, "");
    break;
  case AST_IF_ELSE:
    fprintf(file, "%*sif\n", indent, "");
    astnode_print(parser, file, node->ifelse.condition, indent + 2);
    astnode_print(parser, file, node->ifelse.ifblock, indent + 2);
    astnode_print(parser, file, node->ifelse.elseblock, indent + 2);
    break;
  case AST_FORLOOP:
    fprintf(file, "%*sfor\n", indent, "");
    astnode_print(parser, file, node->forloop.init, indent + 2);
    astnode_print(parser, file, node->forloop.condition, indent + 2);
    astnode_print(parser, file, node->forloop.step, indent + 2);
    astnode_print(parser, file, node->forloop.stmt, indent + 2);
    break;
  case AST_RETURN: {
    fprintf(file, "%*sreturn \n", indent, "");
    astnode_print(parser, file, node->ret.expr, indent + 2);
    break;
  }
  case AST_FUNC_CALL: {
    String func_name = parser_token_content(parser, node->funccall.name);
    fprintf(file, "%*scall %.*s \n", indent, "", func_name.len, func_name.data);

    for (int i = 0; i < node->funccall.params.len; i++) {
      astnode_print(parser, file, node->funccall.params.nodes[i], indent + 4);
    }
    break;
  }
  case AST_PROGRAM:
    fprintf(file, "%*sprogram \n", indent, "");
    for (int i = 0; i < node->program.items.len; i++) {
      astnode_print(parser, file, node->program.items.nodes[i], indent + 4);
    }
    break;
  case AST_FUNC_DEF:
    fprintf(file, "%*sfunc \n", indent, "");
    astnode_print(parser, file, node->func.prototype, indent + 2);
    astnode_print(parser, file, node->func.block, indent + 2);
    break;
  case AST_FUNC_PROTO: {
    String func_proto = parser_token_content(parser, node->funcproto.name);
    String ret = parser_token_content(parser, node->funcproto.retType);
    fprintf(file, "%*sfuncproto %.*s\n", indent, "", func_proto.len,
            func_proto.data);
    fprintf(file, "%*srettype %.*s\n", indent + 2, "", ret.len, ret.data);
    for (int i = 0; i < node->funcproto.params.len; i++) {
      astnode_print(parser, file, node->funcproto.params.nodes[i], indent + 4);
    }
    break;
  }
  case AST_STRUCT: {
    String structure = parser_token_content(parser, node->structure.name);
    fprintf(file, "%*sstruct %.*s (size:%d)\n", indent, "", structure.len,
            structure.data, node->structure.size);

    fprintf(file, "%*sdecl members \n", indent + 2, "");
    for (int i = 0; i < node->structure.members.len; i++) {
      astnode_print(parser, file, node->structure.members.nodes[i], indent + 4);
    }
    break;
  }

  case AST_BLOCK: {
    fprintf(file, "%*sblock \n", indent, "");
    fprintf(file, "%*sstatements \n", indent + 2, "");
    for (int i = 0; i < node->block.len; i++) {
      astnode_print(parser, file, node->block.nodes[i], indent + 4);
    }
    break;
  }
  case AST_LITERAL: {
    String literal = parser_token_content(parser, node->literal);
    fprintf(file, "%*sliteral int %.*s \n", indent, "", literal.len,
            literal.data);
    break;
  }

  /* case AST_ASSIGN: { */
  /*   fprintf(file, "%*sassign \n", indent, ""); */
  /*   astnode_print(parser, file, node->assign.var, indent + 2); */
  /*   astnode_print(parser, file, node->assign.expr, indent + 2); */
  /*   break; */
  /* } */

  case AST_DECL: {
    String decl = parser_token_content(parser, node->decl.name);

    fprintf(file, "%*sdecl \n", indent, "");
    fprintf(file, "%*sname: %.*s numpointers: %d size:%d \n", indent + 2, "",
            decl.len, decl.data, node->decl.num_pointers, node->decl.size);
    if (node->decl.expr != NULL) {
      astnode_print(parser, file, node->decl.expr, indent + 4);
    }
    break;
  }

  case AST_MEMBER_ACCESS: {
    String var = parser_token_content(parser, node->member.name);
    fprintf(file, "%*smember %.*s \n", indent, "", var.len, var.data);
    astnode_print(parser, file, node->member.next, indent+2);
    break;
  }

  case AST_VAR: {
    String var = parser_token_content(parser, node->var.name);
    fprintf(file, "%*svar %.*s \n", indent, "", var.len, var.data);

    astnode_print(parser, file, node->var.member_access, indent+2);

    /* AstNode *member = node->var.member_access; */
    /* while(member && member->kind == AST_MEMBER_ACCESS){ */
    /*   String str = parser_token_content(parser, member->member.name); */
    /*   fprintf(file, "%*s . %.*s \n", indent+2, "", str.len, str.data); */
    /*   member = member->member.next; */
    /* } */

    // fprintf(file, "%*spreviousdecls: \n", indent + 2, "");
    // var_print_declchain(file, node->var.decl_chain, parser, indent + 4);
    break;
  }

  case AST_BINOP: {
    String binop = parser_token_content(parser, node->binop.op);
    fprintf(file, "%*sBinOp %.*s \n", indent, "", binop.len, binop.data);
    astnode_print(parser, file, node->binop.left, indent + 2);
    astnode_print(parser, file, node->binop.right, indent + 2);
    break;
  }

  case AST_UNOP: {
    String unop = parser_token_content(parser, node->unop.op);
    fprintf(file, "%*sUnOp %.*s \n", indent, "", unop.len, unop.data);
    astnode_print(parser, file, node->unop.expr, indent + 2);
    break;
  }

  case AST_ERROR:
    switch (node->error.kind) {
    case ERR_UNINITIALIZED_NODE:
      fprintf(file, "%*serror uninitialized node\n", indent, "");
      break;
    case ERR_UNEXPECTED_TOKEN: {

      String actual =
          parser_token_content(parser, node->error.unexpectedToken.actual);

      fprintf(file, "%*serror unexpected token\n", indent, "");
      fprintf(file, "%*s%d:%d got %.*s (%s) \n", indent + 2, "",
              node->error.unexpectedToken.actual.line,
              node->error.unexpectedToken.actual.col, actual.len, actual.data,
              token_names[node->error.unexpectedToken.actual.kind]);

      fprintf(file, "%*sand I tried to parse %s \n", indent + 2, "",
              node->error.unexpectedToken.triedToParse);
      break;
    }

    case ERR_INVALID_AST: {

      String context =
          parser_token_content(parser, node->error.invalid.context_token);

      fprintf(file, "%*serror invalid ast\n", indent, "");
      fprintf(file, "%*s%d:%d %.*s %s\n", indent + 2, "",
              node->error.invalid.context_token.line,
              node->error.invalid.context_token.col, context.len, context.data,
              token_names[node->error.invalid.context_token.kind]);
      fprintf(file, "%*sdescription: %s \n", indent + 2, "",
              node->error.invalid.description);
      astnode_print(parser, file, node->error.invalid.node, indent + 2);
      break;
    }
    default:
      fprintf(file, "%*serror print not yet implemented\n", indent, "");
      break;
    }
    break;
  default:
    fprintf(file, "%*sastnode print not yet implemented\n", indent, "");
  }

  const char *type_names[] = {FOREACH_TYPE_KIND(GENERATE_STRING)};
  if (node->type != NULL) {
    if (node->type->kind == TYPE_ERROR) {
      fprintf(file, "%*stype error: %s\n", indent, "", node->type->error);
    } else {
      fprintf(file, "%*stype %s\n", indent, "",
      type_names[node->type->kind]);
    }
  }
}

int parser_init(Parser *parser) {
  parser->root = NULL;
  int status = 0;
  status = lex_init(&parser->lexer);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize lexer \n");
    return -1;
  }
  status = str_interner_init(&parser->pool, 128);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize string interner \n");
    return -1;
  }
  status = ptr_bucket_init(&parser->struct_definitions, 2);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize struct definition bucket \n");
    return -1;
  }
  status = ptr_bucket_init(&parser->function_definitions, 2);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize function definition bucket \n");
    return -1;
  }
  status = depgraph_init(&parser->struct_dependencies);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize struct dependency graph\n");
    return -1;
  }
  status = scope_init(&parser->scope);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize scoped declarations bucket \n");
    return -1;
  }
  status = vartable_init(&parser->assembly_variables);
  if(status < 0){
    fprintf(stderr, "couldnt initialize variable table for assembly generation\n");
    return -1;
  }
  return status;
}


void parser_quit(Parser *parser) {
  if (parser->root != NULL) {
    astnode_free(parser->root);
    free(parser->root);
  }
  scope_quit(&parser->scope);
  depgraph_quit(&parser->struct_dependencies);
  ptr_bucket_quit(&parser->struct_definitions);
  ptr_bucket_quit(&parser->function_definitions);
  str_interner_quit(&parser->pool);
  lex_quit(&parser->lexer);
  vartable_quit(&parser->assembly_variables);
}

int parser_resolve_missing_structs(Parser *parser) {

  depgraph_prepare(&parser->struct_dependencies);
  //depgraph_print(&parser->struct_dependencies);

  uint64_t to_resolve = depgraph_resolve(&parser->struct_dependencies);

  AstNode *circular_decl;

  while (to_resolve != DEPGRAPH_EMPTY) {
    StrSlice slice = str_slice_from_uint64(to_resolve);
    String str = str_interner_get(&parser->pool, slice);

    // skip those that are already resolved
    AstNode *def = ptr_bucket_get(&parser->struct_definitions, to_resolve);
    assert(def->kind == AST_STRUCT);
    if (def->structure.members_all_defined) {
      to_resolve = depgraph_resolve(&parser->struct_dependencies);
      continue;
    }

    bool all_defined = true;
    int size = 0;
    for (int i = 0; i < def->structure.members.len; i++) {
      AstNode *member_decl = def->structure.members.nodes[i];
      assert(member_decl->kind == AST_DECL);
      int member_size = size_of_type(parser, member_decl->decl.kind);
      member_decl->decl.size = member_size;
      if (member_size < 0) {
        all_defined = false;
        circular_decl = member_decl;
        break;
      }
      size = offset_align(size, member_size);
      size += member_size;
    }
    if (all_defined) {
      size = offset_align(size, 4);
    } else {
      size = -1;
      fprintf(stderr, "circular dependency:\n");
      astnode_print(parser, stderr, circular_decl, 0);
    }
    def->structure.members_all_defined = all_defined;
    def->structure.size = size;
    to_resolve = depgraph_resolve(&parser->struct_dependencies);
  }

  return 0;
}

Type *new_type(){
  Type *result = malloc(sizeof(Type));
  result->kind = TYPE_UNRESOLVED;
  result->definition = NULL;
  result->num_pointers = 0;
  result->error = NULL;
  return result;
}

Type *copy_type(Type *c){
  Type *result = new_type();
  result->kind = c->kind;
  result->definition = c->definition;
  result->num_pointers = c->num_pointers;
  result->error = c->error;
  return result;
}

Type *literal_to_type(AstNode *node) {
  assert(node->kind == AST_LITERAL);
  Type *t = new_type();
  switch (node->literal.kind) {
  case TOK_LITERAL_INT:
    t->kind = TYPE_INT;
    break;
  default:
    t->kind = TYPE_ERROR;
    t->error = "unsupported type";
    break;
  }
  return t;
}

Type *funcproto_to_type(Parser *parser, AstNode *node){
  assert(node->kind == AST_FUNC_PROTO);
  Type *t = new_type();

  switch(node->funcproto.retType.kind){
  case TOK_KEYWORD_INT:
    t->kind = TYPE_INT;
    return t;

  case TOK_IDENTIFIER: {
    uint64_t decltypekey = str_slice_to_uint64(node->funcproto.retType.string);
    AstNode *structdef =
        ptr_bucket_get(&parser->struct_definitions, decltypekey);
    if (structdef == NULL) {
      t->kind = TYPE_ERROR;
      t->error = "structure undefined";
      return t;
    }
    t->kind = TYPE_FUNC;
    t->definition = structdef;
    return t;
    break;
  }

  default:
    t->kind = TYPE_ERROR;
    t->error = "return type not supported";
    return t;
  }
}

Type *decl_to_type(Parser *parser, AstNode *node){
  assert(node->kind == AST_DECL);
  Type *t = new_type();
  t->num_pointers = node->decl.num_pointers;
  if (node->decl.kind.kind == TOK_KEYWORD_INT) {
    t->kind = TYPE_INT;
    return t;
  }
  if(node->decl.kind.kind == TOK_KEYWORD_CHAR){
    t->kind = TYPE_CHAR;
    return t;
  }

  if (node->decl.kind.kind == TOK_IDENTIFIER) {
    uint64_t decltypekey = str_slice_to_uint64(node->decl.kind.string);
    AstNode *structdef =
        ptr_bucket_get(&parser->struct_definitions, decltypekey);
    if (structdef == NULL) {
      t->kind = TYPE_ERROR;
      t->error = "structure undefined";
      return t;
    }
    node->decl.size = structdef->structure.size;

    t->kind = TYPE_STRUCT;
    t->definition = structdef;
    return t;
  }
  t->kind = TYPE_ERROR;
  t->error = "unsupported type declaration";
  return t;
}

void resolve_var_to_type(Parser *parser, AstNode *node) {
  assert(node->kind == AST_VAR);

  // variable needs to be declared
  // to retrieve the type
  AstNode *decl = node->var.declaration;
  if (decl == NULL) {
    node->type = new_type();
    node->type->kind = TYPE_ERROR;
    node->type->error = "undeclared type";
    return;
  }

  // if the variable has no member
  // access after it then it's just
  // the declared type
  if (node->var.member_access == NULL) {
    node->type = copy_type(decl->type);
    return;
  }

  // if it has a member access after
  // it but according to the declaration
  // is not a struct it is an error
  if (decl->type->kind != TYPE_STRUCT) {
    node->type = new_type();
    node->type->kind = TYPE_ERROR;
    node->type->error = "member access for something that is not a struct\n";
    return;
  }

  AstNode *member_access = node->var.member_access;
  AstNode *current_struct_definition = decl->type->definition;

  // it must be a struct so there must
  // be a definition for the struct
  /* if (current_struct_definition == NULL) { */
  /*   node->type = new_type(); */
  /*   node->type->kind = TYPE_ERROR; */
  /*   node->type->error = "declared variable is missing a struct definition"; */
  /*   return; */
  /* } */

  Type *t = NULL;
  Type *last = NULL;
  do {

    // for every member access the previous struct
    // needs to have the defnition that contains the member
    if(current_struct_definition == NULL){
      member_access->type = new_type();
      member_access->type->kind = TYPE_ERROR;
      member_access->type->error = "struct definition of member is missing";
      t = copy_type(member_access->type);
      break;
    }

    assert(member_access->kind == AST_MEMBER_ACCESS);
    assert(current_struct_definition->kind == AST_STRUCT);

    // find the member declaration
    // within the struct definition
    // if there isn't any its an error
    AstNode *struct_member_decl = NULL;
    for (int i = 0; i < current_struct_definition->structure.members.len; i++) {
      AstNode *member_of_struct =
          current_struct_definition->structure.members.nodes[i];
      uint64_t memofstructkey =
          str_slice_to_uint64(member_of_struct->decl.name.string);
      uint64_t memaccesskey =
          str_slice_to_uint64(member_access->member.name.string);
      if (memaccesskey == memofstructkey) {
        struct_member_decl = member_of_struct;
        break;
      }
    }

    if (struct_member_decl == NULL) {
      member_access->type = new_type();
      member_access->type->kind = TYPE_ERROR;
      member_access->type->error =
          "struct has no corresponding member with that name";
      t = copy_type(member_access->type);
      break;
    }

    // now we can determine the
    // type of this member
    member_access->type = last = decl_to_type(parser, struct_member_decl);
    member_access = member_access->member.next;
    current_struct_definition = ptr_bucket_get(&parser->struct_definitions, str_slice_to_uint64(struct_member_decl->decl.kind.string));
  } while(member_access);

  if(t == NULL){
    t = copy_type(last);
  }

  node->type = t;
  return;
}

Type *type_expect_same(Type *t1, Type *t2){
  if(t1->kind == TYPE_ERROR){
    Type *t = new_type();
    t->kind = TYPE_ERROR;
    t->error = "expected same types but left one has an error";
    return t;
  }

  if(t2->kind == TYPE_ERROR){
    Type *t = new_type();
    t->kind = TYPE_ERROR;
    t->error = "expected same types but right one has an error";
    return t;
  }

  if(t1->num_pointers != t2->num_pointers){
    Type *t = new_type();
    t->kind = TYPE_ERROR;
    t->error = "types don't have different amount of pointer indirections";
    return t;
  }

  if(t1->kind != t2->kind){
    Type *t = new_type();
    t->kind = TYPE_ERROR;
    t->error = "types don't match";
    return t;
  }

  if(t1->kind == TYPE_STRUCT){
    Type *t = new_type();
    if(t1->definition == NULL){
      t->kind = TYPE_ERROR;
      t->error = "first struct definition missing";
      return t;
    }
    if(t2->definition == NULL){
      t->kind = TYPE_ERROR;
      t->error = "seond struct definition missing";
      return t;
    }
    uint64_t namekey1 = str_slice_to_uint64(t1->definition->structure.name.string);
    uint64_t namekey2 = str_slice_to_uint64(t2->definition->structure.name.string);

    if(namekey1 != namekey2){
      t->kind = TYPE_ERROR;
      t->error = "different structures";
      return t;
    }
  }

  Type *t = copy_type(t1);
  return t;
}

/* int var_member_size(Parser *parser, AstNode *node){ */
/*   assert(node != NULL); */
/*   assert(node->kind == AST_VAR); */

/*   if(node->var.member_access == NULL){ */
/*     assert(node->var.declaration != NULL); */
/*     assert(node->var.declaration->kind == AST_DECL); */
/*     return size_of_type(parser, node->var.declaration->decl.kind); */
/*   } */

/*   AstNode *member_ */

/* } */

/* int var_struct_member_offset(Parser *parser, AstNode *structure, AstNode *member_access, int current_offset){ */
/*   assert(structure != NULL); */
/*   assert(member_access != NULL); */
/*   assert(structure->kind == AST_STRUCT); */
/*   assert(member_access->kind == AST_MEMBER_ACCESS); */

/*   /\* uint64_t member_name = member_access->ki *\/ */


/* } */

int var_member_offset(Parser *parser, AstNode *node, int *last_member_size) {
  assert(node != NULL);
  assert(node->kind == AST_VAR);
  assert(node->var.declaration != NULL);

  *last_member_size = 0;

  if (node->var.member_access == NULL) {
    *last_member_size = size_of_type(parser, node->var.declaration->decl.kind);
    return 0;
  }

  if (!node->var.declaration->type) {
    printf("no type\n");
    *last_member_size = size_of_type(parser, node->var.declaration->decl.kind);
    return 0;
  }

  if (node->var.declaration->type->kind != TYPE_STRUCT) {
    printf("var member offset: type not struct\n");
    *last_member_size = size_of_type(parser, node->var.declaration->decl.kind);
    return 0;
  }


  AstNode *member_access = node->var.member_access;
  AstNode *declaration = ptr_bucket_get(&parser->struct_definitions,
                                        str_slice_to_uint64(node->var.declaration->decl.kind.string));

  int offset = 0;
  uint64_t current_member_name;
  int last_decl_size = 0;

  current_member_name = str_slice_to_uint64(member_access->member.name.string);

  while (declaration && member_access) {
    bool found_member = false;
    AstNode *member_decl = NULL;

    // find member variable
    // with the name
    int i=0;
    for (; i < declaration->structure.members.len; i++) {
      member_decl = declaration->structure.members.nodes[i];
      uint64_t member_name = str_slice_to_uint64(member_decl->decl.name.string);

      if (member_decl->decl.size <= 0) {
        printf("ooooohhh member size was negative or zero");
      }

      if (member_name == current_member_name) {
        found_member = true;
        *last_member_size = member_decl->decl.size;
        break;
      }

      offset = offset_align(offset, member_decl->decl.size);
      offset += member_decl->decl.size;
    }

    if (!found_member) {
      fprintf(stderr, "member not found\n");
      break;
    }

    declaration =
        ptr_bucket_get(&parser->struct_definitions,
                       str_slice_to_uint64(member_decl->decl.kind.string));
    member_access = member_access->member.next;
    if(!member_access){
      break;
    }

    current_member_name = str_slice_to_uint64(member_access->member.name.string);
  }

  return offset;
}

void parser_resolve_types(Parser *parser, AstNode *node, AstNode *current_func_node) {
  if (!node) {
    return;
  }
  if(node->type){
    return;
  }

  const char *type_names[] = {FOREACH_TYPE_KIND(GENERATE_STRING)};
  switch (node->kind) {
  case AST_LITERAL: {
    node->type = literal_to_type(node);
    break;
  }
  case AST_STRUCT: {
    break;
  }
  case AST_DECL: {
    node->type = decl_to_type(parser, node);
    parser_resolve_types(parser, node->decl.expr, current_func_node);
    break;
  }
  case AST_MEMBER_ACCESS: {
    break;
  }
  case AST_VAR:
    resolve_var_to_type(parser, node);
    /* node->type = var_to_type(parser, node); */
    break;
  case AST_PROGRAM: {
    for(int i=0; i<node->program.items.len; i++){
      parser_resolve_types(parser, node->program.items.nodes[i], current_func_node);
    }
    break;
  }
  case AST_FUNC_PROTO: {
    node->type = funcproto_to_type(parser, node);
    for(int i=0; i<node->funcproto.params.len; i++){
      parser_resolve_types(parser, node->funcproto.params.nodes[i], node);
    }
    break;
  }
  case AST_FUNC_DEF: {
    parser_resolve_types(parser, node->func.prototype, NULL);
    node->type = copy_type(node->func.prototype->type);
    parser_resolve_types(parser, node->func.block, node);
    break;
  }
  case AST_BLOCK: {
    for (int i = 0; i < node->block.len; i++) {
      parser_resolve_types(parser, node->block.nodes[i], current_func_node);
    }
    break;
  }
  case AST_IF_ELSE: {
    parser_resolve_types(parser, node->ifelse.condition, current_func_node);
    parser_resolve_types(parser, node->ifelse.ifblock, current_func_node);
    parser_resolve_types(parser, node->ifelse.elseblock, current_func_node);
    break;
  }
  case AST_RETURN: {
    parser_resolve_types(parser, node->ret.expr, current_func_node);

    if(current_func_node == NULL){
      node->type = new_type();
      node->type->kind = TYPE_ERROR;
      node->type->error = "return is not inside a function";
      break;
    }

    node->type = type_expect_same(current_func_node->type, node->ret.expr->type);
    if(node->type->kind == TYPE_ERROR){
      node->type->error = "return value doesn't match the signiture of the function";
    }
    break;
  }
  case AST_BREAK: {
    break;
  }
  case AST_FORLOOP: {
    parser_resolve_types(parser, node->forloop.init, current_func_node);
    parser_resolve_types(parser, node->forloop.condition, current_func_node);
    parser_resolve_types(parser, node->forloop.stmt, current_func_node);
    parser_resolve_types(parser, node->forloop.step, current_func_node);
    break;
  }

    /* case AST_ASSIGN: */
  /*   parser_resolve_types(parser, node->assign.var, current_func_node); */
  /*   parser_resolve_types(parser, node->assign.expr, current_func_node); */
  /*   node->type = type_expect_same(node->assign.var->type, node->assign.expr->type); */
  /*   break; */

  case AST_BINOP:
    parser_resolve_types(parser, node->binop.left, current_func_node);
    parser_resolve_types(parser, node->binop.right, current_func_node);
    node->type =
        type_expect_same(node->binop.left->type, node->binop.right->type);
    break;
  case AST_UNOP:
    parser_resolve_types(parser, node->unop.expr, current_func_node);
    node->type = copy_type(node->unop.expr->type);
    break;
  case AST_ERROR:
    break;
  case AST_FUNC_CALL: {
    AstNode *funcdef =
        ptr_bucket_get(&parser->function_definitions,
                       str_slice_to_uint64(node->funccall.name.string));

    if(!funcdef){
      node->type = new_type();
      node->type->kind = TYPE_ERROR;
      node->type->error = "no function found";
      return;
    }

    assert(funcdef->kind == AST_FUNC_DEF);

    AstNode *prototype = funcdef->func.prototype;

    if(prototype->funcproto.params.len != node->funccall.params.len){
      node->type = new_type();
      node->type->kind = TYPE_ERROR;
      node->type->error = "number of arguments not matching number of parameters";
      return;
    }

    for(int i=0; i<node->funccall.params.len; i++){

      // first resolve the parameter
      parser_resolve_types(parser,
                           node->funccall.params.nodes[i],
                           current_func_node);

      // check if the type matches
      Type *type_of_arg = node->funccall.params.nodes[i]->type;
      Type *type_of_param = prototype->funcproto.params.nodes[i]->type;
      if(!type_of_param){
        parser_resolve_types(parser, prototype->funcproto.params.nodes[i],
                             NULL);
        type_of_param = prototype->funcproto.params.nodes[i]->type;
      }

      Type *t = type_expect_same(type_of_arg, type_of_param);
      if(t->error){
        free(node->funccall.params.nodes[i]->type);
        node->funccall.params.nodes[i]->type = t;
        break;
      } else {
        free(t);
      }
    }

    node->type = copy_type(prototype->type);
  }
    break;
  }
}

bool collect_errors(Parser *parser, AstNode *node, String content, int fileid,
                    FileManager *manager) {
  assert(manager != NULL);
  assert(parser != NULL);
  if (!node) {
    return 0;
  }
  switch (node->kind) {
  case AST_ERROR: {
    String filename = filemanager_get_filename(manager, fileid);
    // get the last parsing error
    AstNode *last_error = node;
    AstNode *previous_error = node;
    while (1) {
      if (last_error->error.kind == ERR_INVALID_AST) {
        if (last_error->error.invalid.node == NULL) {
          Token context = last_error->error.invalid.context_token;
          String membername = parser_token_content(parser, context);
          fprintf(stderr, RED "%.*s:%d:%d" COLOR_RESET " parsing error " "%s \n", filename.len,
                  filename.data, context.line, context.col,
                  last_error->error.invalid.description);
          break;
        }
        previous_error = last_error;
        last_error = last_error->error.invalid.node;
      } else if (last_error->error.kind == ERR_UNEXPECTED_TOKEN) {

        Token actual = last_error->error.unexpectedToken.actual;
        String actual_str = parser_token_content(parser, actual);
        fprintf(stderr, RED "%.*s:%d:%d  " COLOR_RESET "unexpected token "  "%.*s but I expected %s \n",
                filename.len, filename.data,
                actual.line,
                actual.col,
                actual_str.len,
                actual_str.data,
                last_error->error.unexpectedToken.triedToParse);

        if (previous_error != last_error) {
          Token context = previous_error->error.invalid.context_token;
          String context_str = parser_token_content(parser, context);
          fprintf(stderr, "\t\t\t because %s at symbol %.*s (%d:%d) \n",
                  previous_error->error.invalid.description, context_str.len,
                  context_str.data,
                  previous_error->error.invalid.context_token.line,
                  previous_error->error.invalid.context_token.col);
        }

        break;
      } else {
        // neither invalid ast
        // nor unexpected token
        // so there is nothing to print
        // regarding parsing
        break;
      }
    }

    // has by definition errors
    return true;
    break;
  }
  case AST_PROGRAM: {
    bool hasErrors = false;
    for (int i = 0; i < node->program.items.len; i++) {
      hasErrors |= collect_errors(parser, node->program.items.nodes[i], content,
                                  fileid, manager);
    }
    return hasErrors;
    break;
  }
  case AST_BLOCK: {
    bool hasErrors = false;
    for (int i = 0; i < node->program.items.len; i++) {
      hasErrors |= collect_errors(parser, node->block.nodes[i], content, fileid,
                                  manager);
    }
    return hasErrors;
    break;
  }
  case AST_LITERAL:
    // as this is the last
    // node in the hierarchy
    // it cannot be an error
    return false;
    break;
  case AST_STRUCT: {
    String filename = filemanager_get_filename(manager, fileid);
    String structname = parser_token_content(parser, node->structure.name);
    bool hasErrors = false;
    for (int i = 0; i < node->structure.members.len; i++) {
      AstNode *member_declaration = node->structure.members.nodes[i];
      if (member_declaration->kind == AST_ERROR) {
        hasErrors = true;
        collect_errors(parser, member_declaration, content, fileid, manager);
      }
    }
    return hasErrors;
    break;
  }
  case AST_DECL: {
    return collect_errors(parser, node->decl.expr, content, fileid, manager);
    break;
  }
  case AST_MEMBER_ACCESS:
  case AST_VAR:
  case AST_FUNC_PROTO:
    break;
  case AST_FUNC_DEF: {
    bool hasErrors = false;
    hasErrors |= collect_errors(parser, node->func.prototype, content, fileid, manager);
    hasErrors |= collect_errors(parser, node->func.block, content, fileid, manager);
    return hasErrors;
  }
  case AST_FUNC_CALL:
    break;
  case AST_IF_ELSE: {
    bool hasErrors = false;
    hasErrors |= collect_errors(parser, node->ifelse.condition, content, fileid,
                                manager);

    hasErrors |= collect_errors(parser, node->ifelse.ifblock, content, fileid,
                                manager);

    hasErrors |= collect_errors(parser, node->ifelse.elseblock, content, fileid,
                                manager);
    return hasErrors;
    break;
  }
  case AST_RETURN:
  case AST_BREAK:
    break;
  case AST_BINOP: {
    return collect_errors(parser, node->binop.left, content, fileid, manager);
    return collect_errors(parser, node->binop.right, content, fileid, manager);
    break;
  }
  case AST_UNOP: {
    return collect_errors(parser, node->unop.expr, content, fileid, manager);
    break;
  }
  case AST_FORLOOP: {
    bool hasErrors = false;
    hasErrors |=
        collect_errors(parser, node->forloop.init, content, fileid, manager);
    hasErrors |= collect_errors(parser, node->forloop.condition, content,
                                fileid, manager);
    hasErrors |=
        collect_errors(parser, node->forloop.stmt, content, fileid, manager);
    hasErrors |=
        collect_errors(parser, node->forloop.step, content, fileid, manager);
    return hasErrors;
    break;
  }
  }

  return false;
}

void parser_node_print(Parser *parser);

void parser_parse(Parser *parser, String content, int fileid, FileManager *manager) {
  lex(&parser->lexer, content, fileid);
  tokens_internalize(&parser->lexer.tokens, content, &parser->pool);
  parser->root = parse_program(parser);

  /* parser_node_print(parser); */
  parser->hasErrors = collect_errors(parser, parser->root, content, fileid, manager);
  if(!parser->hasErrors){

    parser_resolve_missing_structs(parser);
    parser_resolve_types(parser, parser->root, NULL);
  }
}

void parser_depgraph_print(Parser *parser) {
  printf("----------- depgraph --------\n");
  printf("--- unresolved: ---\n");
  for (int i = 0; i < parser->struct_dependencies.dependencies.len; i++) {
    if (i == parser->struct_dependencies.resolved_i) {
      printf("---to be resolved: ----\n");
    }
    IntTuple tuple = parser->struct_dependencies.dependencies.data[i];
    String first =
        str_interner_get(&parser->pool, str_slice_from_uint64(tuple.first));
    String second =
        str_interner_get(&parser->pool, str_slice_from_uint64(tuple.second));
    str_file_print(first, stdout);
    printf("\t");
    str_file_print(second, stdout);
    printf("\n");
  }
}

void parser_structdef_print(Parser *parser) {
  printf("-----------structdefs --------\n");
  for (int i = 0; i < parser->struct_definitions.cap; i++) {
    PtrBucketEntry entry = parser->struct_definitions.data[i];
    if (entry.key) {
      astnode_print(parser, stdout, entry.data, 0);
      printf("\n");
    }
  }
}

void parser_node_print(Parser *parser) {
  printf("astnode: \n");
  if (parser->root != NULL) {
    astnode_print(parser, stdout, parser->root, 2);
  } else {
    printf("  null \n");
  }
}

void parser_print(Parser *parser) {
  parser_node_print(parser);
  parser_structdef_print(parser);
  parser_depgraph_print(parser);
}

void parser_dump_assembly_program(Parser *parser, AstNode *node, FILE *file,
                                  int indent, AstNode *currentfunc,
                                  int *stack_offset);

void parser_dump_assembly(Parser *parser, FILE *file) {
  int stack_offset = 0;
  parser_dump_assembly_program(parser, parser->root, file, 0, NULL,
                               &stack_offset);
}

String parser_token_content(Parser *parser, Token tok) {
  return str_interner_get(&parser->pool, tok.string);
}

#include "compiler.h"
#include <stdlib.h>



void free_expr(AstExpr *expr){
  if(expr == NULL){return;}
  switch (expr->kind) {

  case EXPR_CONST:
    break;

  case EXPR_UNOP:
    free_expr(expr->unop.expr);
    break;

  case EXPR_BINOP:
    free_expr(expr->binop.left);
    free_expr(expr->binop.right);
    break;

  case EXPR_ASSIGN:
    free_expr(expr->assign.expr);
    break;

  case EXPR_VAR:
    break;
  }
}

void free_stmt(AstBlockItem *stmt){
  if(stmt == NULL){return;}
  switch (stmt->kind) {

  case BLOCK_ITEM_COMPOUND:
    free_stmt_list(stmt->compound.blockitems);
    break;

  case BLOCK_ITEM_IFELSE:
    free_expr(stmt->ifelse.condition);
    free_stmt(stmt->ifelse.truestmt);
    free_stmt(stmt->ifelse.elsestmt);
    break;

  case BLOCK_ITEM_RETURN:
    free_expr(stmt->expr);
    break;

  case BLOCK_ITEM_EXPR:
    free(stmt->expr);
    break;

  case BLOCK_ITEM_DECL:
    free_expr(stmt->decl.option);
    break;
  }
}

void free_stmt_list(AstBlockItemList list) {
  for (int i = 0; i < list.len; i++) {
    free_stmt(&list.data[i]);
  }
}

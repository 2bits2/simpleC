#include "compiler.h"

void print_expr(AstExpr *expr, String *buffer, int indent) {
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
  case EXPR_ASSIGN:
    printf("%*sassign \n", indent, "");
    printf("%*s  id: %.*s \n", indent, "", expr->assign.name.len,
           buffer->data + expr->assign.name.start);
    print_expr(expr->assign.expr, buffer, indent + 2);
  case EXPR_VAR:
    printf("%*svar %.*s\n", indent, "", expr->var.len, buffer->data + expr->var.start );
    break;
  }
}

void print_block_item(AstBlockItem *stmt, String *buffer, int indent){
  if(stmt == NULL){
    printf("%*snull \n", indent, "");
  }

  switch(stmt->kind){
  case BLOCK_ITEM_IFELSE:
    printf("%*sif \n", indent, "");
    printf("%*scondition: \n", indent+2, "");
    print_expr(stmt->ifelse.condition, buffer, indent + 4);

    printf("%*sifblock: \n", indent+2, "");
    print_block_item(stmt->ifelse.truestmt, buffer, indent + 4);

    if(stmt->ifelse.elsestmt != NULL){
      printf("%*selseblock: \n", indent+2, "");
      print_block_item(stmt->ifelse.elsestmt, buffer, indent + 4);
    }
    break;

  case BLOCK_ITEM_COMPOUND: {

    printf("%*scompound{ \n", indent, "");

    for(int i=0; i<stmt->compound.blockitems.len; i++){
      print_block_item(&stmt->compound.blockitems.data[i], buffer, indent+2);
    }

    printf("%*s} \n", indent, "");

    break;
  }
  case BLOCK_ITEM_RETURN:
    printf("%*sreturn \n", indent, "");
    print_expr(stmt->expr, buffer, indent + 2);
    break;
  case BLOCK_ITEM_EXPR:
    printf("%*sexpr \n", indent, "");
    print_expr(stmt->expr, buffer, indent + 2);
    break;
  case BLOCK_ITEM_DECL:
    printf("%*sdecl \n", indent, "");
    printf("%*sname: %.*s\n", indent + 2, "", stmt->decl.name.len,
           buffer->data + stmt->decl.name.start);
    if (stmt->decl.option != NULL) {
      print_expr(stmt->decl.option, buffer, indent + 2);
    }
    break;
  }
}

void print_block_item_list(AstBlockItemList *list, String *buffer, int indent) {
  for (int i = 0; i < list->len; i++) {
    print_block_item(&list->data[i], buffer, indent);
  }
}

void print_fundef(AstFunDef *fundef, String *buffer, int indent) {
  if (fundef == NULL) {
    printf("%*sfundef NULL \n", indent, "");
    return;
  }
  printf("%*sfunction %.*s \n", indent, "", fundef->name.len,
         buffer->data + fundef->name.start);

  print_block_item_list(&fundef->stmts, buffer, indent);
  //print_expr(fundef->stmts->expr, buffer, indent+2);
}

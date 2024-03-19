#include "compiler.h"

void gencode_fundef(FILE *file, AstFunDef *fundef, String *buffer, VarMap *vars,
                    int lastVarId, int scopelevel);


void gencode_expr(FILE *file, AstExpr *expr, String *buffer, VarMap *vars, int lastVarId, int scope) {

  switch (expr->kind) {
  case EXPR_VAR: {

    int var_id = varmap_find_var(vars, lastVarId, expr->var, scope, buffer);
    VarMapEntry *var = varmap_get(vars, var_id);
    if(var == NULL){
      fprintf(stderr, "variable %.*s was not declared\n", expr->var.len, buffer->data + expr->var.start);
      return;
    }

    fprintf(file, "movl %d(%%ebp), %%eax\n", var->stack_offset);
    break;
  }

  case EXPR_ASSIGN: {
    int var_id = varmap_find_var(vars, lastVarId, expr->assign.name, scope, buffer);
    VarMapEntry *var = varmap_get(vars, var_id);
    if(var == NULL){
      fprintf(stderr, "variable %.*s was not declared\n", expr->var.len,
              buffer->data + expr->var.start);
      return;
    }
    gencode_expr(file, expr->assign.expr, buffer, vars, lastVarId, scope);
    fprintf(file, "movl %%eax, %d(%%ebp)\n", var->stack_offset);
    break;
  }
  case EXPR_BINOP:

    switch (expr->binop.kind) {
    case TOK_LOGICAL_OR: {
      static int d = 0;

      int label = d;
      d++;

      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "cmpl $0, %%eax\n");
      fprintf(file, "je _logicalorcheck%d\n", label);
      fprintf(file, "movl $1, %%eax\n");
      fprintf(file, "jmp _logicalorend%d\n", label);

      fprintf(file, "_logicalorcheck%d:\n", label);
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "cmpl $0, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setne %%al\n");
      fprintf(file, "_logicalorend%d:\n", label);
      break;
    }

    case TOK_LOGICAL_AND: {
      static int d = 0;
      int label = d;
      d++;

      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "cmpl $0, %%eax\n");
      fprintf(file, "jne _logicalandcheck%d\n", label);
      fprintf(file, "jmp _logicalandend%d\n", label);

      fprintf(file, "_logicalandcheck%d:\n", label);
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "cmpl $0, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setne %%al\n");

      fprintf(file, "_logicalandend%d:\n", label);

    } break;
    case TOK_PLUS:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "addl %%ecx, %%eax\n");
      break;

    case TOK_MINUS:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "subl %%ecx, %%eax\n");
      break;

    case TOK_MUL:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "imul %%ecx, %%eax\n");
      break;

    case TOK_DIV:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cdq\n");
      fprintf(file, "idivl %%ecx, %%eax\n");
      break;

    case TOK_LOGICAL_EQUAL:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "sete %%al\n");
      break;

    case TOK_LOGICAL_NOT_EQUAL:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setne %%al\n");
      break;

    case TOK_LOGICAL_GREATER_EQUAL:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setge %%al\n");
      break;

    case TOK_LOGICAL_GREATER:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setg %%al\n");
      break;

    case TOK_LOGICAL_LESS:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setl %%al\n");
      break;

    case TOK_LOGICAL_LESS_EQUAL:
      gencode_expr(file, expr->binop.left, buffer, vars, lastVarId, scope);
      fprintf(file, "push %%eax\n");
      gencode_expr(file, expr->binop.right, buffer, vars, lastVarId, scope);
      fprintf(file, "mov %%eax, %%ecx\n");
      fprintf(file, "pop %%eax\n");

      fprintf(file, "cmpl %%ecx, %%eax\n");
      fprintf(file, "movl $0, %%eax\n");
      fprintf(file, "setle %%al\n");
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
    gencode_expr(file, expr->unop.expr, buffer, vars, lastVarId, scope);
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

int gencode_stmt(FILE *file, AstBlockItem *stmt, String *buffer, VarMap *vars,
                 int lastVarId, int scopelevel);


void gencode_block_itemlist(FILE *file, AstBlockItemList *list, String *buffer,
                            VarMap *vars, int lastVarId, int scopelevel) {
  for (int i = 0; i < list->len; i++) {
    lastVarId =
        gencode_stmt(file, &list->data[i], buffer, vars, lastVarId, scopelevel);
  }
  int scope_size = varmap_scope_size(vars, lastVarId);
  fprintf(file, "addl $%d, %%esp\n", scope_size);
}

int gencode_ifelse(FILE *file, AstBlockItem *stmt, String *buffer, VarMap *vars,
                 int lastVarId, int scopelevel){
  static int labelgenerator = 0;

  int label = labelgenerator;
  labelgenerator++;

  gencode_expr(file, stmt->ifelse.condition, buffer, vars, lastVarId,
               scopelevel);
  fprintf(file, "cmpl $0, %%eax\n");
  fprintf(file, "je   _elseclause%d \n", labelgenerator);

  gencode_stmt(file, stmt->ifelse.truestmt, buffer, vars, lastVarId,
               scopelevel);
  fprintf(file, "jmp   _afterelse%d \n", labelgenerator);

  fprintf(file, "_elseclause%d:\n", labelgenerator);

  if (stmt->ifelse.elsestmt) {
    gencode_stmt(file, stmt->ifelse.elsestmt, buffer, vars, lastVarId,
                 scopelevel);
  }

  fprintf(file, "_afterelse%d:\n", labelgenerator);
  return lastVarId;
}

int gencode_declaration(FILE *file, AstBlockItem *stmt, String *buffer,
                        VarMap *vars, int lastVarId, int scopelevel){
  int var_id = varmap_new_id(vars);

  int maybeDeclaredId =
      varmap_find_var(vars, lastVarId, stmt->decl.name, scopelevel, buffer);

  VarMapEntry *maybeDeclaredVar = varmap_get(vars, maybeDeclaredId);
  if (maybeDeclaredVar && maybeDeclaredVar->scopelevel == scopelevel) {
    fprintf(stderr, "variable %.*s was already declared \n",
            stmt->decl.name.len, buffer->data + stmt->decl.name.start);
    return lastVarId;
  }

  VarMapEntry *lastVar = varmap_get(vars, lastVarId);
  int lastVarOffset = 0;
  if (lastVar == NULL) {
    lastVarOffset = 0;
  } else {
    lastVarOffset = lastVar->stack_offset;
  }

  VarMapEntry *var = varmap_get(vars, var_id);

  if (var == NULL) {
    fprintf(stderr, "couldnt generate code vor declaration of %.*s\n",
            stmt->decl.name.len, buffer->data + stmt->decl.name.start);
    return lastVarId;
  }

  /* int varsize = kind_size(stmt->decl.kind.kind); */
  //int newoffset = lastVarOffset - varsize

  var->stack_offset = lastVarOffset - 4;
  var->view = stmt->decl.name;
  var->prev = lastVarId;
  var->scopelevel = scopelevel;

  /* fprintf(file, "subl $%d, %%esp\n", ); */
  // fprintf(file, "push %%eax\n");

  fprintf(file, "pushl %%eax\n");
  if (stmt->decl.option != NULL) {
    gencode_expr(file, stmt->decl.option, buffer, vars, lastVarId, scopelevel);
  } else {
    fprintf(file, "movl $0, %%eax\n");
  }
  fprintf(file, "movl %%eax, %d(%%ebp)\n", var->stack_offset);
  return var_id;
}

int gencode_forloop(FILE *file, AstBlockItem *stmt, String *buffer,
                    VarMap *vars, int lastVarId, int scopelevel) {
  static int labelgenerator = 0;
  int label = labelgenerator;
  labelgenerator++;

  int newlastVarId = lastVarId;
  if (stmt->forloop.init != NULL) {
    newlastVarId = gencode_stmt(file, stmt->forloop.init, buffer, vars,
                                newlastVarId, scopelevel + 1);
  }

  fprintf(file, "_forcondition%d:\n", label);

  if (stmt->forloop.condition != NULL) {
    gencode_expr(file, stmt->forloop.condition, buffer, vars, newlastVarId,
                 scopelevel);
  } else {
    fprintf(file, "movl $1, %%eax\n");
  }

  fprintf(file, "cmpl $0, %%eax\n");
  fprintf(file, "je _forend%d\n", label);

  gencode_stmt(file, stmt->forloop.body, buffer, vars, newlastVarId,
               scopelevel + 1);

  gencode_expr(file, stmt->forloop.postExpr, buffer, vars, newlastVarId,
               scopelevel + 1);

  fprintf(file, "jmp _forcondition%d\n", label);
  fprintf(file, "_forend%d:\n", label);
  return lastVarId;
}

int gencode_return(FILE *file, AstBlockItem *stmt, String *buffer, VarMap *vars,
                   int lastVarId, int scopelevel){
  gencode_expr(file, stmt->expr, buffer, vars, lastVarId, scopelevel);
  fprintf(file, "movl %%ebp, %%esp \n");
  fprintf(file, "pop %%ebp\n");
  fprintf(file, "ret\n");
  return lastVarId;
}

int gencode_stmt(FILE *file, AstBlockItem *stmt, String *buffer, VarMap *vars,
                 int lastVarId, int scopelevel) {
  switch (stmt->kind) {
  case BLOCK_ITEM_RETURN:
    gencode_return(file, stmt, buffer, vars, lastVarId, scopelevel);
    break;
  case BLOCK_ITEM_EXPR:
    gencode_expr(file, stmt->expr, buffer, vars, lastVarId, scopelevel);
    break;
  case BLOCK_ITEM_IFELSE:
    gencode_ifelse(file, stmt, buffer, vars, lastVarId, scopelevel);
    break;
  case BLOCK_ITEM_COMPOUND: {
    gencode_block_itemlist(file, &stmt->compound.blockitems, buffer, vars,
                           lastVarId, scopelevel + 1);
    break;
  }
  case BLOCK_ITEM_DECL: {
    return gencode_declaration(file, stmt, buffer, vars, lastVarId, scopelevel);
    break;
  }
  case BLOCK_ITEM_FORLOOP: {
    return gencode_forloop(file, stmt, buffer, vars, lastVarId, scopelevel);
    break;
  }
  }
  return lastVarId;
}

void gencode_fundef(FILE *file, AstFunDef *fundef, String *buffer, VarMap *vars,
                    int lastVarId, int scopelevel) {
  fprintf(file, ".globl %.*s\n", fundef->name.len,
          buffer->data + fundef->name.start);

  fprintf(file, "%.*s:\n", fundef->name.len, buffer->data + fundef->name.start);

  // basepointer prolog
  fprintf(file, "push %%ebp\n");
  fprintf(file, "movl %%esp, %%ebp\n");

  gencode_block_itemlist(file, &fundef->stmts, buffer, vars, lastVarId,
                         scopelevel);
}

void gencode_file(FILE *file, Ast *ast, VarMap *vars, int lastVarId,
                  int scopelevel) {
  gencode_fundef(file, ast->fundef, &ast->charbuf, vars, -1, 0);
}

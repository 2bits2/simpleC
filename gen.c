#include "compiler.h"
#include <assert.h>

void parser_dump_assembly_program(Parser *parser, AstNode *node, FILE *file,
                                  int indent, AstNode *currentfunc,
                                  int *stack_offset) {

  if(parser->hasErrors){
    fprintf(file, "encountered error previously\n");
    return;
  }

  static const char *ast_names[] = {FOREACH_AST_KIND(GENERATE_STRING)};
  if (node == NULL) {
    return;
  }

  switch (node->kind) {
  case AST_PROGRAM: {

    fprintf(file, "%*s.text \n", indent, "");
    fprintf(file, "%*s.globl main \n", indent, "");

    /* vartable_checkpoint_get(&parser->assembly_variables); */

    for (int i = 0; i < node->program.items.len; i++) {
      int normal_stack_offset = 0;
      parser_dump_assembly_program(parser, node->program.items.nodes[i], file,
                                   indent, NULL, &normal_stack_offset);
    }
    break;
  }
  case AST_FUNC_DEF: {
    AstFuncPrototype *proto = &node->func.prototype->funcproto;
    String name = parser_token_content(parser, proto->name);

    fprintf(file, "%*s%.*s:\n", indent, "", name.len, name.data);

    // function preamble
    fprintf(file, "%*spushq %%rbp\n", indent + 2, "");
    fprintf(file, "%*smovq %%rsp, %%rbp\n", indent + 2, "");

    int normal_stack_offset = 0;

    for(int i=0; i<proto->params.len; i++){
      AstNode *decl = proto->params.nodes[i];

      parser_dump_assembly_program(parser, decl, file, indent, node,
    &normal_stack_offset);

    /*   // TODO: not supporting structs yet */
    /*   int var_offset = decl->decl.assembly_base_offset; */
    /*   int size = decl->decl.size; */
    /*   var_offset += size; */
    }

    for (int i = 0; i < node->func.block->block.len; i++) {
      parser_dump_assembly_program(parser, node->func.block->block.nodes[i],
                                   file, indent + 2, node,
                                   &normal_stack_offset);
    }
    /* // function exit */
    fprintf(file, "%*s%.*sexit:\n", indent + 2, "", name.len, name.data);
    fprintf(file, "%*smovq %%rbp, %%rsp\n", indent + 2, "");
    fprintf(file, "%*spopq %%rbp\n", indent + 2, "");
    fprintf(file, "%*sret\n", indent + 2, "");
    break;
  }
  case AST_RETURN: {
    if (currentfunc == NULL) {
      fprintf(stderr, "return should be placed inside a function\n");
      return;
    }
    parser_dump_assembly_program(parser, node->ret.expr, file, indent,
                                 currentfunc, stack_offset);
    AstFuncPrototype *proto = &currentfunc->func.prototype->funcproto;

    String name = parser_token_content(parser, proto->name);
    fprintf(file, "%*sjmp %.*sexit\n", indent, "", name.len, name.data);
    break;
  }
  case AST_LITERAL: {

    String literal = parser_token_content(parser, node->literal);
    if (node->literal.kind == TOK_LITERAL_INT) {
      fprintf(file, "%*smovl $%.*s, %%eax\n", indent, "", literal.len,
              literal.data);
      break;
    }

    fprintf(stderr, "literals not yet fully supported\n");

    break;
  }
  case AST_BINOP: {

    switch (node->binop.op.kind) {

    case TOK_LOGICAL_GREATER:
    case TOK_LOGICAL_EQUAL:
    case TOK_LOGICAL_GREATER_EQUAL:
    case TOK_LOGICAL_LESS_EQUAL:
    case TOK_LOGICAL_LESS: {
      parser_dump_assembly_program(parser, node->binop.left, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*spushq %%rax\n", indent, "");
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*smovq %%rax, %%rdx\n", indent, "");
      fprintf(file, "%*spopq %%rax\n", indent, "");
      fprintf(file, "%*scmpq %%rdx, %%rax\n", indent, "");

      if (node->binop.op.kind == TOK_LOGICAL_LESS)
        fprintf(file, "%*ssetl %%al\n", indent, "");
      else if (node->binop.op.kind == TOK_LOGICAL_LESS_EQUAL)
        fprintf(file, "%*ssetle %%al\n", indent, "");
      else if (node->binop.op.kind == TOK_LOGICAL_GREATER_EQUAL)
        fprintf(file, "%*ssetge %%al\n", indent, "");
      else if (node->binop.op.kind == TOK_LOGICAL_GREATER)
        fprintf(file, "%*ssetg %%al\n", indent, "");
      else if (node->binop.op.kind == TOK_LOGICAL_EQUAL)
        fprintf(file, "%*ssete %%al\n", indent, "");

      fprintf(file, "%*smovzbl %%al, %%eax\n", indent, "");
      break;
    }

    case TOK_MINUS:
      parser_dump_assembly_program(parser, node->binop.left, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*spushq %%rax\n", indent, "");
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*smovl %%eax, %%edx\n", indent, "");
      fprintf(file, "%*spopq %%rax\n", indent, "");
      fprintf(file, "%*ssubl %%edx, %%eax\n", indent, "");
      break;
    case TOK_PLUS:
      parser_dump_assembly_program(parser, node->binop.left, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*spushq %%rax\n", indent, "");
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*smovl %%eax, %%edx\n", indent, "");
      fprintf(file, "%*spopq %%rax\n", indent, "");
      fprintf(file, "%*saddl %%edx, %%eax\n", indent, "");
      break;
    case TOK_MUL:
      parser_dump_assembly_program(parser, node->binop.left, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*spushq %%rax\n", indent, "");
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*smovl %%eax, %%edx\n", indent, "");
      fprintf(file, "%*spopq %%rax\n", indent, "");
      fprintf(file, "%*simul %%edx, %%eax\n", indent, "");
      break;
    case TOK_DIV:
      parser_dump_assembly_program(parser, node->binop.left, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*spushq %%rax\n", indent, "");
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);
      fprintf(file, "%*smovl %%eax, %%ecx\n", indent, "");
      fprintf(file, "%*spopq %%rax\n", indent, "");
      fprintf(file, "%*scltd\n", indent, "");
      fprintf(file, "%*sidivl %%ecx \n", indent, "");
      break;

    case TOK_ASSIGN:
      if (node->binop.left->kind != AST_VAR) {
        fprintf(stderr, "can't assign to expression only to variable\n");
        return;
      }
      parser_dump_assembly_program(parser, node->binop.right, file, indent,
                                   currentfunc, stack_offset);

      int var_offset =
          node->binop.left->var.declaration->decl.assembly_base_offset;
      int member_size = 0;
      int member_offset =
          var_member_offset(parser, node->binop.left, &member_size);

      /* AstNode *structdef = ptr_bucket_get(&parser->struct_definitions,
       * str_slice_to_uint64(node->binop.left->var.declaration->decl.kind.string));
       */
      /* assert(structdef != NULL); */
      /* assert(structdef->kind == AST_STRUCT); */

      int size = size_of_type(parser, node->binop.left->var.declaration->decl
                                          .kind); // structdef->structure.size;

      /* printf("assign: size %d  var_offset %d  member_offset %d\n", size, */
      /*        var_offset, member_offset); */

      var_offset += (size - (member_offset));
      if (var_offset % member_size != 0) {
        var_offset = offset_align(var_offset - member_size, member_size);
      }

      if (member_size == 4) {
        fprintf(file, "%*smovl %%eax, -%d(%%rbp)\n", indent, "", var_offset);
      } else if (member_size == 8) {
        fprintf(file, "%*smovq %%rax, -%d(%%rbp) \n", indent, "", var_offset);
      } else if (member_size == 1) {
        /* fprintf(file, "%*smovsbl -%d(%%rbp), %%edx\n", indent, "",
         * var_offset); */
        fprintf(file, "%*smovb %%al, -%d(%%rbp)\n", indent, "", var_offset);
      } else {
        fprintf(stderr, "assign size %d not supported yet\n", member_size);
      }

      break;
    }
    break;
  }
  case AST_STRUCT:
    break;
  case AST_VAR: {
    int member_size = 0;
    int member_offset = var_member_offset(parser, node, &member_size);
    int var_offset = node->var.declaration->decl.assembly_base_offset;

    int size = 0;
    if (node->var.declaration->type->kind == TYPE_STRUCT) {
      AstNode *structdef = ptr_bucket_get(
          &parser->struct_definitions,
          str_slice_to_uint64(node->var.declaration->decl.kind.string));
      assert(structdef != NULL);
      assert(structdef->kind == AST_STRUCT);
      size = structdef->structure.size;
    } else {
      size = size_of_type(parser, node->var.declaration->decl.kind);
    }

    /* int size = structdef->structure.size; */
    /* printf("var offset %d member offset %d size %d \n", var_offset, */
    /* member_offset, size); */

    var_offset += (size - member_offset);
    if (var_offset % member_size != 0) {
      var_offset = offset_align(var_offset - member_size, member_size);
    }

    if (member_size == 4) {
      fprintf(file, "%*smovl -%d(%%rbp), %%eax\n", indent, "", var_offset);
    } else if (member_size == 8) {
      fprintf(file, "%*smovq -%d(%%rbp), %%rax\n", indent, "", var_offset);
    } else if (member_size == 1) {
      fprintf(file, "%*smovsbl -%d(%%rbp), %%edx\n", indent, "", var_offset);
      fprintf(file, "%*smovl %%edx, %%eax\n", indent, "");
    }

    break;
  }
  case AST_DECL: {
    int size = size_of_type(parser, node->decl.kind);
    node->decl.assembly_base_offset = *stack_offset;

    *stack_offset += size;

    int var_offset = node->decl.assembly_base_offset + size;

    fprintf(file, "%*spushq %%rax\n", indent, "");
    fprintf(file, "%*ssubq $%d, %%rsp\n", indent, "", size);

    if (node->decl.expr) {
      parser_dump_assembly_program(parser, node->decl.expr, file, indent,
                                   currentfunc, stack_offset);

      if (node->type->kind != AST_STRUCT) {
        if (size == 4) {
          fprintf(file, "%*smovl %%eax, -%d(%%rbp)\n", indent, "", var_offset);
        } else if (size == 1) {
          fprintf(file, "%*smovb %%al, -%d(%%rbp)\n", indent, "", var_offset);
        } else if (size == 8) {
          fprintf(file, "%*smovq %%rax, -%d(%%rbp)\n", indent, "", var_offset);
        } else {
          fprintf(stderr, "unsupported var type size\n");
        }
      }
    }

    break;
  }
  case AST_FUNC_PROTO:
    break;
  case AST_FUNC_CALL:
    break;
  case AST_BLOCK: {
    int current_stack_offset = *stack_offset;
    for (int i = 0; i < node->block.len; i++) {
      parser_dump_assembly_program(parser, node->block.nodes[i], file, indent,
                                   currentfunc, stack_offset);
    }
    *stack_offset = current_stack_offset;
    break;
  }
  case AST_IF_ELSE: {
    static int if_id = 0;
    int id = if_id;
    if_id++;

    parser_dump_assembly_program(parser, node->ifelse.condition, file, indent,
                                 currentfunc, stack_offset);

    fprintf(file, "%*scmpl $0, %%eax\n", indent, "");
    fprintf(file, "%*sje ifFalse%d\n", indent, "", id);

    fprintf(file, "%*sifTrue%d:\n", indent, "", id);

    parser_dump_assembly_program(parser, node->ifelse.ifblock, file, indent,
                                 currentfunc, stack_offset);

    fprintf(file, "%*sjmp fi%d\n", indent, "", id);

    fprintf(file, "%*sifFalse%d:\n", indent, "", id);
    parser_dump_assembly_program(parser, node->ifelse.elseblock, file, indent,
                                 currentfunc, stack_offset);
    fprintf(file, "%*sfi%d:\n", indent, "", id);
    break;
  }
  case AST_BREAK:
    break;
  case AST_UNOP:
    break;
  case AST_FORLOOP: {
    static int loop_id = 0;
    int id = loop_id;
    loop_id++;

    int saved_stack = *stack_offset;

    parser_dump_assembly_program(parser, node->forloop.init, file, indent,
                                 currentfunc, stack_offset);
    fprintf(file, "%*sforloop%d:\n", indent, "", id);
    parser_dump_assembly_program(parser, node->forloop.condition, file, indent,
                                 currentfunc, stack_offset);
    fprintf(file, "%*scmpl $0, %%eax\n", indent, "");
    fprintf(file, "%*sje forexit%d\n", indent, "", id);
    parser_dump_assembly_program(parser, node->forloop.stmt, file, indent,
                                 currentfunc, stack_offset);

    parser_dump_assembly_program(parser, node->forloop.step, file, indent,
                                 currentfunc, stack_offset);
    fprintf(file, "%*sjmp forloop%d\n", indent, "", id);
    fprintf(file, "%*sforexit%d:\n", indent, "", id);
    *stack_offset = saved_stack;
    break;
  }
  case AST_MEMBER_ACCESS:
    break;
  case AST_ERROR:
    fprintf(file, "sorry %s not supported yet\n", ast_names[node->kind]);
    break;
  }
}

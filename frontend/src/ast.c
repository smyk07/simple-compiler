/*
 * ast: SCULL Abstract Syntax Tree function declarations and node definitions.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "frontend/ast.h"

#include "core/common.h"
#include "core/ds/arena.h"
#include "core/ds/dynamic_array.h"
#include "core/ds/ht.h"
#include "core/utils.h"
#include "frontend/types.h"
#include "frontend/var.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void ast_init(ast *a) {
  arena_init(&a->arena);
  dynamic_array_init(&a->instrs, sizeof(instr_node *));
}

/*
 * @brief: prints a variable.
 *
 * @param var: pointer to a "variable" struct.
 */
static void check_var_and_print(variable *var) {
  switch (var->type) {
  case TYPE_U8:
  case TYPE_U16:
  case TYPE_U32:
  case TYPE_U64:
  case TYPE_U128:
  case TYPE_I8:
  case TYPE_I16:
  case TYPE_I32:
  case TYPE_I64:
  case TYPE_I128:
  case TYPE_BOOL:
  case TYPE_CHAR:
  case TYPE_STRING:
    printf("%s %s", type_to_str(var->type), var->name);
    break;

  case TYPE_POINTER:
    printf("*%s", var->name);
    break;

  case TYPE_INVALID:
  case TYPE_VOID:
    break;
  }
}

/*
 * @brief: prints an expression node. (declaration)
 *
 * @param expr: pointer to an expression node.
 */
static void check_arithmetic_expr_and_print(arithmetic_expr_node *expr);

/*
 * @brief: prints a term node.
 *
 * @param expr: pointer to a term node.
 */
static void check_term_and_print(term_node *term) {
  switch (term->kind) {
  case TERM_INVALID:
    printf("invalid term");
    break;

  case TERM_INT:
    printf("%d", term->value.integer);
    break;

  case TERM_CHAR:
    printf("\'%c\'", term->value.character);
    break;

  case TERM_STRING:
    char *str = term->value.str;
    u64 len = strlen(str);
    if (len >= 1 && str[len - 1] == '\n')
      printf("\"%.*s\"", (int)(len - 1), str);
    else
      printf("\"%s\"", str);
    break;

  case TERM_IDENTIFIER:
    printf("%s", term->identifier.name);
    break;

  case TERM_POINTER:
  case TERM_DEREF:
    printf("*%s", term->identifier.name);
    break;

  case TERM_ADDOF:
    printf("&%s", term->identifier.name);
    break;

  case TERM_ARRAY_ACCESS:
    printf("%s[", term->array_access.array_var.name);
    check_arithmetic_expr_and_print(term->array_access.index_expr);
    printf("]");
    break;

  case TERM_ARRAY_LITERAL:
    printf("{...}");
    break;

  case TERM_FUNCTION_CALL:
    printf("%s(", term->fn_call.name);
    for (u64 i = 0; i < term->fn_call.parameters.count; i++) {
      arithmetic_expr_node *arg =
          dynamic_array_get_ptr(&term->fn_call.parameters, i);
      check_arithmetic_expr_and_print(arg);
      if (i < term->fn_call.parameters.count - 1) {
        printf(", ");
      }
    }
    printf(")");
    break;
  }
}

/*
 * @brief: prints an expression node. (definition)
 *
 * @param expr: pointer to an expression node.
 */
static void check_arithmetic_expr_and_print(arithmetic_expr_node *expr) {
  switch (expr->kind) {
  case AR_EXPR_INVALID:
    printf("invalid arithmetic expr");
    break;

  case AR_EXPR_TERM:
    check_term_and_print(&expr->term);
    break;

  case AR_EXPR_ADD:
    printf("(");
    check_arithmetic_expr_and_print(expr->binary.left);
    printf(" + ");
    check_arithmetic_expr_and_print(expr->binary.right);
    printf(")");
    break;

  case AR_EXPR_SUBTRACT:
    printf("(");
    check_arithmetic_expr_and_print(expr->binary.left);
    printf(" - ");
    check_arithmetic_expr_and_print(expr->binary.right);
    printf(")");
    break;

  case AR_EXPR_MULTIPLY:
    printf("(");
    check_arithmetic_expr_and_print(expr->binary.left);
    printf(" * ");
    check_arithmetic_expr_and_print(expr->binary.right);
    printf(")");
    break;

  case AR_EXPR_DIVIDE:
    printf("(");
    check_arithmetic_expr_and_print(expr->binary.left);
    printf(" / ");
    check_arithmetic_expr_and_print(expr->binary.right);
    printf(")");
    break;

  case AR_EXPR_MODULO:
    printf("(");
    check_arithmetic_expr_and_print(expr->binary.left);
    printf(" %% ");
    check_arithmetic_expr_and_print(expr->binary.right);
    printf(")");
    break;

  case AR_EXPR_UNARY_MINUS:
    printf("-");
    check_arithmetic_expr_and_print(expr->unary);
    break;
  }
}

static void check_expr_node_and_print(expr_node *expr);

static void check_binary_node_and_print(term_binary_node *bnode,
                                        char *operator) {
  check_term_and_print(&bnode->lhs);
  printf(" %s ", operator);
  check_term_and_print(&bnode->rhs);
}

static void check_rel_node_and_print(rel_node *rel) {
  switch (rel->kind) {
  case REL_INVALID:
    printf("invalid rel expr");
    break;

  case REL_IS_EQUAL:
    check_binary_node_and_print(&rel->comparison, "==");
    break;
  case REL_NOT_EQUAL:
    check_binary_node_and_print(&rel->comparison, "!=");
    break;
  case REL_LESS_THAN:
    break;
  case REL_LESS_THAN_OR_EQUAL:
    check_binary_node_and_print(&rel->comparison, "<=");
    break;
  case REL_GREATER_THAN:
    check_binary_node_and_print(&rel->comparison, ">");
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    check_binary_node_and_print(&rel->comparison, ">=");
    break;
  }
}

static void check_logical_node_and_print(logical_node *log) {
  switch (log->kind) {
  case LOG_INVALID:
    printf("invalid logical node");
    break;

  case LOG_AND:
    check_expr_node_and_print(log->binary.lhs);
    printf(" AND ");
    check_expr_node_and_print(log->binary.rhs);
    printf("\n");
    break;
  case LOG_OR:
    check_expr_node_and_print(log->binary.lhs);
    printf(" OR ");
    check_expr_node_and_print(log->binary.rhs);
    printf("\n");
    break;
  case LOG_NOT:
    printf("NOT ");
    check_expr_node_and_print(log->unary.operand);
    break;
  }
}

static void check_expr_node_and_print(expr_node *expr) {
  switch (expr->kind) {
  case EXPR_INVALID:
    printf("invalid expr");
    break;

  case EXPR_TERM:
    check_term_and_print(&expr->term);
    break;
  case EXPR_LOGICAL:
    check_logical_node_and_print(&expr->logical);
    break;
  case EXPR_RELATIONAL:
    check_rel_node_and_print(&expr->relational);
    break;
  case EXPR_BOOL:
    if (expr->boolean)
      printf("true\n");
    else
      printf("false\n");
  }
}

/*
 * Counts the indentation for ast printing
 */
static u32 icount = 0;

#define PRINT_INDENTATION                                                      \
  for (u32 i = 0; i < icount; i++)                                             \
    fputc('\t', stdout);

static void print_cond_block(cond_block_node *block) {
  if (!block)
    return;

  icount++;

  if (block->kind == COND_SINGLE_INSTR) {
    print_instr(block->single);
  } else {
    for (u64 i = 0; i < block->multi.count; i++) {
      instr_node *instr =
          *(instr_node **)dynamic_array_get_ptr(&block->multi, i);
      print_instr(instr);
    }
  }

  icount--;
}

/*
 * @brief: print an instruction.
 *
 * @param instr: pointer to an instruction.
 */
void print_instr(instr_node *instr) {
  PRINT_INDENTATION

  printf("[line %" PRIu64 "] ", instr->line);

  switch (instr->kind) {
  case INSTR_INVALID:
    printf("invalid instr\n");
    break;

  case INSTR_DECLARE:
    printf("declare: ");
    check_var_and_print(&instr->declare_variable);
    printf("\n");
    break;

  case INSTR_INITIALIZE:
    printf("initialize: ");
    check_var_and_print(&instr->initialize_variable.var);
    printf(" = ");
    switch (instr->initialize_variable.var.type) {
    case TYPE_U8:
    case TYPE_U16:
    case TYPE_U32:
    case TYPE_U64:
    case TYPE_U128:
    case TYPE_I8:
    case TYPE_I16:
    case TYPE_I32:
    case TYPE_I64:
    case TYPE_I128:
    case TYPE_POINTER:
      check_arithmetic_expr_and_print(instr->initialize_variable.arithmetic);
      printf("\n");
      break;

    case TYPE_BOOL:
      check_expr_node_and_print(&instr->initialize_variable.boolean);
      break;

    case TYPE_CHAR:
      switch (instr->initialize_variable.arithmetic->kind) {
      case AR_EXPR_TERM:
        printf("\'%c\'\n",
               instr->initialize_variable.arithmetic->term.value.character);
        break;
      default:
        break;
      }
      break;

    default:
      break;
    }
    break;

  case INSTR_ASSIGN:
    printf("assign: ");
    check_var_and_print(&instr->assign.identifier);
    printf(" = ");
    check_arithmetic_expr_and_print(instr->assign.expr);
    printf("\n");
    break;

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    printf("assign to array subscript: ");
    check_var_and_print(&instr->assign_to_array_subscript.var);
    printf("[");
    check_arithmetic_expr_and_print(
        instr->assign_to_array_subscript.index_expr);
    printf("] = ");
    check_arithmetic_expr_and_print(
        instr->assign_to_array_subscript.expr_to_assign);
    printf("\n");
    break;

  case INSTR_DECLARE_ARRAY:
    printf("declare array: ");
    check_var_and_print(&instr->declare_array.var);
    printf("[");
    check_arithmetic_expr_and_print(instr->declare_array.size_expr);
    printf("]\n");
    break;

  case INSTR_INITIALIZE_ARRAY:
    printf("initialize array: ");
    check_var_and_print(&instr->initialize_array.var);
    printf("[");
    check_arithmetic_expr_and_print(instr->initialize_array.size_expr);
    printf("] = {");
    for (u64 i = 0; i < instr->initialize_array.literal.elements.count; i++) {
      arithmetic_expr_node *elem =
          dynamic_array_get_ptr(&instr->initialize_array.literal.elements, i);
      check_arithmetic_expr_and_print(elem);
      if (i < instr->initialize_array.literal.elements.count - 1) {
        printf(", ");
      }
    }
    printf("}\n");
    break;

  case INSTR_IF: {
    if_node *ifn = &instr->if_;
    printf("if ");
    check_expr_node_and_print(&ifn->condition);
    PRINT_INDENTATION
    printf("then:\n");
    print_cond_block(&ifn->then);
    if (ifn->else_) {
      PRINT_INDENTATION
      printf("else:\n");
      print_cond_block(ifn->else_);
    }
    break;
  }

  case INSTR_MATCH: {
    match_node *match = &instr->match;

    printf("match ");
    check_arithmetic_expr_and_print(match->expr);
    printf(" {\n");

    icount++;

    for (u64 i = 0; i < match->cases.count; i++) {
      match_case_node *case_node = dynamic_array_get_ptr(&match->cases, i);

      PRINT_INDENTATION

      printf("case ");

      switch (case_node->kind) {
      case MATCH_CASE_INVALID:
        printf("invalid match case\n");
        break;

      case MATCH_CASE_VALUES: {
        for (u64 j = 0; j < case_node->values.values.count; j++) {
          arithmetic_expr_node *val;
          dynamic_array_get(&case_node->values.values, j, &val);
          check_arithmetic_expr_and_print(val);
          if (j < case_node->values.values.count - 1)
            printf(", ");
        }
        printf(":\n");
        break;
      }

      case MATCH_CASE_RANGE: {
        check_arithmetic_expr_and_print(case_node->range.start);
        printf("...");
        check_arithmetic_expr_and_print(case_node->range.end);
        printf(":\n");
        break;
      }

      case MATCH_CASE_DEFAULT: {
        printf("_:\n");
        break;
      }
      }

      print_cond_block(&case_node->body);
    }

    icount--;

    PRINT_INDENTATION

    printf("}\n");

    break;
  }

  case INSTR_GOTO:
    printf("goto: %s\n", instr->goto_.label);
    break;

  case INSTR_LABEL:
    printf("label: %s\n", instr->label.label);
    break;

  case INSTR_LOOP:
    switch (instr->loop.kind) {
    case LOOP_INVALID:
      printf("invalid loop\n");
      break;

    case LOOP_UNCONDITIONAL:
      printf("loop starts: \n");
      break;

    case LOOP_WHILE:
      printf("while loop starts, break condition: ");
      check_expr_node_and_print(&instr->loop.conditional.break_condition);
      printf("\n");
      break;

    case LOOP_DO_WHILE:
      printf("do-while-loop starts, break condition: ");
      check_expr_node_and_print(&instr->loop.conditional.break_condition);
      printf("\n");
      break;

    case LOOP_FOR:
      printf("for %s in ", instr->loop._for.iterator.name);
      check_arithmetic_expr_and_print(instr->loop._for.range_start);
      printf("...");
      check_arithmetic_expr_and_print(instr->loop._for.range_end);
      printf(" {\n");
      break;
    }

    icount++;

    for (u64 i = 0; i < instr->loop.instrs.count; i++) {
      instr_node *_instr =
          *(instr_node **)dynamic_array_get_ptr(&instr->loop.instrs, i);
      print_instr(_instr);
    }

    icount--;
    break;

  case INSTR_LOOP_BREAK:
    printf("loop break\n");
    break;

  case INSTR_LOOP_CONTINUE:
    printf("loop continue\n");
    break;

  case INSTR_FN_DECLARE:
  case INSTR_FN_DEFINE:
    printf("function %s: %s(",
           instr->fn_declare_node.kind == FN_DECLARED ? "declaration"
                                                      : "definition",
           instr->fn_declare_node.name);

    for (u64 i = 0; i < instr->fn_declare_node.parameters.count; i++) {
      variable *param =
          dynamic_array_get_ptr(&instr->fn_declare_node.parameters, i);
      check_var_and_print(param);
      if (i < instr->fn_declare_node.parameters.count - 1) {
        printf(", ");
      }
    }
    if (instr->fn_declare_node.is_variadic) {
      printf(", ...");
    }
    printf(")");

    if (instr->fn_declare_node.returntype != TYPE_VOID) {
      printf(" : ");
      printf("%s", type_to_str(instr->fn_declare_node.returntype));
    }
    printf("\n");

    if (instr->kind == INSTR_FN_DEFINE) {

      icount++;
      for (u64 i = 0; i < instr->fn_define_node.defined.instrs.count; i++) {
        instr_node *body_instr = *(instr_node **)dynamic_array_get_ptr(
            &instr->fn_declare_node.defined.instrs, i);
        print_instr(body_instr);
      }
      icount--;
    }
    break;

  case INSTR_RETURN:
    printf("return: ");
    if (instr->ret_node.returnvals.count == 0) {
      printf("void\n");
    } else {
      for (u64 i = 0; i < instr->ret_node.returnvals.count; i++) {
        arithmetic_expr_node *ret_expr =
            dynamic_array_get_ptr(&instr->ret_node.returnvals, i);
        check_arithmetic_expr_and_print(ret_expr);
        if (i < instr->ret_node.returnvals.count - 1) {
          printf(", ");
        }
      }
      printf("\n");
    }
    break;

  case INSTR_FN_CALL:
    printf("function call: %s(", instr->fn_call.name);
    for (u64 i = 0; i < instr->fn_call.parameters.count; i++) {
      arithmetic_expr_node *arg =
          dynamic_array_get_ptr(&instr->fn_call.parameters, i);
      check_arithmetic_expr_and_print(arg);
      if (i < instr->fn_call.parameters.count - 1) {
        printf(", ");
      }
    }
    printf(")\n");
    break;
  }
}

void print_ast(ast *program_ast) {
  for (u64 i = 0; i < program_ast->instrs.count; i++) {
    instr_node *instr =
        *(instr_node **)dynamic_array_get_ptr(&program_ast->instrs, i);
    print_instr(instr);
  }
}

static void free_arithmetic_expr_node(arithmetic_expr_node *expr);

static void free_arithmetic_exprs(dynamic_array *exprs);

static void free_term_node(term_node *term) {
  switch (term->kind) {
  case TERM_INVALID:
    scu_punreachable("attempting to free invalid term_node\n");
    break;

  case TERM_INT:
  case TERM_CHAR:
  case TERM_STRING:
  case TERM_IDENTIFIER:
  case TERM_POINTER:
  case TERM_DEREF:
  case TERM_ADDOF:
    break;
  case TERM_ARRAY_ACCESS:
    free_arithmetic_expr_node(term->array_access.index_expr);
    break;
  case TERM_ARRAY_LITERAL:
    dynamic_array_free(&term->array_literal.elements);
    break;
  case TERM_FUNCTION_CALL:
    free_arithmetic_exprs(&term->fn_call.parameters);
    break;
  }
}

static void free_arithmetic_expr_node(arithmetic_expr_node *expr) {
  switch (expr->kind) {
  case AR_EXPR_INVALID:
    scu_punreachable("attempting to free invalid arithmetic_expr_node\n");
    break;

  case AR_EXPR_TERM:
    free_term_node(&expr->term);
    break;

  case AR_EXPR_ADD:
  case AR_EXPR_SUBTRACT:
  case AR_EXPR_MULTIPLY:
  case AR_EXPR_DIVIDE:
  case AR_EXPR_MODULO:
    free_arithmetic_expr_node(expr->binary.left);
    free_arithmetic_expr_node(expr->binary.right);
    break;

  case AR_EXPR_UNARY_MINUS:
    free_arithmetic_expr_node(expr->unary);
    break;
  }
}

static void free_arithmetic_exprs(dynamic_array *exprs) {
  for (u64 i = 0; i < exprs->count; i++) {
    arithmetic_expr_node *expr = dynamic_array_get_ptr(exprs, i);
    free_arithmetic_expr_node(expr);
  }
  dynamic_array_free(exprs);
}

static void free_expr_node(expr_node *expr);

static void free_rel_node(rel_node *rel) {
  free_term_node(&rel->comparison.lhs);
  free_term_node(&rel->comparison.rhs);
}

static void free_logical_node(logical_node *log) {
  switch (log->kind) {
  case LOG_INVALID:
    scu_punreachable("attempting to free invalid logical_node\n");
    break;

  case LOG_AND:
  case LOG_OR:
    free_expr_node(log->binary.lhs);
    free_expr_node(log->binary.rhs);
    break;
  case LOG_NOT:
    free_expr_node(log->unary.operand);
    break;
  }
}

static void free_expr_node(expr_node *expr) {
  switch (expr->kind) {
  case EXPR_INVALID:
    scu_punreachable("attempting to free invalid expr_node\n");
    break;

  case EXPR_TERM:
    free_term_node(&expr->term);
    break;
  case EXPR_LOGICAL:
    free_logical_node(&expr->logical);
    break;
  case EXPR_RELATIONAL:
    free_rel_node(&expr->relational);
    break;
  case EXPR_BOOL:
    break;
  }
}

static void free_instrs(dynamic_array *instrs);

static void free_cond_block_node(cond_block_node *block) {
  if (!block)
    return;

  if (block->kind == COND_MULTI_INSTR)
    free_instrs(&block->multi);
  else
    free_instr(block->single);
}

void free_instr(instr_node *instr) {
  switch (instr->kind) {
  case INSTR_INVALID:
    scu_punreachable("attempting to free invalid instr_node\n");
    break;

  case INSTR_DECLARE:
    free(instr->declare_variable.name);
    break;

  case INSTR_INITIALIZE:
    if (instr->initialize_variable.var.type != TYPE_BOOL)
      free_arithmetic_expr_node(instr->initialize_variable.arithmetic);
    free(instr->initialize_variable.var.name);
    break;

  case INSTR_DECLARE_ARRAY:
    free_arithmetic_expr_node(instr->declare_array.size_expr);
    free(instr->declare_array.var.name);
    break;

  case INSTR_INITIALIZE_ARRAY:
    free_arithmetic_expr_node(instr->initialize_array.size_expr);
    free_arithmetic_exprs(&instr->initialize_array.literal.elements);
    free(instr->initialize_array.var.name);
    break;

  case INSTR_ASSIGN:
    free_arithmetic_expr_node(instr->assign.expr);
    break;

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    free_arithmetic_expr_node(instr->assign_to_array_subscript.index_expr);
    free_arithmetic_expr_node(instr->assign_to_array_subscript.expr_to_assign);
    break;

  case INSTR_IF:
    free_expr_node(&instr->if_.condition);
    free_cond_block_node(&instr->if_.then);

    for (u64 i = 0; i < instr->if_.else_ifs.count; i++) {
      if_node *node = dynamic_array_get_ptr(&instr->if_.else_ifs, i);
      free_cond_block_node(&node->then);
    }
    dynamic_array_free(&instr->if_.else_ifs);

    free_cond_block_node(instr->if_.else_);
    break;

  case INSTR_MATCH:
    free_arithmetic_expr_node(instr->match.expr);

    for (u64 i = 0; i < instr->match.cases.count; i++) {
      match_case_node *case_node =
          dynamic_array_get_ptr(&instr->match.cases, i);

      switch (case_node->kind) {
      case MATCH_CASE_INVALID:
        scu_punreachable("attempting to free invalid match_case_node\n");
        break;

      case MATCH_CASE_VALUES:
        for (u64 j = 0; j < case_node->values.values.count; j++) {
          arithmetic_expr_node *expr;
          dynamic_array_get(&case_node->values.values, j, &expr);
          free_arithmetic_expr_node(expr);
        }
        dynamic_array_free(&case_node->values.values);
        break;

      case MATCH_CASE_RANGE:
        free_arithmetic_expr_node(case_node->range.start);
        free_arithmetic_expr_node(case_node->range.end);
        break;

      case MATCH_CASE_DEFAULT:
        break;
      }

      free_cond_block_node(&case_node->body);
    }

    dynamic_array_free(&instr->match.cases);
    break;

  case INSTR_LOOP:
    switch (instr->loop.kind) {
    case LOOP_INVALID:
      scu_punreachable("attempting to free invalid loop_node\n");
      break;

    case LOOP_DO_WHILE:
    case LOOP_WHILE:
      free_expr_node(&instr->loop.conditional.break_condition);
      break;
    case LOOP_FOR:
      free_arithmetic_expr_node(instr->loop._for.range_start);
      free_arithmetic_expr_node(instr->loop._for.range_end);
      break;
    case LOOP_UNCONDITIONAL:
      break;
    }

    ht_destroy(instr->loop.variables);
    free_instrs(&instr->loop.instrs);
    break;

  case INSTR_FN_DEFINE:
    ht_destroy(instr->fn_define_node.defined.variables);
    free_instrs(&instr->fn_define_node.defined.instrs);
    dynamic_array_free(&instr->fn_define_node.parameters);
    break;

  case INSTR_FN_DECLARE:
    dynamic_array_free(&instr->fn_declare_node.parameters);
    break;

  case INSTR_RETURN:
    free_arithmetic_exprs(&instr->ret_node.returnvals);
    break;

  case INSTR_FN_CALL:
    free_arithmetic_exprs(&instr->fn_call.parameters);
    break;

  case INSTR_GOTO:
  case INSTR_LABEL:
  case INSTR_LOOP_BREAK:
  case INSTR_LOOP_CONTINUE:
    break;
  }
}

static void free_instrs(dynamic_array *instrs) {
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node *instr = *(instr_node **)dynamic_array_get_ptr(instrs, i);
    free_instr(instr);
  }
  dynamic_array_free(instrs);
}

void ast_free(ast *program_ast) {
  if (program_ast == NULL)
    return;
  free_instrs(&program_ast->instrs);
  arena_free(&program_ast->arena);
}

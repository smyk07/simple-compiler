/*
 * semantic: variable checking and type checking for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "semantic.h"
#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/ht.h"
#include "utils.h"
#include "var.h"

#include <inttypes.h>
#include <stddef.h>

#define _POSIX_C_SOURCE 200809L
#include <string.h>

u32 evaluate_const_expr(expr_node *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (expr->kind) {
  case EXPR_TERM:
    if (expr->term.kind == TERM_INT) {
      return expr->term.value.integer;
    }
    scu_perror("Array size must be a constant expression\n");
    return 0;

  case EXPR_ADD:
    return evaluate_const_expr(expr->binary.left) +
           evaluate_const_expr(expr->binary.right);

  case EXPR_SUBTRACT:
    return evaluate_const_expr(expr->binary.left) -
           evaluate_const_expr(expr->binary.right);

  case EXPR_MULTIPLY:
    return evaluate_const_expr(expr->binary.left) *
           evaluate_const_expr(expr->binary.right);

  case EXPR_DIVIDE: {
    u32 right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      scu_perror("Division by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) / right;
  }

  case EXPR_MODULO: {
    u32 right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      scu_perror("Division by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) % right;
  }
  }
}

/*
 * @brief: convert a type enumeration to its string representation.
 */
static const char *type_to_str(type type) {
  switch (type) {
  case TYPE_INT:
    return "int";
  case TYPE_CHAR:
    return "char";
  case TYPE_STRING:
    return "string";
  case TYPE_POINTER:
    return "ptr";
  case TYPE_VOID:
    return "void";
  }
}

/*
 * @brief: check function call validity (declaration)
 *
 * @param fn_call: pointer to the function call node.
 * @param functions: pointer to the functions hash table.
 * @param variables: pointer to the variables hash table (for argument
 * expressions).
 * @param line: line number of the function call.
 */
static void check_function_call(fn_call_node *fn_call, ht *functions,
                                ht *variables, u64 line);

/*
 * @brief: check for types in an instr_node (declaration)
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 */
static void instr_typecheck(instr_node *instr, ht *variables, ht *functions);

/*
 * @brief: running stack offset counter for allocating variables and arrays.
 */
static u64 current_stack_offset = 0;

/*
 * @brief: insert a new variable into the variables hash table.
 *
 * @param var_to_declare: the variable struct to append.
 * @param variables: pointer to the variables hash table.
 */
static void declare_variables(variable *var_to_declare, ht *variables) {
  if (!var_to_declare || !var_to_declare->name || !variables)
    return;

  variable *var = ht_search(variables, var_to_declare->name);
  if (var)
    return;

  var_to_declare->stack_offset = current_stack_offset;
  current_stack_offset += 1;
  ht_insert(variables, var_to_declare->name, var_to_declare);
}

/*
 * @brief: insert a new array into the variables hash table.
 *
 * @param var_to_declare: the variable struct to append.
 * @param variables: pointer to the variables hash table.
 */
static void declare_array(variable *arr_to_declare, expr_node *size_expr,
                          ht *variables) {
  if (!arr_to_declare || !arr_to_declare->name || !variables)
    return;

  variable *var = ht_search(variables, arr_to_declare->name);
  if (var)
    return;

  u32 array_size = evaluate_const_expr(size_expr);
  u64 size_bytes = array_size * 4;
  arr_to_declare->stack_offset = current_stack_offset;
  current_stack_offset += size_bytes;

  ht_insert(variables, arr_to_declare->name, arr_to_declare);
}

/*
 * @brief: check variables in terms
 *
 * @param term: pointer to a term_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static void term_check_variables(term_node *term, ht *variables,
                                 ht *functions) {

  switch (term->kind) {
  case TERM_IDENTIFIER:
    variable *var = ht_search(variables, term->identifier.name);
    if (!var) {
      scu_perror("Use of undeclared variable: %s [line %u]\n",
                 term->identifier.name, term->identifier.line);
    }
    break;

  case TERM_FUNCTION_CALL:
    check_function_call(&term->fn_call, functions, variables, term->line);
    break;

  default:
    break;
  }
}

/*
 * @brief: check variables in expressions
 *
 * @param expr: pointer to an expr_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static void expr_check_variables(expr_node *expr, ht *variables,
                                 ht *functions) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_check_variables(&expr->term, variables, functions);
    break;
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    expr_check_variables(expr->binary.left, variables, functions);
    expr_check_variables(expr->binary.right, variables, functions);
    break;
  }
}

/*
 * @brief: check variables in relational expressions
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static void rel_check_variables(rel_node *rel, ht *variables, ht *functions) {
  term_check_variables(&rel->comparison.lhs, variables, functions);
  term_check_variables(&rel->comparison.rhs, variables, functions);
}

static void instr_check_variables(instr_node *instr, ht *variables,
                                  ht *functions);

static void cond_block_check_variables(cond_block_node *blk, ht *variables,
                                       ht *functions) {
  if (!blk)
    return;

  if (blk->kind == COND_SINGLE_INSTR) {
    instr_check_variables(blk->single, variables, functions);
    instr_typecheck(blk->single, variables, functions);
  } else {
    for (u64 i = 0; i < blk->multi.count; i++) {
      instr_node instr_;
      dynamic_array_get(&blk->multi, i, &instr_);
      instr_check_variables(&instr_, variables, functions);
      instr_typecheck(&instr_, variables, functions);
    }
  }
}

/*
 * @brief: check loop validity and body instructions
 *
 * @param loop: pointer to the loop node.
 * @param variables: pointer to the parent scope's variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static void check_loop(loop_node *loop, ht *parent_variables, ht *functions) {
  if (!loop)
    return;

  // copying the parent scope's variables into current scope so that semantics
  // dont break.
  // This is just a temporary solution
  // TODO THIS NEEDS TO BE BETTER (along with a lot of other things)
  for (u64 i = 0; i < parent_variables->capacity; i++) {
    ht_item *item = parent_variables->items[i];
    if (item && item->key && item->value) {
      ht_insert(loop->variables, item->key, item->value);
    }
  }

  if (loop->kind == LOOP_WHILE || loop->kind == LOOP_DO_WHILE) {
    rel_check_variables(&loop->conditional.break_condition, loop->variables,
                        functions);
  } else if (loop->kind == LOOP_FOR) {
    expr_check_variables(loop->_for.range_start, loop->variables, functions);
    expr_check_variables(loop->_for.range_end, loop->variables, functions);

    declare_variables(&loop->_for.iterator, loop->variables);
  }

  for (u64 i = 0; i < loop->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&loop->instrs, i, &instr);

    instr_check_variables(&instr, loop->variables, functions);
    instr_typecheck(&instr, loop->variables, functions);
  }
}

/*
 * @brief: check variables in an individual instruction
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static void instr_check_variables(instr_node *instr, ht *variables,
                                  ht *functions) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    declare_variables(&instr->declare_variable, variables);
    break;

  case INSTR_INITIALIZE:
    expr_check_variables(instr->initialize_variable.expr, variables, functions);
    declare_variables(&instr->initialize_variable.var, variables);
    break;

  case INSTR_DECLARE_ARRAY:
    declare_array(&instr->declare_array.var, instr->declare_array.size_expr,
                  variables);
    break;

  case INSTR_INITIALIZE_ARRAY:
    declare_array(&instr->initialize_array.var,
                  instr->initialize_array.size_expr, variables);
    for (u64 i = 0; i < instr->initialize_array.literal.elements.count; i++) {
      expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);
      expr_check_variables(&elem, variables, functions);
    }
    break;

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    variable *arr =
        ht_search(variables, instr->assign_to_array_subscript.var.name);
    if (!arr) {
      scu_perror("Use of undeclared array: %s [line %u]\n",
                 instr->assign_to_array_subscript.var.name,
                 instr->assign_to_array_subscript.var.line);
    }
    expr_check_variables(instr->assign_to_array_subscript.index_expr, variables,
                         functions);
    expr_check_variables(instr->assign_to_array_subscript.expr_to_assign,
                         variables, functions);
    break;

  case INSTR_ASSIGN:
    expr_check_variables(instr->assign.expr, variables, functions);
    break;

  case INSTR_IF:
    rel_check_variables(&instr->if_.rel, variables, functions);
    cond_block_check_variables(&instr->if_.then, variables, functions);
    if (instr->if_.else_)
      cond_block_check_variables(instr->if_.else_, variables, functions);
    break;

  case INSTR_MATCH:
    expr_check_variables(instr->match.expr, variables, functions);

    for (u64 i = 0; i < instr->match.cases.count; i++) {
      match_case_node case_node;
      dynamic_array_get(&instr->match.cases, i, &case_node);

      switch (case_node.kind) {
      case MATCH_CASE_VALUES:
        for (u64 j = 0; j < case_node.values.values.count; j++) {
          expr_node *expr;
          dynamic_array_get(&case_node.values.values, j, &expr);
          expr_check_variables(expr, variables, functions);
        }
        break;
      case MATCH_CASE_RANGE:
        expr_check_variables(case_node.range.start, variables, functions);
        expr_check_variables(case_node.range.end, variables, functions);
        break;
      case MATCH_CASE_DEFAULT:
        break;
      }

      cond_block_check_variables(&case_node.body, variables, functions);
    }
    break;

  case INSTR_LOOP:
    check_loop(&instr->loop, variables, functions);
    break;

  default:
    break;
  }
}

/*
 * @brief: make sure labels are properly declared and not duplicated
 *
 * @param labels: pointer to the labels dynamic_array.
 * @param instr: pointer to an instr_node.
 */
static void check_label(dynamic_array *labels, instr_node *instr) {
  const char *label_name = instr->label.label;
  for (u64 i = 0; i < labels->count; i++) {
    char *existing;
    dynamic_array_get(labels, i, &existing);
    if (strcmp(label_name, existing) == 0) {
      scu_perror("Duplicate label declaration: %s [line %u]\n", label_name,
                 instr->line);
      return;
    }
  }
  dynamic_array_append(labels, &label_name);
}

/*
 * @brief: make sure labels are properly used in goto instructions
 *
 * @param labels: pointer to the labels dynamic_array.
 * @param instr: pointer to an instr_node.
 */
static void check_goto(dynamic_array *labels, instr_node *instr) {
  u32 found = 0;
  for (u64 i = 0; i < labels->count; i++) {
    char *label;
    dynamic_array_get(labels, i, &label);
    if (strcmp(label, instr->goto_.label) == 0) {
      found = 1;
      break;
    }
  }
  if (!found) {
    scu_perror("Use of undeclared label: %s [line %u]\n", instr->goto_.label,
               instr->line);
  }
}

static void instrs_check_labels(dynamic_array *instrs, dynamic_array *labels);

static void cond_block_check_labels(cond_block_node *block,
                                    dynamic_array *labels) {
  if (!block)
    return;

  if (block->kind == COND_SINGLE_INSTR) {
    instr_node *instr = block->single;

    if (instr->kind == INSTR_LABEL)
      check_label(labels, instr);
    else if (instr->kind == INSTR_GOTO)
      check_goto(labels, instr);
    else if (instr->kind == INSTR_IF)
      instrs_check_labels(
          &((dynamic_array){.items = instr, .count = 1, .capacity = 1}),
          labels);
  } else {
    for (u64 i = 0; i < block->multi.count; i++) {
      instr_node instr;
      dynamic_array_get(&block->multi, i, &instr);

      if (instr.kind == INSTR_LABEL)
        check_label(labels, &instr);
      else if (instr.kind == INSTR_GOTO)
        check_goto(labels, &instr);
      else if (instr.kind == INSTR_IF)
        cond_block_check_labels(&instr.if_.then, labels),
            cond_block_check_labels(instr.if_.else_, labels);
    }
  }
}

/*
 * @brief: check for declaration of labels AND the use of labels in goto
 * instructions
 *
 * @param instr: pointer to an instr_node.
 * @param labels: pointer to the labels dynamic_array.
 */
static void instrs_check_labels(dynamic_array *instrs, dynamic_array *labels) {
  // check labels first
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_LABEL)
      check_label(labels, &instr);
  }

  // then check goto
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_GOTO)
      check_goto(labels, &instr);
  }

  // then check if
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_IF) {
      cond_block_check_labels(&instr.if_.then, labels);
      if (instr.if_.else_)
        cond_block_check_labels(instr.if_.else_, labels);
    }
  }
}

/*
 * @brief: check for types in an expr_node (declaration)
 *
 * @param expr: pointer to an expr_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables hash table.
 */
static type expr_type(expr_node *expr, type target_type, ht *variables,
                      ht *functions);

/*
 * @brief: check for types in a term_node
 *
 * @param term: pointer to a term_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables hash table.
 * @param line: where the term is situated in the source buffer.
 */
static type term_type(term_node *term, ht *variables, ht *functions) {
  switch (term->kind) {
  case TERM_INT:
    return TYPE_INT;
  case TERM_CHAR:
    return TYPE_CHAR;
  case TERM_STRING:
    return TYPE_STRING;
  case TERM_POINTER:
  case TERM_DEREF:
  case TERM_ADDOF:
  case TERM_IDENTIFIER:
    return get_var_type(variables, &term->identifier);

  case TERM_ARRAY_ACCESS:
    type array_type = get_var_type(variables, &term->array_access.array_var);
    if (array_type == TYPE_VOID) {
      scu_perror("Array '%s' not declared [line %" PRIu64 "]\n",
                 term->array_access.array_var.name, term->line);
      return TYPE_VOID;
    }
    type index_type = expr_type(term->array_access.index_expr, TYPE_INT,
                                variables, functions);
    if (index_type != TYPE_INT) {
      scu_perror("Array index must be of type int, got type at [line %" PRIu64
                 "]\n",
                 term->line);
    }
    return array_type;

  case TERM_ARRAY_LITERAL:
    scu_perror("Array literal cannot be used in expressions [line %" PRIu64
               "]\n",
               term->line);
    return -1;
    break;

  case TERM_FUNCTION_CALL: {
    fn_node *fn = ht_search(functions, term->fn_call.name);
    if (!fn) {
      scu_perror("Call to undeclared function: %s [line %" PRIu64 "]\n",
                 term->fn_call.name, term->line);
      return TYPE_VOID;
    }

    if (!fn->is_variadic &&
        term->fn_call.parameters.count != fn->parameters.count) {
      scu_perror("Function '%s' expects %" PRIu64 " arguments, but %" PRIu64
                 " were provided [line %" PRIu64 "]\n",
                 term->fn_call.name, fn->parameters.count,
                 term->fn_call.parameters.count, term->line);
    } else if (fn->is_variadic &&
               term->fn_call.parameters.count < fn->parameters.count) {
      scu_perror("Variadic function '%s' requires at least %" PRIu64
                 " fixed arguments [line %" PRIu64 "]\n",
                 term->fn_call.name, fn->parameters.count, term->line);
    }

    for (u64 i = 0;
         i < term->fn_call.parameters.count && i < fn->parameters.count; i++) {
      expr_node arg_expr;
      dynamic_array_get(&term->fn_call.parameters, i, &arg_expr);

      variable param;
      dynamic_array_get(&fn->parameters, i, &param);

      type arg_type = expr_type(&arg_expr, param.type, variables, functions);

      if (arg_type != param.type) {
        if (!(param.type == TYPE_POINTER &&
              (arg_type == TYPE_STRING || arg_type == TYPE_POINTER))) {
          scu_perror(

              "Type mismatch in argument %" PRIu64
              " to function '%s': expected %s, got %s [line %" PRIu64 "]\n",
              i + 1, term->fn_call.name, type_to_str(param.type),
              type_to_str(arg_type), term->line);
        }
      }
    }

    if (fn->returntypes.count == 0) {
      scu_perror("Function '%s' has no return value but is used in expression "
                 "[line %" PRIu64 "]\n",
                 term->fn_call.name, term->line);
      return TYPE_VOID;
    }

    type return_type;
    dynamic_array_get(&fn->returntypes, 0, &return_type);
    return return_type;
  }
  }
}

/*
 * @brief: check for types in an expr_node (definition)
 *
 * @param expr: pointer to an expr_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables hash table.
 */
static type expr_type(expr_node *expr, type target_type, ht *variables,
                      ht *functions) {
  type lhs, rhs;

  switch (expr->kind) {
  case EXPR_TERM:
    return term_type(&expr->term, variables, functions);
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    lhs = expr_type(expr->binary.left, target_type, variables, functions);
    rhs = expr_type(expr->binary.right, target_type, variables, functions);
    break;
  }

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror("Type mismatch in arithmetic expression: %s vs %s [line %u]\n",
               lhs_type_str, rhs_type_str, expr->line);
  }

  return lhs;
}

/*
 * @brief: check for types in a rel_node
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables hash table.
 */
static void rel_typecheck(rel_node *rel, ht *variables, ht *functions) {
  type lhs, rhs;

  lhs = term_type(&rel->comparison.lhs, variables, functions);
  rhs = term_type(&rel->comparison.rhs, variables, functions);

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror("Type mismatch in conditional statement: %s vs %s [line %u]\n",
               lhs_type_str, rhs_type_str, rel->line);
  }
}

/*
 * @brief: check for types in an instr_node
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 */
static void instr_typecheck(instr_node *instr, ht *variables, ht *functions) {
  switch (instr->kind) {
  case INSTR_INITIALIZE: {
    type target_type = instr->initialize_variable.var.type;
    type expr_result = expr_type(instr->initialize_variable.expr, target_type,
                                 variables, functions);
    if (target_type == TYPE_POINTER) {
      return;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror("Type mismatch in initialization to %s - %s to %s [line %u]\n",
                 instr->assign.identifier.name, expr_result_str,
                 target_type_str, instr->line);
    }
    break;
  }

  case INSTR_INITIALIZE_ARRAY: {
    type array_type = instr->initialize_array.var.type;
    for (u64 i = 0; i < instr->initialize_array.literal.elements.count; i++) {
      expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);
      type elem_type = expr_type(&elem, array_type, variables, functions);
      if (array_type != elem_type && array_type != TYPE_POINTER) {
        const char *array_type_str = type_to_str(array_type);
        const char *elem_type_str = type_to_str(elem_type);
        scu_perror("Type mismatch in array initialization - element %" PRIu64
                   " is %s but array is %s [line %u]\n",
                   i, elem_type_str, array_type_str, instr->line);
      }
    }
    break;
  }

  case INSTR_ASSIGN: {
    type target_type = get_var_type(variables, &instr->assign.identifier);
    type expr_result =
        expr_type(instr->assign.expr, target_type, variables, functions);
    if (target_type == TYPE_POINTER) {
      return;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror("Type mismatch in assignment to %s - %s to %s [line %u]\n",
                 instr->assign.identifier.name, expr_result_str,
                 target_type_str, instr->line);
    }
    break;
  }

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT: {
    type array_type =
        get_var_type(variables, &instr->assign_to_array_subscript.var);

    type index_type = expr_type(instr->assign_to_array_subscript.index_expr,
                                TYPE_INT, variables, functions);
    if (index_type != TYPE_INT) {
      scu_perror("Array index must be of type int, got %s [line %u]\n",
                 type_to_str(index_type), instr->line);
    }

    type expr_result =
        expr_type(instr->assign_to_array_subscript.expr_to_assign, array_type,
                  variables, functions);
    if (array_type != expr_result && array_type != TYPE_POINTER) {
      const char *array_type_str = type_to_str(array_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror(

          "Type mismatch in array assignment to %s - %s to %s [line %u]\n",
          instr->assign_to_array_subscript.var.name, expr_result_str,
          array_type_str, instr->line);
    }
    break;
  }

  case INSTR_IF: {
    rel_typecheck(&instr->if_.rel, variables, functions);
    if (instr->if_.else_ifs.count > 0) {
      for (u64 i = 0; i < instr->if_.else_ifs.count; i++) {
        if_node else_if_node;
        dynamic_array_get(&instr->if_.else_ifs, i, &else_if_node);
        rel_typecheck(&else_if_node.rel, variables, functions);
      }
    }
    break;
  }

  case INSTR_MATCH: {
    type match_expr_type =
        expr_type(instr->match.expr, TYPE_INT, variables, functions);

    for (u64 i = 0; i < instr->match.cases.count; i++) {
      match_case_node case_node;
      dynamic_array_get(&instr->match.cases, i, &case_node);

      switch (case_node.kind) {
      case MATCH_CASE_VALUES: {
        for (u64 j = 0; j < case_node.values.values.count; j++) {
          expr_node *expr;
          dynamic_array_get(&case_node.values.values, j, &expr);
          type value_type =
              expr_type(expr, match_expr_type, variables, functions);

          if (value_type != match_expr_type) {
            const char *match_type_str = type_to_str(match_expr_type);
            const char *value_type_str = type_to_str(value_type);
            scu_perror("Type mismatch in match case - expected %s but got %s "
                       "[line %u]\n",
                       match_type_str, value_type_str, instr->line);
          }
        }
        break;
      }

      case MATCH_CASE_RANGE: {
        type start_type = expr_type(case_node.range.start, match_expr_type,
                                    variables, functions);
        if (start_type != match_expr_type) {
          const char *match_type_str = type_to_str(match_expr_type);
          const char *start_type_str = type_to_str(start_type);
          scu_perror("Type mismatch in match range start - expected %s but got "
                     "%s [line %u]\n",
                     match_type_str, start_type_str, instr->line);
        }

        type end_type = expr_type(case_node.range.end, match_expr_type,
                                  variables, functions);
        if (end_type != match_expr_type) {
          const char *match_type_str = type_to_str(match_expr_type);
          const char *end_type_str = type_to_str(end_type);
          scu_perror("Type mismatch in match range end - expected %s but got "
                     "%s [line %u]\n",
                     match_type_str, end_type_str, instr->line);
        }
        break;
      }

      case MATCH_CASE_DEFAULT:
        break;
      }
    }
    break;
  }

  default:
    break;
  }
}

/*
 * @brief: insert a new function into the functions hash table.
 *
 * @param fn: pointer to the function node to register.
 * @param functions: pointer to the functions hash table.
 */
static void register_function(fn_node *fn, ht *functions) {
  if (!fn || !fn->name || !functions)
    return;

  fn_node *existing = ht_search(functions, fn->name);
  if (existing) {
    if (existing->is_variadic != fn->is_variadic) {
      scu_perror("Function '%s' variadic mismatch\n", fn->name);
      return;
    }

    if (!fn->is_variadic && existing->parameters.count != fn->parameters.count)
      scu_perror(
          "Function '%s' parameter count mismatch: declared with %" PRIu64
          ", but has %" PRIu64 "\n",
          fn->name, existing->parameters.count, fn->parameters.count);

    if (fn->kind == FN_DEFINED && existing->kind == FN_DEFINED) {
      scu_perror("Duplicate function definition: %s\n", fn->name);
      return;
    }

    if (existing->parameters.count != fn->parameters.count)
      scu_perror(
          "Function '%s' parameter count mismatch: declared with %" PRIu64
          ", but has %" PRIu64 "\n",
          fn->name, existing->parameters.count, fn->parameters.count);

    if (existing->returntypes.count != fn->returntypes.count)
      scu_perror("Function '%s' return type count mismatch\n", fn->name);

    if (fn->kind == FN_DECLARED && existing->kind == FN_DEFINED)
      return;

  } else {
    ht_insert(functions, fn->name, fn);
  }
}

/*
 * @brief: check function call validity (definition)
 *
 * @param fn_call: pointer to the function call node.
 * @param functions: pointer to the functions hash table.
 * @param variables: pointer to the variables hash table (for argument
 * expressions).
 * @param line: line number of the function call.
 */
static void check_function_call(fn_call_node *fn_call, ht *functions,
                                ht *variables, u64 line) {
  if (!fn_call || !fn_call->name)
    return;

  fn_node *fn = ht_search(functions, fn_call->name);

  if (!fn) {
    scu_perror("Call to undeclared function: %s [line %" PRIu64 "]\n",
               fn_call->name, line);
    return;
  }

  if (!fn->is_variadic && fn_call->parameters.count != fn->parameters.count) {
    scu_perror("Function '%s' expects %" PRIu64 " arguments, but %" PRIu64
               " were provided [line %" PRIu64 "]\n",
               fn_call->name, fn->parameters.count, fn_call->parameters.count,
               line);
    return;
  } else if (fn->is_variadic &&
             fn_call->parameters.count < fn->parameters.count) {
    scu_perror("Variadic function '%s' requires at least %" PRIu64
               " fixed arguments [line %" PRIu64 "]\n",
               fn_call->name, fn->parameters.count, line);
    return;
  }

  for (u64 i = 0; i < fn_call->parameters.count && i < fn->parameters.count;
       i++) {
    expr_node arg_expr;
    dynamic_array_get(&fn_call->parameters, i, &arg_expr);

    variable param;
    dynamic_array_get(&fn->parameters, i, &param);

    type arg_type = expr_type(&arg_expr, param.type, variables, functions);

    if (arg_type != param.type && param.type != TYPE_POINTER) {
      scu_perror("Type mismatch in argument %" PRIu64
                 " to function '%s': expected %s, got %s [line %" PRIu64 "]\n",
                 i + 1, fn_call->name, type_to_str(param.type),
                 type_to_str(arg_type), line);
    }
  }
}

/*
 * @brief: check return statement validity within a function
 *
 * @param ret: pointer to the return node.
 * @param fn: pointer to the containing function node.
 * @param variables: pointer to the variables hash table.
 * @param line: line number of the return statement.
 */
static void check_return_statement(return_node *ret, fn_node *fn, ht *variables,
                                   ht *functions, u64 line) {
  if (!ret || !fn)
    return;

  if (ret->returnvals.count != fn->returntypes.count) {
    scu_perror("Function '%s' expects %" PRIu64 " return values, but %" PRIu64
               " were provided [line %" PRIu64 "]\n",
               fn->name, fn->returntypes.count, ret->returnvals.count, line);
    return;
  }

  for (u64 i = 0; i < ret->returnvals.count; i++) {
    expr_node ret_expr;
    dynamic_array_get(&ret->returnvals, i, &ret_expr);

    type expected_type;
    dynamic_array_get(&fn->returntypes, i, &expected_type);

    type actual_type =
        expr_type(&ret_expr, expected_type, variables, functions);
    if (actual_type != expected_type && expected_type != TYPE_POINTER) {
      scu_perror("Return type mismatch in function '%s': expected %s, got %s "
                 "[line %" PRIu64 "]\n",
                 fn->name, type_to_str(expected_type), type_to_str(actual_type),
                 line);
    }
  }
}

/*
 * @brief: register function parameters as variables in the function's local
 * scope
 *
 * @param fn: pointer to the containing function node.
 */
static void register_function_parameters(fn_node *fn) {
  if (fn->kind != FN_DEFINED)
    return;

  for (u64 i = 0; i < fn->parameters.count; i++) {
    variable param;
    dynamic_array_get(&fn->parameters, i, &param);
    param.stack_offset = i;
    dynamic_array_set(&fn->parameters, i, &param);

    ht_insert(fn->defined.variables, param.name, &param);
  }
}

/*
 * @brief: recursively check function body instructions
 *
 * @param fn: pointer to the containing function node.
 * @param functions: pointer to the functions hash table.
 */
static void check_function_body(fn_node *fn, ht *functions) {
  if (fn->kind != FN_DEFINED)
    return;

  register_function_parameters(fn);

  u64 saved_offset = current_stack_offset;
  current_stack_offset = fn->parameters.count;

  for (u64 i = 0; i < fn->defined.instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&fn->defined.instrs, i, &instr);

    instr_check_variables(&instr, fn->defined.variables, functions);
    instr_typecheck(&instr, fn->defined.variables, functions);

    if (instr.kind == INSTR_RETURN) {
      check_return_statement(&instr.ret_node, fn, fn->defined.variables,
                             functions, instr.line);
    }
  }

  current_stack_offset = saved_offset;
}

void check_semantics(dynamic_array *instrs, ht *variables, ht *functions) {
  // Define and declare any / all functions
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_FN_DECLARE || instr.kind == INSTR_FN_DEFINE) {
      // since this is a union anyways
      register_function(&instr.fn_declare_node, functions);
    }

    if (instr.kind == INSTR_FN_CALL)
      check_function_call(&instr.fn_call, functions, variables, instr.line);
  }

  // Validate everything
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_FN_DEFINE) {
      check_function_body(&instr.fn_define_node, functions);
    } else if (instr.kind != INSTR_FN_DECLARE) {
      instr_check_variables(&instr, variables, functions);
      instr_typecheck(&instr, variables, functions);
    }
  }

  // Check labels
  dynamic_array labels;
  dynamic_array_init(&labels, sizeof(char *));
  instrs_check_labels(instrs, &labels);
  dynamic_array_free(&labels);

  scu_check_errors();
}

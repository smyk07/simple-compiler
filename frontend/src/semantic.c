/*
 * semantic: variable checking and type checking for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "frontend/semantic.h"
#include "frontend/ast.h"
#include "frontend/types.h"
#include "frontend/var.h"

#include "core/ds/dynamic_array.h"
#include "core/ds/ht.h"
#include "core/utils.h"

#include <inttypes.h>
#include <stddef.h>

#include <string.h>

u32 evaluate_const_expr(arithmetic_expr_node *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (expr->kind) {
  case AR_EXPR_INVALID:
    scu_punreachable("Invalid arithmetic expression found");
    return 0;
    break;

  case AR_EXPR_TERM:
    if (expr->term.kind == TERM_INT) {
      return expr->term.value.integer;
    }
    scu_perror("Array size must be a constant expression [line %" PRIu64 "]\n",
               expr->line);
    return 0;

  case AR_EXPR_ADD:
    return evaluate_const_expr(expr->binary.left) +
           evaluate_const_expr(expr->binary.right);

  case AR_EXPR_SUBTRACT:
    return evaluate_const_expr(expr->binary.left) -
           evaluate_const_expr(expr->binary.right);

  case AR_EXPR_MULTIPLY:
    return evaluate_const_expr(expr->binary.left) *
           evaluate_const_expr(expr->binary.right);

  case AR_EXPR_DIVIDE: {
    u32 right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      scu_perror("Division by zero in array size [line %" PRIu64 "]\n",
                 expr->line);
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) / right;
  }

  case AR_EXPR_MODULO: {
    u32 right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      scu_perror("Division by zero in array size [line %" PRIu64 "]\n",
                 expr->line);
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) % right;
  }

  case AR_EXPR_UNARY_MINUS: {
    return evaluate_const_expr(expr->unary);
  }
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
static scu_result check_function_call(fn_call_node *fn_call, ht *functions,
                                      ht *variables, u64 line);

/*
 * @brief: check for types in an instr_node (declaration)
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 */
static scu_result instr_typecheck(instr_node *instr, ht *variables,
                                  ht *functions);

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
static scu_result declare_variables(variable *var_to_declare, ht *variables) {
  if (!var_to_declare)
    scu_punreachable("null variable node");
  if (!var_to_declare->name)
    scu_punreachable("uninitizlied variable node");
  if (!variables)
    scu_punreachable("null variables hash table");

  variable *var = ht_search(variables, var_to_declare->name);
  if (var) {
    scu_perror("Re-declaration of variable %s ast line %" PRIu64 "\n",
               var_to_declare->name, var_to_declare->line);
    return SCU_ERR_SEMA;
  }

  var_to_declare->stack_offset = current_stack_offset;
  current_stack_offset += 1;
  ht_insert(variables, var_to_declare->name, var_to_declare);

  return SCU_SUCCESS;
}

/*
 * @brief: insert a new array into the variables hash table.
 *
 * @param var_to_declare: the variable struct to append.
 * @param variables: pointer to the variables hash table.
 */
static scu_result declare_array(variable *arr_to_declare,
                                arithmetic_expr_node *size_expr,
                                ht *variables) {
  if (!arr_to_declare)
    scu_punreachable("null array declaration node");
  if (!arr_to_declare->name)
    scu_punreachable("uninitizlied array declaration node");
  if (!variables)
    scu_punreachable("null variables hash table");

  variable *var = ht_search(variables, arr_to_declare->name);
  if (var) {
    scu_perror("Re-declaration of array %s at line %" PRIu64 "\n",
               arr_to_declare->name, arr_to_declare->line);
    return SCU_ERR_SEMA;
  }

  u32 array_size = evaluate_const_expr(size_expr);
  u64 size_bytes = array_size * type_size(arr_to_declare->type);
  arr_to_declare->stack_offset = current_stack_offset;
  current_stack_offset += size_bytes;

  ht_insert(variables, arr_to_declare->name, arr_to_declare);

  return SCU_SUCCESS;
}

/*
 * @brief: check variables in terms
 *
 * @param term: pointer to a term_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static scu_result term_check_variables(term_node *term, ht *variables,
                                       ht *functions) {
  switch (term->kind) {
  case TERM_IDENTIFIER:
    variable *var = ht_search(variables, term->identifier.name);
    if (!var) {
      scu_perror("Use of undeclared variable: %s [line %" PRIu64 "]\n",
                 term->identifier.name, term->identifier.line);
      return SCU_ERR_SEMA;
    }
    break;

  case TERM_FUNCTION_CALL:
    SCU_TRY(
        check_function_call(&term->fn_call, functions, variables, term->line));
    break;

  default:
    break;
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check variables in expressions
 *
 * @param expr: pointer to an expr_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static scu_result arithmetic_expr_check_variables(arithmetic_expr_node *expr,
                                                  ht *variables,
                                                  ht *functions) {
  switch (expr->kind) {
  case AR_EXPR_INVALID:
    scu_punreachable("Invalid arithmetic expression found");
    break;

  case AR_EXPR_TERM:
    SCU_TRY(term_check_variables(&expr->term, variables, functions));
    break;

  case AR_EXPR_ADD:
  case AR_EXPR_SUBTRACT:
  case AR_EXPR_MULTIPLY:
  case AR_EXPR_DIVIDE:
  case AR_EXPR_MODULO:
    SCU_TRY(arithmetic_expr_check_variables(expr->binary.left, variables,
                                            functions));
    SCU_TRY(arithmetic_expr_check_variables(expr->binary.right, variables,
                                            functions));
    break;

  case AR_EXPR_UNARY_MINUS:
    SCU_TRY(arithmetic_expr_check_variables(expr->unary, variables, functions));
  }

  return SCU_SUCCESS;
}

static scu_result expr_check_variables(expr_node *expr, ht *variables,
                                       ht *functions);

/*
 * @brief: check variables in relational expressions
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static scu_result rel_check_variables(rel_node *rel, ht *variables,
                                      ht *functions) {
  SCU_TRY(term_check_variables(&rel->comparison.lhs, variables, functions));
  SCU_TRY(term_check_variables(&rel->comparison.rhs, variables, functions));

  return SCU_SUCCESS;
}

static scu_result logical_check_variables(logical_node *log, ht *variables,
                                          ht *functions) {
  switch (log->kind) {
  case LOG_INVALID:
    scu_punreachable("Invalid logical expression found");
    break;

  case LOG_AND:
  case LOG_OR:
    SCU_TRY(expr_check_variables(log->binary.lhs, variables, functions));
    SCU_TRY(expr_check_variables(log->binary.rhs, variables, functions));
    break;

  case LOG_NOT:
    SCU_TRY(expr_check_variables(log->unary.operand, variables, functions));
    break;
  }

  return SCU_SUCCESS;
}

static scu_result expr_check_variables(expr_node *expr, ht *variables,
                                       ht *functions) {
  switch (expr->kind) {
  case EXPR_INVALID:
    scu_punreachable("Invalid expression found");
    break;

  case EXPR_TERM:
    SCU_TRY(term_check_variables(&expr->term, variables, functions));
    break;
  case EXPR_LOGICAL:
    SCU_TRY(logical_check_variables(&expr->logical, variables, functions));
    break;
  case EXPR_RELATIONAL:
    SCU_TRY(rel_check_variables(&expr->relational, variables, functions));
    break;
  case EXPR_BOOL:
    break;
  }

  return SCU_SUCCESS;
}

static scu_result instr_check_variables(instr_node *instr, ht *variables,
                                        ht *functions);

static scu_result cond_block_check_variables(cond_block_node *blk,
                                             ht *variables, ht *functions) {
  if (!blk)
    scu_punreachable("Null conditional block node");
  if (!variables)
    scu_punreachable("Null variables hash table");
  if (!functions)
    scu_punreachable("Null functions hash table");

  if (blk->kind == COND_SINGLE_INSTR) {
    SCU_TRY(instr_check_variables(blk->single, variables, functions));
    SCU_TRY(instr_typecheck(blk->single, variables, functions));
  } else {
    for (u64 i = 0; i < blk->multi.count; i++) {
      instr_node instr_;
      dynamic_array_get(&blk->multi, i, &instr_);
      SCU_TRY(instr_check_variables(&instr_, variables, functions));
      SCU_TRY(instr_typecheck(&instr_, variables, functions));
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check loop validity and body instructions
 *
 * @param loop: pointer to the loop node.
 * @param variables: pointer to the parent scope's variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static scu_result check_loop(loop_node *loop, ht *parent_variables,
                             ht *functions) {
  if (!loop)
    scu_punreachable("null loop_node");
  if (!parent_variables)
    scu_punreachable("null parent_variables hash table");
  if (!functions)
    scu_punreachable("null functions hash table");

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
    SCU_TRY(expr_check_variables(&loop->conditional.break_condition,
                                 loop->variables, functions));
  } else if (loop->kind == LOOP_FOR) {
    SCU_TRY(arithmetic_expr_check_variables(loop->_for.range_start,
                                            loop->variables, functions));
    SCU_TRY(arithmetic_expr_check_variables(loop->_for.range_end,
                                            loop->variables, functions));

    SCU_TRY(declare_variables(&loop->_for.iterator, loop->variables));
  }

  for (u64 i = 0; i < loop->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&loop->instrs, i, &instr);

    SCU_TRY(instr_check_variables(&instr, loop->variables, functions));
    SCU_TRY(instr_typecheck(&instr, loop->variables, functions));
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check variables in an individual instruction
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 * @param functions: pointer to the functions hash table.
 */
static scu_result instr_check_variables(instr_node *instr, ht *variables,
                                        ht *functions) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    SCU_TRY(declare_variables(&instr->declare_variable, variables));
    break;

  case INSTR_INITIALIZE:
    if (instr->initialize_variable.var.type == TYPE_BOOL) {
      SCU_TRY(expr_check_variables(&instr->initialize_variable.boolean,
                                   variables, functions));
    } else {
      SCU_TRY(arithmetic_expr_check_variables(
          instr->initialize_variable.arithmetic, variables, functions));
    }
    SCU_TRY(declare_variables(&instr->initialize_variable.var, variables));
    break;

  case INSTR_DECLARE_ARRAY:
    SCU_TRY(declare_array(&instr->declare_array.var,
                          instr->declare_array.size_expr, variables));
    break;

  case INSTR_INITIALIZE_ARRAY:
    SCU_TRY(declare_array(&instr->initialize_array.var,
                          instr->initialize_array.size_expr, variables));
    for (u64 i = 0; i < instr->initialize_array.literal.elements.count; i++) {
      arithmetic_expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);
      SCU_TRY(arithmetic_expr_check_variables(&elem, variables, functions));
    }
    break;

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    variable *arr =
        ht_search(variables, instr->assign_to_array_subscript.var.name);

    if (!arr) {
      scu_perror("Use of undeclared array: %s [line %" PRIu64 "]\n",
                 instr->assign_to_array_subscript.var.name,
                 instr->assign_to_array_subscript.var.line);
      return SCU_ERR_SEMA;
    }

    SCU_TRY(arithmetic_expr_check_variables(
        instr->assign_to_array_subscript.index_expr, variables, functions));

    SCU_TRY(arithmetic_expr_check_variables(
        instr->assign_to_array_subscript.expr_to_assign, variables, functions));
    break;

  case INSTR_ASSIGN:
    SCU_TRY(arithmetic_expr_check_variables(instr->assign.expr, variables,
                                            functions));
    break;

  case INSTR_IF:
    SCU_TRY(expr_check_variables(&instr->if_.condition, variables, functions));
    SCU_TRY(cond_block_check_variables(&instr->if_.then, variables, functions));
    if (instr->if_.else_)
      SCU_TRY(
          cond_block_check_variables(instr->if_.else_, variables, functions));
    break;

  case INSTR_MATCH:
    SCU_TRY(arithmetic_expr_check_variables(instr->match.expr, variables,
                                            functions));

    for (u64 i = 0; i < instr->match.cases.count; i++) {
      match_case_node case_node;
      dynamic_array_get(&instr->match.cases, i, &case_node);

      switch (case_node.kind) {
      case MATCH_CASE_INVALID:
        scu_punreachable("Invalid match case node found");
        break;

      case MATCH_CASE_VALUES:
        for (u64 j = 0; j < case_node.values.values.count; j++) {
          arithmetic_expr_node *expr;
          dynamic_array_get(&case_node.values.values, j, &expr);
          SCU_TRY(arithmetic_expr_check_variables(expr, variables, functions));
        }
        break;
      case MATCH_CASE_RANGE:
        SCU_TRY(arithmetic_expr_check_variables(case_node.range.start,
                                                variables, functions));
        SCU_TRY(arithmetic_expr_check_variables(case_node.range.end, variables,
                                                functions));
        break;
      case MATCH_CASE_DEFAULT:
        break;
      }

      SCU_TRY(
          cond_block_check_variables(&case_node.body, variables, functions));
    }
    break;

  case INSTR_LOOP:
    SCU_TRY(check_loop(&instr->loop, variables, functions));
    break;

  default:
    break;
  }

  return SCU_SUCCESS;
}

/*
 * @brief: make sure labels are properly declared and not duplicated
 *
 * @param labels: pointer to the labels dynamic_array.
 * @param instr: pointer to an instr_node.
 */
static scu_result check_label(dynamic_array *labels, instr_node *instr) {
  const char *label_name = instr->label.label;
  for (u64 i = 0; i < labels->count; i++) {
    char *existing;
    dynamic_array_get(labels, i, &existing);
    if (strcmp(label_name, existing) == 0) {
      scu_perror("Duplicate label declaration: %s [line %" PRIu64 "]\n",
                 label_name, instr->line);
      return SCU_ERR_SEMA;
    }
  }
  dynamic_array_append(labels, &label_name);

  return SCU_SUCCESS;
}

/*
 * @brief: make sure labels are properly used in goto instructions
 *
 * @param labels: pointer to the labels dynamic_array.
 * @param instr: pointer to an instr_node.
 */
static scu_result check_goto(dynamic_array *labels, instr_node *instr) {
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
    scu_perror("Use of undeclared label: %s [line %" PRIu64 "]\n",
               instr->goto_.label, instr->line);
    return SCU_ERR_SEMA;
  }

  return SCU_SUCCESS;
}

static scu_result instrs_check_labels(dynamic_array *instrs,
                                      dynamic_array *labels);

static scu_result cond_block_check_labels(cond_block_node *block,
                                          dynamic_array *labels) {
  if (!block)
    scu_punreachable("null conditional block node");

  if (block->kind == COND_SINGLE_INSTR) {
    instr_node *instr = block->single;

    if (instr->kind == INSTR_LABEL)
      SCU_TRY(check_label(labels, instr));
    else if (instr->kind == INSTR_GOTO)
      SCU_TRY(check_goto(labels, instr));
    else if (instr->kind == INSTR_IF)
      SCU_TRY(instrs_check_labels(
          &((dynamic_array){.items = instr, .count = 1, .capacity = 1}),
          labels));
  } else {
    for (u64 i = 0; i < block->multi.count; i++) {
      instr_node instr;
      dynamic_array_get(&block->multi, i, &instr);

      if (instr.kind == INSTR_LABEL)
        SCU_TRY(check_label(labels, &instr));
      else if (instr.kind == INSTR_GOTO)
        SCU_TRY(check_goto(labels, &instr));
      else if (instr.kind == INSTR_IF)
        SCU_TRY(cond_block_check_labels(&instr.if_.then, labels));
      SCU_TRY(cond_block_check_labels(instr.if_.else_, labels));
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check for declaration of labels AND the use of labels in goto
 * instructions
 *
 * @param instr: pointer to an instr_node.
 * @param labels: pointer to the labels dynamic_array.
 */
static scu_result instrs_check_labels(dynamic_array *instrs,
                                      dynamic_array *labels) {
  // check labels first
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_LABEL)
      SCU_TRY(check_label(labels, &instr));
  }

  // then check goto
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_GOTO)
      SCU_TRY(check_goto(labels, &instr));
  }

  // then check if
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_IF) {
      SCU_TRY(cond_block_check_labels(&instr.if_.then, labels));
      if (instr.if_.else_)
        SCU_TRY(cond_block_check_labels(instr.if_.else_, labels));
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check for types in an expr_node (declaration)
 *
 * @param expr: pointer to an expr_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables hash table.
 */
static type arithmetic_expr_type(arithmetic_expr_node *expr, type target_type,
                                 ht *variables, ht *functions);

static bool type_is_integer(type t) {
  switch (t) {
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
    return true;
  default:
    return false;
  }
}

/*
 * @brief: check for types in a term_node
 *
 * @param term: pointer to a term_node.
 * @param expected: type expected from term.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables hash table.
 *
 * @return type of the term
 */
static type term_type(term_node *term, type expected, ht *variables,
                      ht *functions) {
  switch (term->kind) {
  case TERM_INVALID:
    scu_punreachable("Invalid term found");
    break;

  case TERM_INT:
    if (type_is_integer(expected))
      return expected;
    return TYPE_I32;

  case TERM_CHAR:
    return TYPE_CHAR;

  case TERM_STRING:
    return TYPE_STRING;

  case TERM_POINTER:
  case TERM_DEREF:
  case TERM_ADDOF:
  case TERM_IDENTIFIER:
    return get_var_type(variables, &term->identifier);

  case TERM_ARRAY_ACCESS: {
    type array_type = get_var_type(variables, &term->array_access.array_var);
    if (array_type == TYPE_VOID) {
      scu_perror("Array '%s' not declared [line %" PRIu64 "]\n",
                 term->array_access.array_var.name, term->line);
      return TYPE_VOID;
    }
    type index_type = arithmetic_expr_type(term->array_access.index_expr,
                                           TYPE_I32, variables, functions);
    if (index_type != TYPE_I32) {
      scu_perror("Array index must be of type int, got type at [line %" PRIu64
                 "]\n",
                 term->line);
    }
    return array_type;
  }

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
      return TYPE_VOID;
    } else if (fn->is_variadic &&
               term->fn_call.parameters.count < fn->parameters.count) {
      scu_perror("Variadic function '%s' requires at least %" PRIu64
                 " fixed arguments [line %" PRIu64 "]\n",
                 term->fn_call.name, fn->parameters.count, term->line);
      return TYPE_VOID;
    }

    for (u64 i = 0;
         i < term->fn_call.parameters.count && i < fn->parameters.count; i++) {
      arithmetic_expr_node arg_expr;
      dynamic_array_get(&term->fn_call.parameters, i, &arg_expr);

      variable param;
      dynamic_array_get(&fn->parameters, i, &param);

      type arg_type =
          arithmetic_expr_type(&arg_expr, param.type, variables, functions);

      if (arg_type != param.type) {
        if (!(param.type == TYPE_POINTER &&
              (arg_type == TYPE_STRING || arg_type == TYPE_POINTER))) {
          scu_perror("Type mismatch in argument %" PRIu64
                     " to function '%s': expected %s, got %s [line %" PRIu64
                     "]\n",
                     i + 1, term->fn_call.name, type_to_str(param.type),
                     type_to_str(arg_type), term->line);
          return TYPE_VOID;
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
static type arithmetic_expr_type(arithmetic_expr_node *expr, type target_type,
                                 ht *variables, ht *functions) {
  type lhs = {0};
  type rhs = {0};

  switch (expr->kind) {
  case AR_EXPR_INVALID:
    scu_punreachable("Invalid arithmetic expression found");
    break;

  case AR_EXPR_TERM:
    return term_type(&expr->term, target_type, variables, functions);

  case AR_EXPR_ADD:
  case AR_EXPR_SUBTRACT:
  case AR_EXPR_MULTIPLY:
  case AR_EXPR_DIVIDE:
  case AR_EXPR_MODULO:
    lhs = arithmetic_expr_type(expr->binary.left, target_type, variables,
                               functions);
    rhs = arithmetic_expr_type(expr->binary.right, target_type, variables,
                               functions);
    break;

  case AR_EXPR_UNARY_MINUS:
    return arithmetic_expr_type(expr->unary, target_type, variables, functions);
  }

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror("Type mismatch in arithmetic expression: %s vs %s [line %" PRIu64
               "]\n",
               lhs_type_str, rhs_type_str, expr->line);
  }

  return lhs;
}

static scu_result expr_typecheck(expr_node *expr, ht *variables, ht *functions);

/*
 * @brief: check for types in a rel_node
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables hash table.
 */
static scu_result rel_typecheck(rel_node *rel, ht *variables, ht *functions) {
  type lhs, rhs;

  lhs = term_type(&rel->comparison.lhs, TYPE_INVALID, variables, functions);
  rhs = term_type(&rel->comparison.rhs, lhs, variables, functions);

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror("Type mismatch in conditional statement: %s vs %s [line %" PRIu64
               "]\n",
               lhs_type_str, rhs_type_str, rel->line);
    return SCU_ERR_SEMA;
  }

  return SCU_SUCCESS;
}

static scu_result logical_typecheck(logical_node *log, ht *variables,
                                    ht *functions) {
  switch (log->kind) {
  case LOG_INVALID:
    scu_punreachable("Invalid logical expression found");
    break;

  case LOG_AND:
  case LOG_OR:
    SCU_TRY(expr_typecheck(log->binary.lhs, variables, functions));
    SCU_TRY(expr_typecheck(log->binary.rhs, variables, functions));
    break;

  case LOG_NOT:
    SCU_TRY(expr_typecheck(log->unary.operand, variables, functions));
    break;
  }

  return SCU_SUCCESS;
}

static scu_result expr_typecheck(expr_node *expr, ht *variables,
                                 ht *functions) {
  switch (expr->kind) {
  case EXPR_INVALID:
    scu_punreachable("Invalid expression found");
    break;

  case EXPR_TERM: {
    type t = term_type(&expr->term, TYPE_BOOL, variables, functions);
    if (t != TYPE_BOOL) {
      scu_perror("Expected a boolean expression, got %s [line %" PRIu64 "]\n",
                 type_to_str(t), expr->term.line);
      return SCU_ERR_SEMA;
    }
    break;
  }

  case EXPR_LOGICAL:
    SCU_TRY(logical_typecheck(&expr->logical, variables, functions));
    break;

  case EXPR_RELATIONAL:
    SCU_TRY(rel_typecheck(&expr->relational, variables, functions));
    break;

  case EXPR_BOOL:
    break;
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check for types in an instr_node
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables hash table.
 */
static scu_result instr_typecheck(instr_node *instr, ht *variables,
                                  ht *functions) {
  if (!instr)
    scu_punreachable("null instr node");
  if (!functions)
    scu_punreachable("null functions hash table");
  if (!variables)
    scu_punreachable("null variables hash table");

  switch (instr->kind) {
  case INSTR_INITIALIZE: {
    type target_type = instr->initialize_variable.var.type;

    if (target_type == TYPE_BOOL) {
      SCU_TRY(expr_typecheck(&instr->initialize_variable.boolean, variables,
                             functions));
      break;
    }

    type expr_result =
        arithmetic_expr_type(instr->initialize_variable.arithmetic, target_type,
                             variables, functions);

    if (target_type == TYPE_POINTER) {
      return SCU_SUCCESS;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror(
          "Type mismatch in initialization to %s - %s to %s [line %" PRIu64
          "]\n",
          instr->assign.identifier.name, expr_result_str, target_type_str,
          instr->line);
      return SCU_ERR_SEMA;
    }
    break;
  }

  case INSTR_INITIALIZE_ARRAY: {
    type array_type = instr->initialize_array.var.type;
    for (u64 i = 0; i < instr->initialize_array.literal.elements.count; i++) {
      arithmetic_expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);
      type elem_type =
          arithmetic_expr_type(&elem, array_type, variables, functions);
      if (array_type != elem_type && array_type != TYPE_POINTER) {
        const char *array_type_str = type_to_str(array_type);
        const char *elem_type_str = type_to_str(elem_type);
        scu_perror("Type mismatch in array initialization - element %" PRIu64
                   " is %s but array is %s [line %" PRIu64 "]\n",
                   i, elem_type_str, array_type_str, instr->line);
        return SCU_ERR_SEMA;
      }
    }
    break;
  }

  case INSTR_ASSIGN: {
    type target_type = get_var_type(variables, &instr->assign.identifier);
    type expr_result = arithmetic_expr_type(instr->assign.expr, target_type,
                                            variables, functions);
    if (target_type == TYPE_POINTER) {
      return SCU_SUCCESS;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror("Type mismatch in assignment to %s - %s to %s [line %" PRIu64
                 "]\n",
                 instr->assign.identifier.name, expr_result_str,
                 target_type_str, instr->line);
      return SCU_ERR_SEMA;
    }
    break;
  }

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT: {
    type array_type =
        get_var_type(variables, &instr->assign_to_array_subscript.var);

    type index_type =
        arithmetic_expr_type(instr->assign_to_array_subscript.index_expr,
                             TYPE_I32, variables, functions);
    if (index_type != TYPE_I32) {
      scu_perror("Array index must be of type int, got %s [line %" PRIu64 "]\n",
                 type_to_str(index_type), instr->line);
      return SCU_ERR_SEMA;
    }

    type expr_result =
        arithmetic_expr_type(instr->assign_to_array_subscript.expr_to_assign,
                             array_type, variables, functions);
    if (array_type != expr_result && array_type != TYPE_POINTER) {
      const char *array_type_str = type_to_str(array_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror(
          "Type mismatch in array assignment to %s - %s to %s [line %" PRIu64
          "]\n",
          instr->assign_to_array_subscript.var.name, expr_result_str,
          array_type_str, instr->line);
      return SCU_ERR_SEMA;
    }
    break;
  }

  case INSTR_IF: {
    SCU_TRY(expr_typecheck(&instr->if_.condition, variables, functions));
    if (instr->if_.else_ifs.count > 0) {
      for (u64 i = 0; i < instr->if_.else_ifs.count; i++) {
        if_node else_if_node;
        dynamic_array_get(&instr->if_.else_ifs, i, &else_if_node);
        SCU_TRY(expr_typecheck(&else_if_node.condition, variables, functions));
      }
    }
    break;
  }

  case INSTR_MATCH: {
    type match_expr_type =
        arithmetic_expr_type(instr->match.expr, TYPE_I32, variables, functions);

    for (u64 i = 0; i < instr->match.cases.count; i++) {
      match_case_node case_node;
      dynamic_array_get(&instr->match.cases, i, &case_node);

      switch (case_node.kind) {
      case MATCH_CASE_INVALID:
        scu_punreachable("Invalid match case node found");
        break;

      case MATCH_CASE_VALUES: {
        for (u64 j = 0; j < case_node.values.values.count; j++) {
          arithmetic_expr_node *expr;
          dynamic_array_get(&case_node.values.values, j, &expr);
          type value_type =
              arithmetic_expr_type(expr, match_expr_type, variables, functions);

          if (value_type != match_expr_type) {
            const char *match_type_str = type_to_str(match_expr_type);
            const char *value_type_str = type_to_str(value_type);
            scu_perror("Type mismatch in match case - expected %s but got %s "
                       "[line %" PRIu64 "]\n",
                       match_type_str, value_type_str, instr->line);
            return SCU_ERR_SEMA;
          }
        }
        break;
      }

      case MATCH_CASE_RANGE: {
        type start_type = arithmetic_expr_type(
            case_node.range.start, match_expr_type, variables, functions);
        if (start_type != match_expr_type) {
          const char *match_type_str = type_to_str(match_expr_type);
          const char *start_type_str = type_to_str(start_type);
          scu_perror("Type mismatch in match range start - expected %s but got "
                     "%s [line %" PRIu64 "]\n",
                     match_type_str, start_type_str, instr->line);
          return SCU_ERR_SEMA;
        }

        type end_type = arithmetic_expr_type(
            case_node.range.end, match_expr_type, variables, functions);
        if (end_type != match_expr_type) {
          const char *match_type_str = type_to_str(match_expr_type);
          const char *end_type_str = type_to_str(end_type);
          scu_perror("Type mismatch in match range end - expected %s but got "
                     "%s [line %" PRIu64 "]\n",
                     match_type_str, end_type_str, instr->line);
          return SCU_ERR_SEMA;
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

  return SCU_SUCCESS;
}

/*
 * @brief: insert a new function into the functions hash table.
 *
 * @param fn: pointer to the function node to register.
 * @param functions: pointer to the functions hash table.
 */
static scu_result register_function(fn_node *fn, ht *functions) {
  if (!fn)
    scu_punreachable("null fn_node");
  if (!fn->name)
    scu_punreachable("uninitizlied fn_node");
  if (!functions)
    scu_punreachable("null functions hash table");

  fn_node *existing = ht_search(functions, fn->name);
  if (existing) {
    if (existing->is_variadic != fn->is_variadic) {
      scu_perror("Function '%s' variadic mismatch\n", fn->name);
      return SCU_ERR_SEMA;
    }

    if (!fn->is_variadic &&
        existing->parameters.count != fn->parameters.count) {
      scu_perror(
          "Function '%s' parameter count mismatch: declared with %" PRIu64
          ", but has %" PRIu64 "\n",
          fn->name, existing->parameters.count, fn->parameters.count);
      return SCU_ERR_SEMA;
    }

    if (fn->kind == FN_DEFINED && existing->kind == FN_DEFINED) {
      scu_perror("Duplicate function definition: %s\n", fn->name);
      return SCU_ERR_SEMA;
    }

    if (existing->parameters.count != fn->parameters.count) {
      scu_perror(
          "Function '%s' parameter count mismatch: declared with %" PRIu64
          ", but has %" PRIu64 "\n",
          fn->name, existing->parameters.count, fn->parameters.count);
      return SCU_ERR_SEMA;
    }

    if (existing->returntypes.count != fn->returntypes.count) {
      scu_perror("Function '%s' return type count mismatch\n", fn->name);
      return SCU_ERR_SEMA;
    }

    if (fn->kind == FN_DECLARED && existing->kind == FN_DEFINED)
      return SCU_ERR_SEMA;

  } else {
    ht_insert(functions, fn->name, fn);
  }

  return SCU_SUCCESS;
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
static scu_result check_function_call(fn_call_node *fn_call, ht *functions,
                                      ht *variables, u64 line) {
  if (!fn_call)
    scu_punreachable("null fn_call_node");
  if (!fn_call->name)
    scu_punreachable("uninitizlied fn_call_node");
  if (!functions)
    scu_punreachable("null functions hash table");
  if (!variables)
    scu_punreachable("null variables hash table");

  fn_node *fn = ht_search(functions, fn_call->name);

  if (!fn) {
    scu_perror("Call to undeclared function: %s [line %" PRIu64 "]\n",
               fn_call->name, line);
    return SCU_ERR_SEMA;
  }

  if (!fn->is_variadic && fn_call->parameters.count != fn->parameters.count) {
    scu_perror("Function '%s' expects %" PRIu64 " arguments, but %" PRIu64
               " were provided [line %" PRIu64 "]\n",
               fn_call->name, fn->parameters.count, fn_call->parameters.count,
               line);
    return SCU_ERR_SEMA;
  }

  else if (fn->is_variadic &&
           fn_call->parameters.count < fn->parameters.count) {
    scu_perror("Variadic function '%s' requires at least %" PRIu64
               " fixed arguments [line %" PRIu64 "]\n",
               fn_call->name, fn->parameters.count, line);
    return SCU_ERR_SEMA;
  }

  for (u64 i = 0; i < fn_call->parameters.count && i < fn->parameters.count;
       i++) {
    arithmetic_expr_node arg_expr;
    dynamic_array_get(&fn_call->parameters, i, &arg_expr);

    variable param;
    dynamic_array_get(&fn->parameters, i, &param);

    type arg_type =
        arithmetic_expr_type(&arg_expr, param.type, variables, functions);

    if (arg_type != param.type && param.type != TYPE_POINTER) {
      scu_perror("Type mismatch in argument %" PRIu64
                 " to function '%s': expected %s, got %s [line %" PRIu64 "]\n",
                 i + 1, fn_call->name, type_to_str(param.type),
                 type_to_str(arg_type), line);
      return SCU_ERR_SEMA;
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: check return statement validity within a function
 *
 * @param ret: pointer to the return node.
 * @param fn: pointer to the containing function node.
 * @param variables: pointer to the variables hash table.
 * @param line: line number of the return statement.
 */
static scu_result check_return_statement(return_node *ret, fn_node *fn,
                                         ht *variables, ht *functions,
                                         u64 line) {
  if (!ret)
    scu_punreachable("null return_node");
  if (!fn)
    scu_punreachable("null function node");

  if (ret->returnvals.count != fn->returntypes.count) {
    scu_perror("Function '%s' expects %" PRIu64 " return values, but %" PRIu64
               " were provided [line %" PRIu64 "]\n",
               fn->name, fn->returntypes.count, ret->returnvals.count, line);
    return SCU_ERR_SEMA;
  }

  for (u64 i = 0; i < ret->returnvals.count; i++) {
    arithmetic_expr_node ret_expr;
    dynamic_array_get(&ret->returnvals, i, &ret_expr);

    type expected_type;
    dynamic_array_get(&fn->returntypes, i, &expected_type);

    type actual_type =
        arithmetic_expr_type(&ret_expr, expected_type, variables, functions);
    if (actual_type != expected_type && expected_type != TYPE_POINTER) {
      scu_perror("Return type mismatch in function '%s': expected %s, got %s "
                 "[line %" PRIu64 "]\n",
                 fn->name, type_to_str(expected_type), type_to_str(actual_type),
                 line);
      return SCU_ERR_SEMA;
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: recursively check function body instructions
 *
 * @param fn: pointer to the containing function node.
 * @param functions: pointer to the functions hash table.
 */
static scu_result check_function_body(fn_node *fn, ht *functions) {
  if (!fn)
    scu_punreachable("null fn_node");
  if (!fn->name)
    scu_punreachable("uninitizlied fn_node");
  if (!functions)
    scu_punreachable("null functions hash table");

  for (u64 i = 0; i < fn->parameters.count; i++) {
    variable param;
    dynamic_array_get(&fn->parameters, i, &param);
    param.stack_offset = i;
    dynamic_array_set(&fn->parameters, i, &param);

    ht_insert(fn->defined.variables, param.name, &param);
  }
  u64 saved_offset = current_stack_offset;
  current_stack_offset = fn->parameters.count;

  for (u64 i = 0; i < fn->defined.instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&fn->defined.instrs, i, &instr);

    SCU_TRY(instr_check_variables(&instr, fn->defined.variables, functions));
    SCU_TRY(instr_typecheck(&instr, fn->defined.variables, functions));

    if (instr.kind == INSTR_RETURN) {
      SCU_TRY(check_return_statement(&instr.ret_node, fn, fn->defined.variables,
                                     functions, instr.line));
    }
  }

  current_stack_offset = saved_offset;

  return SCU_SUCCESS;
}

scu_result check_semantics(dynamic_array *instrs, ht *variables,
                           ht *functions) {
  // Define and declare any / all functions
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_FN_DECLARE || instr.kind == INSTR_FN_DEFINE) {
      // since this is a union anyways
      SCU_TRY(register_function(&instr.fn_declare_node, functions));
    }

    if (instr.kind == INSTR_FN_CALL)
      SCU_TRY(check_function_call(&instr.fn_call, functions, variables,
                                  instr.line));
  }

  // Validate everything
  for (u64 i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_FN_DEFINE) {
      SCU_TRY(check_function_body(&instr.fn_define_node, functions));
    } else if (instr.kind != INSTR_FN_DECLARE) {
      SCU_TRY(instr_check_variables(&instr, variables, functions));
      SCU_TRY(instr_typecheck(&instr, variables, functions));
    }
  }

  // Check labels
  dynamic_array labels;
  dynamic_array_init(&labels, sizeof(char *));
  SCU_TRY(instrs_check_labels(instrs, &labels));
  dynamic_array_free(&labels);

  return SCU_SUCCESS;
}

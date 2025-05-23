#include "data_structures.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#include <string.h>

// Variable tracking
int find_variables(dynamic_array *variables, variable *var_to_find,
                   int *errors) {
  for (unsigned int i = 0; i < variables->count; i++) {
    variable var;
    dynamic_array_get(variables, i, &var);

    if (strcmp(var_to_find->name, var.name) == 0) {
      return i;
    }
  }

  scu_perror(errors, "Use of undeclared variable: %s\n", var_to_find->name);
  return -1;
}

void declare_variables(variable *var_to_declare, dynamic_array *variables) {
  if (!var_to_declare || !var_to_declare->name || !variables)
    return;

  for (unsigned int i = 0; i < variables->count; i++) {
    variable var;
    if (dynamic_array_get(variables, i, &var) != 0)
      continue;

    if (var.name && strcmp(var_to_declare->name, var.name) == 0) {
      return;
    }
  }

  dynamic_array_append(variables, var_to_declare);
}

void term_check_variables(term_node *term, dynamic_array *variables,
                          int *errors) {
  switch (term->kind) {
  case TERM_INPUT:
    break;
  case TERM_INT:
    break;
  case TERM_CHAR:
    break;
  case TERM_IDENTIFIER:
    find_variables(variables, &term->identifier, errors);
    break;
  }
}

void expr_check_variables(expr_node *expr, dynamic_array *variables,
                          int *errors) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_check_variables(&expr->term, variables, errors);
    break;
  case EXPR_ADD:
    term_check_variables(&expr->add.lhs, variables, errors);
    term_check_variables(&expr->add.rhs, variables, errors);
    break;
  case EXPR_SUBTRACT:
    term_check_variables(&expr->subtract.lhs, variables, errors);
    term_check_variables(&expr->subtract.rhs, variables, errors);
    break;
  case EXPR_MULTIPLY:
    term_check_variables(&expr->multiply.lhs, variables, errors);
    term_check_variables(&expr->multiply.rhs, variables, errors);
    break;
  case EXPR_DIVIDE:
    term_check_variables(&expr->divide.lhs, variables, errors);
    term_check_variables(&expr->divide.rhs, variables, errors);
    break;
  case EXPR_MODULO:
    term_check_variables(&expr->modulo.lhs, variables, errors);
    term_check_variables(&expr->modulo.rhs, variables, errors);
    break;
  }
}

void rel_check_variables(rel_node *rel, dynamic_array *variables, int *errors) {
  switch (rel->kind) {
  case REL_IS_EQUAL:
    term_check_variables(&rel->is_equal.lhs, variables, errors);
    term_check_variables(&rel->is_equal.rhs, variables, errors);
    break;
  case REL_NOT_EQUAL:
    term_check_variables(&rel->not_equal.lhs, variables, errors);
    term_check_variables(&rel->not_equal.rhs, variables, errors);
    break;
  case REL_LESS_THAN:
    term_check_variables(&rel->less_than.lhs, variables, errors);
    term_check_variables(&rel->less_than.rhs, variables, errors);
    break;
  case REL_LESS_THAN_OR_EQUAL:
    term_check_variables(&rel->less_than_or_equal.lhs, variables, errors);
    term_check_variables(&rel->less_than_or_equal.rhs, variables, errors);
    break;
  case REL_GREATER_THAN:
    term_check_variables(&rel->greater_than.lhs, variables, errors);
    term_check_variables(&rel->greater_than.rhs, variables, errors);
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    term_check_variables(&rel->greater_than_or_equal.lhs, variables, errors);
    term_check_variables(&rel->greater_than_or_equal.rhs, variables, errors);
    break;
  }
}

void instr_check_variables(instr_node *instr, dynamic_array *variables,
                           int *errors) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    declare_variables(&instr->declare_variable, variables);
    break;
  case INSTR_ASSIGN:
    expr_check_variables(&instr->assign.expr, variables, errors);
    break;
  case INSTR_IF:
    rel_check_variables(&instr->if_.rel, variables, errors);
    instr_check_variables(instr->if_.instr, variables, errors);
    break;
  case INSTR_GOTO:
    break;
  case INSTR_OUTPUT:
    term_check_variables(&instr->output.term, variables, errors);
    break;
  case INSTR_LABEL:
    break;
  }
}

// Typechecking
token_kind var_type(char *name, dynamic_array *variables, int *errors) {
  for (unsigned int i = 0; i < variables->count; i++) {
    variable var;
    dynamic_array_get(variables, i, &var);

    if (strcmp(name, var.name) == 0) {
      return var.type;
    }
  }
  scu_perror(errors, "Use of undeclared variable: %s\n", name);
  return -1;
}

token_kind term_type(term_node *term, token_kind target_type,
                     dynamic_array *variables, int *errors) {
  switch (term->kind) {
  case TERM_INPUT: {
    return target_type;
    break;
  }
  case TERM_INT:
    return TOKEN_TYPE_INT;
    break;
  case TERM_CHAR:
    return TOKEN_TYPE_CHAR;
    break;
  case TERM_IDENTIFIER:
    return var_type(term->identifier.name, variables, errors);
    break;
  default:
    return -1;
  }
}

token_kind expr_type(expr_node *expr, token_kind target_type,
                     dynamic_array *variables, int *errors) {
  token_kind lhs, rhs;

  switch (expr->kind) {
  case EXPR_TERM:
    return term_type(&expr->term, target_type, variables, errors);
  case EXPR_ADD:
    lhs = term_type(&expr->add.lhs, target_type, variables, errors);
    rhs = term_type(&expr->add.rhs, target_type, variables, errors);
    break;
  case EXPR_SUBTRACT:
    lhs = term_type(&expr->subtract.lhs, target_type, variables, errors);
    rhs = term_type(&expr->subtract.rhs, target_type, variables, errors);
    break;
  case EXPR_MULTIPLY:
    lhs = term_type(&expr->multiply.lhs, target_type, variables, errors);
    rhs = term_type(&expr->multiply.rhs, target_type, variables, errors);
    break;
  case EXPR_DIVIDE:
    lhs = term_type(&expr->divide.lhs, target_type, variables, errors);
    rhs = term_type(&expr->divide.rhs, target_type, variables, errors);
    break;
  case EXPR_MODULO:
    lhs = term_type(&expr->modulo.lhs, target_type, variables, errors);
    rhs = term_type(&expr->modulo.rhs, target_type, variables, errors);
    break;
  }

  if (lhs != rhs) {
    scu_perror(errors, "Type mismatch in arithmetic expression: %d vs %d\n",
               lhs, rhs);
  }
  return lhs;
}

void rel_typecheck(rel_node *rel, dynamic_array *variables, int *errors) {
  token_kind lhs, rhs;

  switch (rel->kind) {
  case REL_IS_EQUAL:
    lhs = term_type(&rel->is_equal.lhs, TOKEN_NULL, variables, errors);
    rhs = term_type(&rel->is_equal.rhs, TOKEN_NULL, variables, errors);
    break;
  case REL_NOT_EQUAL:
    lhs = term_type(&rel->not_equal.lhs, TOKEN_NULL, variables, errors);
    rhs = term_type(&rel->not_equal.rhs, TOKEN_NULL, variables, errors);
    break;
  case REL_LESS_THAN:
    lhs = term_type(&rel->less_than.lhs, TOKEN_NULL, variables, errors);
    rhs = term_type(&rel->less_than.rhs, TOKEN_NULL, variables, errors);
    break;
  case REL_LESS_THAN_OR_EQUAL:
    lhs =
        term_type(&rel->less_than_or_equal.lhs, TOKEN_NULL, variables, errors);
    rhs =
        term_type(&rel->less_than_or_equal.rhs, TOKEN_NULL, variables, errors);
    break;
  case REL_GREATER_THAN:
    lhs = term_type(&rel->greater_than.lhs, TOKEN_NULL, variables, errors);
    rhs = term_type(&rel->greater_than.rhs, TOKEN_NULL, variables, errors);
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    lhs = term_type(&rel->greater_than_or_equal.lhs, TOKEN_NULL, variables,
                    errors);
    rhs = term_type(&rel->greater_than_or_equal.rhs, TOKEN_NULL, variables,
                    errors);
    break;
  }

  if (lhs != rhs) {
    scu_perror(errors, "Type mismatch in conditional statement: %d vs %d\n",
               lhs, rhs);
  }
}

void instr_typecheck(instr_node *instr, dynamic_array *variables, int *errors) {
  switch (instr->kind) {
  case INSTR_ASSIGN: {
    token_kind target_type =
        var_type(instr->assign.identifier.name, variables, errors);

    token_kind expr_result =
        expr_type(&instr->assign.expr, target_type, variables, errors);
    if (target_type != expr_result) {
      scu_perror(errors, "Type mismatch in assignment to %s - %d to %d\n",
                 instr->assign.identifier.name, target_type, expr_result);
    }
    break;
  }
  case INSTR_IF:
    rel_typecheck(&instr->if_.rel, variables, errors);
    break;
  case INSTR_OUTPUT:
  case INSTR_DECLARE:
  case INSTR_GOTO:
  case INSTR_LABEL:
    break;
  }
}

void check_semantics(dynamic_array *instrs, dynamic_array *variables,
                     int *errors) {
  // Semantic Analysis - Check variables
  for (unsigned int i = 0; i < instrs->count; i++) {
    struct instr_node instr;
    dynamic_array_get(instrs, i, &instr);
    instr_check_variables(&instr, variables, errors);
  }

  scu_check_errors(errors);

  // Semantic Analysis - Check variable types
  for (unsigned int i = 0; i < instrs->count; i++) {
    struct instr_node instr;
    dynamic_array_get(instrs, i, &instr);
    instr_typecheck(&instr, variables, errors);
  }

  scu_check_errors(errors);
}

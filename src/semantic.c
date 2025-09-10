#include "semantic.h"
#include "data_structures.h"
#include "parser.h"
#include "utils.h"

#define _POSIX_C_SOURCE 200809L
#include <string.h>

int find_variables(dynamic_array *variables, variable *var_to_find,
                   unsigned int *errors) {
  for (unsigned int i = 0; i < variables->count; i++) {
    variable var;
    dynamic_array_get(variables, i, &var);

    if (strcmp(var_to_find->name, var.name) == 0) {
      return i;
    }
  }

  scu_perror(errors, "Use of undeclared variable: %s [line %u]\n",
             var_to_find->name, var_to_find->line);
  scu_check_errors(errors);
  return -1;
}

/*
 * @brief: append a new variable to the variables dynamic_array.
 *
 * @param var_to_declare: the variable struct to append.
 * @param variables: pointer to the variables dynamic_array.
 */
static void declare_variables(variable *var_to_declare,
                              dynamic_array *variables) {
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

/*
 * @brief: check variables in terms
 *
 * @param term: pointer to a term_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void term_check_variables(term_node *term, dynamic_array *variables,
                                 unsigned int *errors) {
  switch (term->kind) {
  case TERM_IDENTIFIER:
    find_variables(variables, &term->identifier, errors);
    break;
  default:
    break;
  }
}

/*
 * @brief: check variables in expressions
 *
 * @param expr: pointer to an expr_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void expr_check_variables(expr_node *expr, dynamic_array *variables,
                                 unsigned int *errors) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_check_variables(&expr->term, variables, errors);
    break;
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    expr_check_variables(expr->binary.left, variables, errors);
    expr_check_variables(expr->binary.right, variables, errors);
    break;
  }
}

/*
 * @brief: check variables in relational expressions
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void rel_check_variables(rel_node *rel, dynamic_array *variables,
                                unsigned int *errors) {
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

/*
 * @brief: check variables in an individual instruction
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void instr_check_variables(instr_node *instr, dynamic_array *variables,
                                  unsigned int *errors) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    declare_variables(&instr->declare_variable, variables);
    break;
  case INSTR_INITIALIZE:
    declare_variables(&instr->initialize_variable.var, variables);
    expr_check_variables(&instr->initialize_variable.expr, variables, errors);
    break;
  case INSTR_ASSIGN:
    expr_check_variables(&instr->assign.expr, variables, errors);
    break;
  case INSTR_IF:
    rel_check_variables(&instr->if_.rel, variables, errors);
    instr_check_variables(instr->if_.instr, variables, errors);
    break;
  case INSTR_OUTPUT:
    term_check_variables(&instr->output.term, variables, errors);
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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void check_label(dynamic_array *labels, instr_node *instr,
                        unsigned int *errors) {
  const char *label_name = instr->label.label;
  for (unsigned int i = 0; i < labels->count; i++) {
    char *existing;
    dynamic_array_get(labels, i, &existing);
    if (strcmp(label_name, existing) == 0) {
      scu_perror(errors, "Duplicate label declaration: %s [line %u]\n",
                 label_name, instr->line);
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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void check_goto(dynamic_array *labels, instr_node *instr,
                       unsigned int *errors) {
  int found = 0;
  for (unsigned int i = 0; i < labels->count; i++) {
    char *label;
    dynamic_array_get(labels, i, &label);
    if (strcmp(label, instr->goto_.label) == 0) {
      found = 1;
      break;
    }
  }
  if (!found) {
    scu_perror(errors, "Use of undeclared label: %s [line %u]\n",
               instr->goto_.label, instr->line);
  }
}

/*
 * @brief: check for declaration of labels AND the use of labels in goto
 * instructions
 *
 * @param instr: pointer to an instr_node.
 * @param labels: pointer to the labels dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void instrs_check_labels(dynamic_array *instrs, dynamic_array *labels,
                                unsigned int *errors) {
  // check labels first
  for (unsigned int i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_LABEL) {
      check_label(labels, &instr, errors);
    }
  }

  // then check goto
  for (unsigned int i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_GOTO) {
      check_goto(labels, &instr, errors);
    }
  }

  // then check if
  for (unsigned int i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);

    if (instr.kind == INSTR_IF) {
      switch (instr.if_.instr->kind) {
      case INSTR_GOTO:
        check_goto(labels, instr.if_.instr, errors);
        break;
      case INSTR_LABEL:
        check_label(labels, instr.if_.instr, errors);
        break;
      default:
        break;
      }
    }
  }
}

type var_type(const char *name, size_t line, dynamic_array *variables,
              unsigned int *errors) {
  for (unsigned int i = 0; i < variables->count; i++) {
    variable var;
    dynamic_array_get(variables, i, &var);

    if (strcmp(name, var.name) == 0) {
      return var.type;
    }
  }
  scu_perror(errors, "Use of undeclared variable: %s [line %u]\n", name, line);
  return -1;
}

/*
 * @brief: check for types in a term_node
 *
 * @param term: pointer to a term_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 * @param line: where the term is situated in the source buffer.
 */
static type term_type(term_node *term, type target_type, size_t line,
                      dynamic_array *variables, unsigned int *errors) {
  switch (term->kind) {
  case TERM_INPUT:
    return target_type;
  case TERM_INT:
    return TYPE_INT;
  case TERM_CHAR:
    return TYPE_CHAR;
  case TERM_POINTER:
  case TERM_DEREF:
  case TERM_ADDOF:
  case TERM_IDENTIFIER:
    return var_type(term->identifier.name, line, variables, errors);
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
  case TYPE_POINTER:
    return "ptr";
  case TYPE_VOID:
    return "void";
  }
}

/*
 * @brief: check for types in an expr_node
 *
 * @param expr: pointer to an expr_node.
 * @param target_type: type enumeration for the type which is required in the
 * instruction.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static type expr_type(expr_node *expr, type target_type,
                      dynamic_array *variables, unsigned int *errors) {
  type lhs, rhs;

  switch (expr->kind) {
  case EXPR_TERM:
    return term_type(&expr->term, target_type, expr->line, variables, errors);
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    lhs = expr_type(expr->binary.left, target_type, variables, errors);
    rhs = expr_type(expr->binary.right, target_type, variables, errors);
    break;
  }

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror(errors,
               "Type mismatch in arithmetic expression: %s vs %s [line %u]\n",
               lhs_type_str, rhs_type_str, expr->line);
  }
  return lhs;
}

/*
 * @brief: check for types in a rel_node
 *
 * @param rel: pointer to a rel_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void rel_typecheck(rel_node *rel, dynamic_array *variables,
                          unsigned int *errors) {
  type lhs, rhs;

  switch (rel->kind) {
  case REL_IS_EQUAL:
    lhs =
        term_type(&rel->is_equal.lhs, TYPE_VOID, rel->line, variables, errors);
    rhs =
        term_type(&rel->is_equal.rhs, TYPE_VOID, rel->line, variables, errors);
    break;
  case REL_NOT_EQUAL:
    lhs =
        term_type(&rel->not_equal.lhs, TYPE_VOID, rel->line, variables, errors);
    rhs =
        term_type(&rel->not_equal.rhs, TYPE_VOID, rel->line, variables, errors);
    break;
  case REL_LESS_THAN:
    lhs =
        term_type(&rel->less_than.lhs, TYPE_VOID, rel->line, variables, errors);
    rhs =
        term_type(&rel->less_than.rhs, TYPE_VOID, rel->line, variables, errors);
    break;
  case REL_LESS_THAN_OR_EQUAL:
    lhs = term_type(&rel->less_than_or_equal.lhs, TYPE_VOID, rel->line,
                    variables, errors);
    rhs = term_type(&rel->less_than_or_equal.rhs, TYPE_VOID, rel->line,
                    variables, errors);
    break;
  case REL_GREATER_THAN:
    lhs = term_type(&rel->greater_than.lhs, TYPE_VOID, rel->line, variables,
                    errors);
    rhs = term_type(&rel->greater_than.rhs, TYPE_VOID, rel->line, variables,
                    errors);
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    lhs = term_type(&rel->greater_than_or_equal.lhs, TYPE_VOID, rel->line,
                    variables, errors);
    rhs = term_type(&rel->greater_than_or_equal.rhs, TYPE_VOID, rel->line,
                    variables, errors);
    break;
  }

  if (lhs != rhs) {
    const char *lhs_type_str = type_to_str(lhs);
    const char *rhs_type_str = type_to_str(rhs);
    scu_perror(errors,
               "Type mismatch in conditional statement: %s vs %s [line %u]\n",
               lhs_type_str, rhs_type_str, rel->line);
  }
}

/*
 * @brief: check for types in an instr_node
 *
 * @param instr: pointer to an instr_node.
 * @param variables: pointer to the variables dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void instr_typecheck(instr_node *instr, dynamic_array *variables,
                            unsigned int *errors) {
  switch (instr->kind) {
  case INSTR_INITIALIZE: {
    type target_type = instr->initialize_variable.var.type;
    type expr_result = expr_type(&instr->initialize_variable.expr, target_type,
                                 variables, errors);
    if (target_type == TYPE_POINTER) {
      return;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror(errors,
                 "Type mismatch in initialization to %s - %s to %s [line %u]\n",
                 instr->assign.identifier.name, expr_result_str,
                 target_type_str, instr->line);
    }
    break;
  }
  case INSTR_ASSIGN: {
    type target_type =
        var_type(instr->assign.identifier.name, instr->line, variables, errors);
    type expr_result =
        expr_type(&instr->assign.expr, target_type, variables, errors);
    if (target_type == TYPE_POINTER) {
      return;
    } else if (target_type != expr_result) {
      const char *target_type_str = type_to_str(target_type);
      const char *expr_result_str = type_to_str(expr_result);
      scu_perror(errors,
                 "Type mismatch in assignment to %s - %s to %s [line %u]\n",
                 instr->assign.identifier.name, expr_result_str,
                 target_type_str, instr->line);
    }
    break;
  }
  case INSTR_IF:
    rel_typecheck(&instr->if_.rel, variables, errors);
    break;
  default:
    break;
  }
}

void check_semantics(dynamic_array *instrs, dynamic_array *variables,
                     unsigned int *errors) {
  // Semantic Analysis - Check variables and their types
  for (unsigned int i = 0; i < instrs->count; i++) {
    instr_node instr;
    dynamic_array_get(instrs, i, &instr);
    instr_check_variables(&instr, variables, errors);
    instr_typecheck(&instr, variables, errors);
  }

  // Semantic Analysis - Check labels
  dynamic_array labels;
  dynamic_array_init(&labels, sizeof(char *));
  instrs_check_labels(instrs, &labels, errors);
  dynamic_array_free(&labels);

  scu_check_errors(errors);
}

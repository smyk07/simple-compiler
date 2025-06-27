#include "parser.h"
#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

void parser_init(dynamic_array *tokens, parser *p) {
  p->tokens = *tokens;
  p->index = 0;
}

void parser_current(parser *p, token *token, unsigned int *errors) {
  dynamic_array_get(&p->tokens, p->index, token);
  if (token->kind == TOKEN_END) {
    scu_check_errors(errors);
  }
}

void parser_advance(parser *p) { p->index++; }

void parse_term_for_expr(parser *p, term_node *term, unsigned int *errors) {
  token token;

  parser_current(p, &token, errors);
  term->line = token.line;
  if (token.kind == TOKEN_INPUT) {
    term->kind = TERM_INPUT;
  } else if (token.kind == TOKEN_INT) {
    term->kind = TERM_INT;
    term->value.integer = token.value.integer;
  } else if (token.kind == TOKEN_CHAR) {
    term->kind = TERM_CHAR;
    term->value.character = token.value.character;
  } else if (token.kind == TOKEN_IDENTIFIER) {
    term->kind = TERM_IDENTIFIER;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;
  } else if (token.kind == TOKEN_ADDRESS_OF) {
    term->kind = TERM_ADDOF;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;
  } else if (token.kind == TOKEN_POINTER) {
    term->kind = TERM_DEREF;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;
  } else {
    scu_perror(errors,
               "Expected a term (input, int, char, identifier, addof, "
               "pointer), got %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }

  parser_advance(p);
}

expr_node *parse_expr(parser *p, unsigned int *errors);

expr_node *parse_factor(parser *p, unsigned int *errors) {
  token token;
  parser_current(p, &token, errors);
  if (token.kind == TOKEN_INT || token.kind == TOKEN_CHAR ||
      token.kind == TOKEN_IDENTIFIER || token.kind == TOKEN_POINTER ||
      token.kind == TOKEN_ADDRESS_OF || token.kind == TOKEN_INPUT) {
    expr_node *node = malloc(sizeof(expr_node));
    node->kind = EXPR_TERM;
    node->line = token.line;

    if (token.kind == TOKEN_INT) {
      node->term.kind = TERM_INT;
      node->term.value.integer = token.value.integer;
    } else if (token.kind == TOKEN_CHAR) {
      node->term.kind = TERM_CHAR;
      node->term.value.character = token.value.character;
    } else if (token.kind == TOKEN_IDENTIFIER) {
      node->term.kind = TERM_IDENTIFIER;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;
    } else if (token.kind == TOKEN_POINTER) {
      node->term.kind = TERM_DEREF;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;
    } else if (token.kind == TOKEN_ADDRESS_OF) {
      node->term.kind = TERM_ADDOF;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;
    } else if (token.kind == TOKEN_INPUT) {
      node->term.kind = TERM_INPUT;
    }

    parser_advance(p);
    return node;
  } else if (token.kind == TOKEN_BRACKET_OPEN) {
    parser_advance(p);
    expr_node *node = parse_expr(p, errors);
    parser_current(p, &token, errors);
    if (token.kind != TOKEN_BRACKET_CLOSE) {
      scu_perror(errors, "Syntax error: expected ')'\n");
      scu_check_errors(errors);
    }
    parser_advance(p);
    return node;
  } else {
    scu_perror(errors, "Syntax error: expected term or '('\n");
    scu_check_errors(errors);
    exit(1);
  }
}

expr_node *parse_term(parser *p, unsigned int *errors) {
  expr_node *left = parse_factor(p, errors);
  while (1) {
    token token;
    parser_current(p, &token, errors);

    if (token.kind == TOKEN_MULTIPLY || token.kind == TOKEN_DIVIDE ||
        token.kind == TOKEN_MODULO) {
      parser_advance(p);
      expr_node *right = parse_factor(p, errors);

      expr_node *parent = malloc(sizeof(expr_node));

      parent->line = token.line;

      if (token.kind == TOKEN_MULTIPLY) {
        parent->kind = EXPR_MULTIPLY;
      } else if (token.kind == TOKEN_DIVIDE) {
        parent->kind = EXPR_DIVIDE;
      } else {
        parent->kind = EXPR_MODULO;
      }
      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }
  return left;
}

expr_node *parse_expr(parser *p, unsigned int *errors) {
  expr_node *left = parse_term(p, errors);
  while (1) {
    token token;
    parser_current(p, &token, errors);

    if (token.kind == TOKEN_ADD || token.kind == TOKEN_SUBTRACT) {
      parser_advance(p);
      expr_node *right = parse_term(p, errors);

      expr_node *parent = malloc(sizeof(expr_node));
      parent->kind = (token.kind == TOKEN_ADD) ? EXPR_ADD : EXPR_SUBTRACT;
      parent->line = token.line;
      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }
  return left;
}

void parse_rel(parser *p, rel_node *rel, unsigned int *errors) {
  token token;
  term_node lhs, rhs;

  parse_term_for_expr(p, &lhs, errors);

  parser_current(p, &token, errors);
  rel->line = token.line;
  if (token.kind == TOKEN_IS_EQUAL) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_IS_EQUAL;
    rel->is_equal.lhs = lhs;
    rel->is_equal.rhs = rhs;
  } else if (token.kind == TOKEN_NOT_EQUAL) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_NOT_EQUAL;
    rel->not_equal.lhs = lhs;
    rel->not_equal.rhs = rhs;
  } else if (token.kind == TOKEN_LESS_THAN) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_LESS_THAN;
    rel->less_than.lhs = lhs;
    rel->less_than.rhs = rhs;
  } else if (token.kind == TOKEN_LESS_THAN_OR_EQUAL) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_LESS_THAN_OR_EQUAL;
    rel->less_than_or_equal.lhs = lhs;
    rel->less_than_or_equal.rhs = rhs;
  } else if (token.kind == TOKEN_GREATER_THAN) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_GREATER_THAN;
    rel->greater_than.lhs = lhs;
    rel->greater_than.rhs = rhs;
  } else if (token.kind == TOKEN_GREATER_THAN_OR_EQUAL) {
    parser_advance(p);
    parse_term_for_expr(p, &rhs, errors);

    rel->kind = REL_GREATER_THAN_OR_EQUAL;
    rel->greater_than_or_equal.lhs = lhs;
    rel->greater_than_or_equal.rhs = rhs;
  } else {
    scu_perror(errors,
               "Expected a relation (==, !=, <, <=, >, >=), got %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
}

void parse_instr(parser *p, instr_node *instr, unsigned int *errors);

void parse_initialize(parser *p, instr_node *instr, type _type, char *_name,
                      unsigned int *errors) {
  instr->kind = INSTR_INITIALIZE;
  instr->initialize_variable.var.type = _type;
  instr->initialize_variable.var.name = _name;
  parser_advance(p);

  expr_node *expr = parse_expr(p, errors);
  instr->initialize_variable.expr = *expr;
  free(expr);
}

void parse_declare(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  type _type = TYPE_VOID;
  char *_name;
  int _line;

  parser_current(p, &token, errors);
  instr->line = token.line;
  if (token.kind == TOKEN_TYPE_INT) {
    _type = TYPE_INT;
  } else if (token.kind == TOKEN_TYPE_CHAR) {
    _type = TYPE_CHAR;
  }
  parser_advance(p);

  parser_current(p, &token, errors);
  _name = token.value.str;
  _line = token.line;
  parser_advance(p);

  instr->kind = INSTR_DECLARE;
  instr->declare_variable.type = _type;
  instr->declare_variable.name = _name;
  instr->declare_variable.line = _line;

  parser_current(p, &token, errors);
  if (token.kind == TOKEN_ASSIGN) {
    parse_initialize(p, instr, _type, _name, errors);
  }
}

void parse_assign(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  instr->kind = INSTR_ASSIGN;

  parser_current(p, &token, errors);
  instr->line = token.line;
  instr->assign.identifier.name = token.value.str;
  instr->assign.identifier.line = token.line;
  if (token.kind == TOKEN_POINTER) {
    instr->assign.identifier.type = TYPE_POINTER;
  }
  parser_advance(p);

  parser_current(p, &token, errors);
  if (token.kind != TOKEN_ASSIGN) {
    scu_perror(errors, "Expected assign, found %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
  parser_advance(p);

  expr_node *expr = parse_expr(p, errors);
  instr->assign.expr = *expr;
  free(expr);
}

void parse_if(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  instr->kind = INSTR_IF;

  parser_advance(p);

  parse_rel(p, &instr->if_.rel, errors);

  parser_current(p, &token, errors);
  instr->line = token.line;
  if (token.kind != TOKEN_THEN) {
    scu_perror(errors, "Expected then, found %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
  parser_advance(p);

  instr->if_.instr = malloc(sizeof(instr_node));
  parse_instr(p, instr->if_.instr, errors);
}

void parse_goto(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  instr->kind = INSTR_GOTO;

  parser_advance(p);

  parser_current(p, &token, errors);
  instr->line = token.line;
  if (token.kind != TOKEN_LABEL) {
    scu_perror(errors, "Expected label, found %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
  parser_advance(p);

  instr->goto_.label = token.value.str;
}

void parse_output(parser *p, instr_node *instr, unsigned int *errors) {
  term_node rhs;

  instr->kind = INSTR_OUTPUT;

  parser_advance(p);
  parse_term_for_expr(p, &rhs, errors);

  instr->line = rhs.line;
  instr->output.term = rhs;
}

void parse_label(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  instr->kind = INSTR_LABEL;

  parser_current(p, &token, errors);
  instr->line = token.line;
  instr->label.label = token.value.str;

  parser_advance(p);
}

void parse_instr(parser *p, instr_node *instr, unsigned int *errors) {
  token token;

  parser_current(p, &token, errors);

  switch (token.kind) {
  case TOKEN_TYPE_INT:
  case TOKEN_TYPE_CHAR:
    parse_declare(p, instr, errors);
    break;
  case TOKEN_IDENTIFIER:
  case TOKEN_POINTER:
    parse_assign(p, instr, errors);
    break;
  case TOKEN_OUTPUT:
    parse_output(p, instr, errors);
    break;
  case TOKEN_IF:
    parse_if(p, instr, errors);
    break;
  case TOKEN_GOTO:
    parse_goto(p, instr, errors);
    break;
  case TOKEN_LABEL:
    parse_label(p, instr, errors);
    break;
  default:
    scu_perror(errors, "unexpected token: %s [line %d]\n",
               show_token_kind(token.kind), token.line);
    scu_check_errors(errors);
  }
}

void parse_program(parser *p, program_node *program, unsigned int *errors) {
  dynamic_array_init(&program->instrs, sizeof(instr_node));

  token token;
  parser_current(p, &token, errors);
  while (token.kind != TOKEN_END) {
    if (token.kind == TOKEN_COMMENT) {
      parser_advance(p);
      parser_current(p, &token, errors);
      continue;
    }
    instr_node *instr = malloc(sizeof(instr_node));
    parse_instr(p, instr, errors);
    dynamic_array_append(&program->instrs, instr);
    free(instr);
    parser_current(p, &token, errors);
  }
}

void check_var_and_print(variable *var) {
  switch (var->type) {
  case TYPE_INT:
  case TYPE_CHAR:
    printf("%s", var->name);
    break;
  case TYPE_POINTER:
    printf("*%s", var->name);
    break;
  case TYPE_VOID:
    break;
  }
}

void check_term_and_print(term_node *term) {
  switch (term->kind) {
  case TERM_INPUT:
    printf("input");
    break;
  case TERM_INT:
    printf("%d", term->value.integer);
    break;
  case TERM_CHAR:
    printf("\'%c\'", term->value.character);
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
  }
}

void check_expr_and_print(expr_node *expr) {
  switch (expr->kind) {
  case EXPR_TERM:
    check_term_and_print(&expr->term);
    break;
  case EXPR_ADD:
    printf("(");
    check_expr_and_print(expr->binary.left);
    printf(" + ");
    check_expr_and_print(expr->binary.right);
    printf(")");
    break;
  case EXPR_SUBTRACT:
    printf("(");
    check_expr_and_print(expr->binary.left);
    printf(" - ");
    check_expr_and_print(expr->binary.right);
    printf(")");
    break;
  case EXPR_MULTIPLY:
    printf("(");
    check_expr_and_print(expr->binary.left);
    printf(" * ");
    check_expr_and_print(expr->binary.right);
    printf(")");
    break;
  case EXPR_DIVIDE:
    printf("(");
    check_expr_and_print(expr->binary.left);
    printf(" / ");
    check_expr_and_print(expr->binary.right);
    printf(")");
    break;
  case EXPR_MODULO:
    printf("(");
    check_expr_and_print(expr->binary.left);
    printf(" %% ");
    check_expr_and_print(expr->binary.right);
    printf(")");
    break;
  }
}

void check_binary_node_and_print(term_binary_node *bnode, char *operator) {
  check_term_and_print(&bnode->lhs);
  printf(" %s ", operator);
  check_term_and_print(&bnode->rhs);
  printf("\n");
}

void print_instr(instr_node instr) {
  printf("[line %d] ", instr.line);
  switch (instr.kind) {
  case INSTR_DECLARE:
    printf("declare: ");
    check_var_and_print(&instr.declare_variable);
    printf("\n");
    break;

  case INSTR_INITIALIZE:
    printf("initialize: ");
    check_var_and_print(&instr.initialize_variable.var);
    printf(" = ");
    switch (instr.initialize_variable.var.type) {
    case TYPE_INT:
    case TYPE_POINTER:
      check_expr_and_print(&instr.initialize_variable.expr);
      printf("\n");
      break;
    case TYPE_CHAR:
      switch (instr.initialize_variable.expr.kind) {
      case EXPR_TERM:
        printf("\'%c\'\n", instr.initialize_variable.expr.term.value.character);
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
    check_var_and_print(&instr.assign.identifier);
    printf(" = ");
    check_expr_and_print(&instr.assign.expr);
    printf("\n");
    break;

  case INSTR_IF:
    printf("if ");
    switch (instr.if_.rel.kind) {
    case REL_IS_EQUAL:
      check_binary_node_and_print(&instr.if_.rel.is_equal, "==");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_NOT_EQUAL:
      check_binary_node_and_print(&instr.if_.rel.not_equal, "!=");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_LESS_THAN:
      check_binary_node_and_print(&instr.if_.rel.less_than, "<");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_LESS_THAN_OR_EQUAL:
      check_binary_node_and_print(&instr.if_.rel.less_than_or_equal, "<=");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_GREATER_THAN:
      check_binary_node_and_print(&instr.if_.rel.greater_than, ">");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_GREATER_THAN_OR_EQUAL:
      check_binary_node_and_print(&instr.if_.rel.greater_than_or_equal, ">=");
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    }
    break;

  case INSTR_GOTO:
    printf("goto: %s\n", instr.goto_.label);
    break;

  case INSTR_OUTPUT:
    printf("output: ");
    check_term_and_print(&instr.output.term);
    printf("\n");
    break;

  case INSTR_LABEL:
    printf("label: %s\n", instr.label.label);
    break;
  }
}

void print_program(program_node *program) {
  scu_pdebug("Parsing Debug Statements:\n");

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);
    print_instr(instr);
  }
}

void free_if_instrs(program_node *program) {
  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node *instr = program->instrs.items + (i * program->instrs.item_size);
    if (instr->kind == INSTR_IF) {
      free(instr->if_.instr);
    }
  }
}

void free_expr(expr_node *expr) {
  switch (expr->kind) {
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    free_expr(expr->binary.left);
    free_expr(expr->binary.right);
  case EXPR_TERM:
    free(expr);
    break;
  }
}

void free_expr_contents(expr_node *expr) {
  switch (expr->kind) {
  case EXPR_TERM:
    break;
  case EXPR_ADD:
  case EXPR_SUBTRACT:
  case EXPR_MULTIPLY:
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    free_expr(expr->binary.left);
    free_expr(expr->binary.right);
    break;
  }
}

void free_expressions(program_node *program) {
  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node *instr = program->instrs.items + (i * program->instrs.item_size);
    if (instr->kind == INSTR_ASSIGN) {
      free_expr_contents(&instr->assign.expr);
    } else if (instr->kind == INSTR_INITIALIZE) {
      free_expr_contents(&instr->initialize_variable.expr);
    }
  }
}

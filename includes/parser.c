#include "parser.h"
#include "lexer.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

void parser_init(dynamic_array tokens, parser *p) {
  p->tokens = tokens;
  p->index = 0;
}

void parser_current(parser *p, token *token, int *errors) {
  dynamic_array_get(&p->tokens, p->index, token);
  if (token->kind == TOKEN_END) {
    scu_check_errors(errors);
  }
}

void parser_advance(parser *p) { p->index++; }

void parse_term(parser *p, term_node *term, int *errors) {
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
    term->identifier.name = token.value.str;
  } else {
    scu_perror(
        errors,
        "Expected a term (input, int, char, identifier), got %s [line %d]\n",
        show_token_kind(token.kind), token.line);
  }

  parser_advance(p);
}

void parse_expr(parser *p, expr_node *expr, int *errors) {
  token token;
  term_node lhs, rhs;

  parse_term(p, &lhs, errors);

  parser_current(p, &token, errors);
  expr->line = token.line;
  if (token.kind == TOKEN_ADD) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    expr->kind = EXPR_ADD;
    expr->add.lhs = lhs;
    expr->add.rhs = rhs;
  } else if (token.kind == TOKEN_SUBTRACT) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    expr->kind = EXPR_SUBTRACT;
    expr->subtract.lhs = lhs;
    expr->subtract.rhs = rhs;
  } else if (token.kind == TOKEN_MULTIPLY) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    expr->kind = EXPR_MULTIPLY;
    expr->multiply.lhs = lhs;
    expr->multiply.rhs = rhs;
  } else if (token.kind == TOKEN_DIVIDE) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    expr->kind = EXPR_DIVIDE;
    expr->divide.lhs = lhs;
    expr->divide.rhs = rhs;
  } else if (token.kind == TOKEN_MODULO) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    expr->kind = EXPR_MODULO;
    expr->modulo.lhs = lhs;
    expr->modulo.rhs = rhs;
  } else {
    expr->kind = EXPR_TERM;
    expr->term = lhs;
  }
}

void parse_rel(parser *p, rel_node *rel, int *errors) {
  token token;
  term_node lhs, rhs;

  parse_term(p, &lhs, errors);

  parser_current(p, &token, errors);
  rel->line = token.line;
  if (token.kind == TOKEN_IS_EQUAL) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_IS_EQUAL;
    rel->is_equal.lhs = lhs;
    rel->is_equal.rhs = rhs;
  } else if (token.kind == TOKEN_NOT_EQUAL) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_NOT_EQUAL;
    rel->not_equal.lhs = lhs;
    rel->not_equal.rhs = rhs;
  } else if (token.kind == TOKEN_LESS_THAN) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_LESS_THAN;
    rel->less_than.lhs = lhs;
    rel->less_than.rhs = rhs;
  } else if (token.kind == TOKEN_LESS_THAN_OR_EQUAL) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_LESS_THAN_OR_EQUAL;
    rel->less_than_or_equal.lhs = lhs;
    rel->less_than_or_equal.rhs = rhs;
  } else if (token.kind == TOKEN_GREATER_THAN) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_GREATER_THAN;
    rel->greater_than.lhs = lhs;
    rel->greater_than.rhs = rhs;
  } else if (token.kind == TOKEN_GREATER_THAN_OR_EQUAL) {
    parser_advance(p);
    parse_term(p, &rhs, errors);

    rel->kind = REL_GREATER_THAN_OR_EQUAL;
    rel->greater_than_or_equal.lhs = lhs;
    rel->greater_than_or_equal.rhs = rhs;
  } else {
    scu_perror(errors,
               "Expected a relation (==, !=, <, <=, >, >=), got %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
}

void parse_instr(parser *p, instr_node *instr, int *errors);

void parse_initialize(parser *p, instr_node *instr, type _type, char *_name,
                      int *errors) {
  instr->kind = INSTR_INITIALIZE;
  instr->initialize_variable.var.type = _type;
  instr->initialize_variable.var.name = _name;
  parser_advance(p);

  parse_expr(p, &instr->initialize_variable.expr, errors);
}

void parse_declare(parser *p, instr_node *instr, int *errors) {
  token token;

  type _type = TYPE_VOID;
  char *_name;

  instr->kind = INSTR_DECLARE;

  parser_current(p, &token, errors);
  instr->line = token.line;

  if (token.kind == TOKEN_TYPE_INT) {
    _type = TYPE_INT;
  } else if (token.kind == TOKEN_TYPE_CHAR) {
    _type = TYPE_CHAR;
  }

  instr->declare_variable.type = _type;
  parser_advance(p);

  parser_current(p, &token, errors);
  _name = token.value.str;
  instr->declare_variable.name = _name;
  instr->declare_variable.line = token.line;
  parser_advance(p);

  parser_current(p, &token, errors);
  if (token.kind == TOKEN_ASSIGN) {
    parse_initialize(p, instr, _type, _name, errors);
  }
}

void parse_assign(parser *p, instr_node *instr, int *errors) {
  token token;

  instr->kind = INSTR_ASSIGN;

  parser_current(p, &token, errors);
  instr->line = token.line;
  instr->assign.identifier.name = token.value.str;
  instr->assign.identifier.line = token.line;
  parser_advance(p);

  parser_current(p, &token, errors);
  if (token.kind != TOKEN_ASSIGN) {
    scu_perror(errors, "Expected assign, found %s [line %d]\n",
               show_token_kind(token.kind), token.line);
  }
  parser_advance(p);

  parse_expr(p, &instr->assign.expr, errors);
}

void parse_if(parser *p, instr_node *instr, int *errors) {
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

void parse_goto(parser *p, instr_node *instr, int *errors) {
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

void parse_output(parser *p, instr_node *instr, int *errors) {
  term_node rhs;

  instr->kind = INSTR_OUTPUT;

  parser_advance(p);
  parse_term(p, &rhs, errors);

  instr->line = rhs.line;
  instr->output.term = rhs;
}

void parse_label(parser *p, instr_node *instr, int *errors) {
  token token;

  instr->kind = INSTR_LABEL;

  parser_current(p, &token, errors);
  instr->line = token.line;
  instr->label.label = token.value.str;

  parser_advance(p);
}

void parse_instr(parser *p, instr_node *instr, int *errors) {
  token token;

  parser_current(p, &token, errors);

  switch (token.kind) {
  case TOKEN_TYPE_INT:
  case TOKEN_TYPE_CHAR:
    parse_declare(p, instr, errors);
    break;
  case TOKEN_IDENTIFIER:
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

void parse_program(parser *p, program_node *program, int *errors) {
  dynamic_array_init(&program->instrs, sizeof(instr_node));

  token token;
  parser_current(p, &token, errors);
  while (token.kind != TOKEN_END) {
    instr_node *instr = malloc(sizeof(instr_node));
    parse_instr(p, instr, errors);
    dynamic_array_append(&program->instrs, instr);
    free(instr);
    parser_current(p, &token, errors);
  }
}

void check_terms_and_print(term_node *lhs, char *operator, term_node * rhs) {
  switch (lhs->kind) {
  case TERM_INPUT:
    printf("input");
    break;
  case TERM_INT:
    printf("\'%d\'", lhs->value.integer);
    break;
  case TERM_CHAR:
    printf("\'%c\'", lhs->value.character);
    break;
  case TERM_IDENTIFIER:
    printf("%s", lhs->identifier.name);
    break;
  }

  printf(" %s ", operator);

  switch (rhs->kind) {
  case TERM_INPUT:
    printf("input\n");
    break;
  case TERM_INT:
    printf("\'%d\'\n", rhs->value.integer);
    break;
  case TERM_CHAR:
    printf("\'%c\'\n", rhs->value.character);
    break;
  case TERM_IDENTIFIER:
    printf("%s\n", rhs->identifier.name);
    break;
  }
}

void print_instr(instr_node instr) {
  printf("[line %d] ", instr.line);
  switch (instr.kind) {
  case INSTR_DECLARE:
    printf("declare: %s\n", instr.declare_variable.name);
    break;
  case INSTR_INITIALIZE:
    printf("initialize: %s = ", instr.initialize_variable.var.name);
    switch (instr.initialize_variable.var.type) {
    case TYPE_INT:
      switch (instr.initialize_variable.expr.kind) {
      case EXPR_TERM:
        switch (instr.initialize_variable.expr.term.kind) {
        case TERM_INPUT:
          printf("input\n");
          break;
        case TERM_INT:
          printf("\'%d\'\n", instr.initialize_variable.expr.term.value.integer);
          break;
        case TERM_CHAR:
          printf("\'%c\'\n",
                 instr.initialize_variable.expr.term.value.character);
          break;
        case TERM_IDENTIFIER:
          printf("%s\n", instr.initialize_variable.expr.term.identifier.name);
          break;
        }
        break;
      case EXPR_ADD:
        check_terms_and_print(&instr.initialize_variable.expr.add.lhs, "+",
                              &instr.initialize_variable.expr.add.rhs);
        break;
      case EXPR_SUBTRACT:
        check_terms_and_print(&instr.initialize_variable.expr.subtract.lhs, "-",
                              &instr.initialize_variable.expr.subtract.rhs);
        break;
      case EXPR_MULTIPLY:
        check_terms_and_print(&instr.initialize_variable.expr.multiply.lhs, "*",
                              &instr.initialize_variable.expr.multiply.rhs);
        break;
      case EXPR_DIVIDE:
        check_terms_and_print(&instr.initialize_variable.expr.divide.lhs, "/",
                              &instr.initialize_variable.expr.divide.rhs);
        break;
      case EXPR_MODULO:
        check_terms_and_print(&instr.initialize_variable.expr.modulo.lhs, "%",
                              &instr.initialize_variable.expr.modulo.rhs);
        break;
      }
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
    printf("assign: %s = ", instr.assign.identifier.name);
    switch (instr.assign.expr.kind) {
    case EXPR_TERM:
      switch (instr.assign.expr.term.kind) {
      case TERM_INPUT:
        printf("input\n");
        break;
      case TERM_INT:
        printf("\'%d\'\n", instr.assign.expr.term.value.integer);
        break;
      case TERM_CHAR:
        printf("\'%c\'\n", instr.assign.expr.term.value.character);
        break;
      case TERM_IDENTIFIER:
        printf("%s\n", instr.assign.expr.term.identifier.name);
        break;
      }
      break;
    case EXPR_ADD:
      check_terms_and_print(&instr.assign.expr.add.lhs, "+",
                            &instr.assign.expr.add.rhs);
      break;
    case EXPR_SUBTRACT:
      check_terms_and_print(&instr.assign.expr.subtract.lhs, "-",
                            &instr.assign.expr.subtract.rhs);
      break;
    case EXPR_MULTIPLY:
      check_terms_and_print(&instr.assign.expr.multiply.lhs, "*",
                            &instr.assign.expr.multiply.rhs);
      break;
    case EXPR_DIVIDE:
      check_terms_and_print(&instr.assign.expr.divide.lhs, "/",
                            &instr.assign.expr.divide.rhs);
      break;
    case EXPR_MODULO:
      check_terms_and_print(&instr.assign.expr.modulo.lhs, "%",
                            &instr.assign.expr.modulo.rhs);
      break;
    }
    break;
  case INSTR_IF:
    printf("if ");
    switch (instr.if_.rel.kind) {
    case REL_IS_EQUAL:
      check_terms_and_print(&instr.if_.rel.is_equal.lhs,
                            "==", &instr.if_.rel.is_equal.rhs);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_NOT_EQUAL:
      check_terms_and_print(&instr.if_.rel.not_equal.lhs,
                            "!=", &instr.if_.rel.not_equal.rhs);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_LESS_THAN:
      check_terms_and_print(&instr.if_.rel.less_than.lhs, "<",
                            &instr.if_.rel.less_than.rhs);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_LESS_THAN_OR_EQUAL:
      check_terms_and_print(&instr.if_.rel.less_than_or_equal.lhs,
                            "<=", &instr.if_.rel.less_than_or_equal.rhs);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_GREATER_THAN:
      check_terms_and_print(&instr.if_.rel.greater_than.lhs, ">",
                            &instr.if_.rel.greater_than.rhs);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    case REL_GREATER_THAN_OR_EQUAL:
      check_terms_and_print(&instr.if_.rel.greater_than_or_equal.lhs,
                            ">=", &instr.if_.rel.greater_than_or_equal.rhs);
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
    switch (instr.output.term.kind) {
    case TERM_INPUT:
      printf("input\n");
      break;
    case TERM_INT:
      printf("\'%d\'\n", instr.output.term.value.integer);
      break;
    case TERM_CHAR:
      printf("\'%c\'\n", instr.output.term.value.character);
      break;
    case TERM_IDENTIFIER:
      printf("%s\n", instr.output.term.identifier.name);
      break;
    }
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

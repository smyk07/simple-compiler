#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

void parser_init(dynamic_array tokens, parser *p) {
  p->tokens = tokens;
  p->index = 0;
}

void parser_current(parser *p, token *token) {
  dynamic_array_get(&p->tokens, p->index, token);
}

void parser_advance(parser *p) { p->index++; }

void parse_term(parser *p, term_node *term) {
  token token;

  parser_current(p, &token);
  if (token.kind == INPUT) {
    term->kind = TERM_INPUT;
  } else if (token.kind == INT) {
    term->kind = TERM_INT;
    term->value = token.value;
  } else if (token.kind == IDENTIFIER) {
    term->kind = TERM_IDENTIFIER;
    term->value = token.value;
  } else {
    printf("Expected a term (input, int, identifier), got %s\n",
           show_token_kind(token.kind));
    exit(1);
  }

  parser_advance(p);
}

void parse_expr(parser *p, expr_node *expr) {
  token token;
  term_node lhs, rhs;

  parse_term(p, &lhs);

  parser_current(p, &token);
  if (token.kind == ADD) {
    parser_advance(p);
    parse_term(p, &rhs);

    expr->kind = EXPR_ADD;
    expr->add.lhs = lhs;
    expr->add.rhs = rhs;
  } else if (token.kind == SUBTRACT) {
    parser_advance(p);
    parse_term(p, &rhs);

    expr->kind = EXPR_SUBTRACT;
    expr->subtract.lhs = lhs;
    expr->subtract.rhs = rhs;
  } else if (token.kind == MULTIPLY) {
    parser_advance(p);
    parse_term(p, &rhs);

    expr->kind = EXPR_MULTIPLY;
    expr->multiply.lhs = lhs;
    expr->multiply.rhs = rhs;
  } else if (token.kind == DIVIDE) {
    parser_advance(p);
    parse_term(p, &rhs);

    expr->kind = EXPR_DIVIDE;
    expr->divide.lhs = lhs;
    expr->divide.rhs = rhs;
  } else if (token.kind == MODULO) {
    parser_advance(p);
    parse_term(p, &rhs);

    expr->kind = EXPR_MODULO;
    expr->modulo.lhs = lhs;
    expr->modulo.rhs = rhs;
  } else {
    expr->kind = EXPR_TERM;
    expr->term = lhs;
  }
}

void parse_rel(parser *p, rel_node *rel) {
  token token;
  term_node lhs, rhs;

  parse_term(p, &lhs);

  parser_current(p, &token);
  if (token.kind == LESS_THAN) {
    parser_advance(p);
    parse_term(p, &rhs);

    rel->kind = REL_LESS_THAN;
    rel->less_than.lhs = lhs;
    rel->less_than.rhs = rhs;
  } else if (token.kind == GREATER_THAN) {
    parser_advance(p);
    parse_term(p, &rhs);

    rel->kind = REL_GREATER_THAN;
    rel->greater_than.lhs = lhs;
    rel->greater_than.rhs = rhs;
  } else {
    printf("Expected a relation (<, >), got %s\n", show_token_kind(token.kind));
    exit(1);
  }
}

void parse_instr(parser *p, instr_node *instr);

void parse_assign(parser *p, instr_node *instr) {
  token token;

  instr->kind = INSTR_ASSIGN;

  parser_current(p, &token);
  instr->assign.identifier = token.value;
  parser_advance(p);

  parser_current(p, &token);
  if (token.kind != ASSIGN) {
    printf("Expected assign, found %s\n", show_token_kind(token.kind));
    exit(1);
  }
  parser_advance(p);

  parse_expr(p, &instr->assign.expr);
}

void parse_if(parser *p, instr_node *instr) {
  token token;

  instr->kind = INSTR_IF;

  parser_advance(p);

  parse_rel(p, &instr->if_.rel);

  parser_current(p, &token);
  if (token.kind != THEN) {
    printf("Expected then, found %s\n", show_token_kind(token.kind));
    exit(1);
  }
  parser_advance(p);

  instr->if_.instr = malloc(sizeof(instr_node));
  parse_instr(p, instr->if_.instr);
}

void parse_goto(parser *p, instr_node *instr) {
  token token;

  instr->kind = INSTR_GOTO;

  parser_advance(p);

  parser_current(p, &token);
  if (token.kind != LABEL) {
    printf("Expected label, found %s\n", show_token_kind(token.kind));
    exit(1);
  }
  parser_advance(p);

  instr->goto_.label = token.value;
}

void parse_output(parser *p, instr_node *instr) {
  term_node rhs;

  instr->kind = INSTR_OUTPUT;
  parser_advance(p);

  parse_term(p, &rhs);

  instr->output.term = rhs;
}

void parse_label(parser *p, instr_node *instr) {
  token token;

  instr->kind = INSTR_LABEL;

  parser_current(p, &token);
  instr->label.label = token.value;

  parser_advance(p);
}

void parse_instr(parser *p, instr_node *instr) {
  token token;

  parser_current(p, &token);
  if (token.kind == IDENTIFIER) {
    parse_assign(p, instr);
  } else if (token.kind == IF) {
    parse_if(p, instr);
  } else if (token.kind == GOTO) {
    parse_goto(p, instr);
  } else if (token.kind == OUTPUT) {
    parse_output(p, instr);
  } else if (token.kind == LABEL) {
    parse_label(p, instr);
  } else {
    printf("unexpected token: %s\n", show_token_kind(token.kind));
    exit(1);
  }
}

void parse_program(parser *p, program_node *program) {
  dynamic_array_init(&program->instrs, sizeof(instr_node));

  token token;
  do {
    instr_node instr;

    parse_instr(p, &instr);

    dynamic_array_append(&program->instrs, &instr);

    parser_current(p, &token);
  } while (token.kind != END);
}

void print_instr(instr_node instr) {
  switch (instr.kind) {
  case INSTR_ASSIGN:
    printf("assign: %s = ", instr.assign.identifier);
    switch (instr.assign.expr.kind) {
    case EXPR_TERM:
      switch (instr.assign.expr.term.kind) {
      case TERM_INPUT:
        printf("input\n");
        break;
      case TERM_INT:
      case TERM_IDENTIFIER:
        printf("%s\n", instr.assign.expr.term.value);
        break;
      }
      break;
    case EXPR_ADD:
      printf("%s + %s\n", instr.assign.expr.add.lhs.value,
             instr.assign.expr.add.rhs.value);
      break;
    case EXPR_SUBTRACT:
      printf("%s i %s\n", instr.assign.expr.subtract.lhs.value,
             instr.assign.expr.subtract.rhs.value);
      break;
    case EXPR_MULTIPLY:
      printf("%s * %s\n", instr.assign.expr.multiply.lhs.value,
             instr.assign.expr.multiply.rhs.value);
      break;
    case EXPR_DIVIDE:
      printf("%s / %s\n", instr.assign.expr.divide.lhs.value,
             instr.assign.expr.divide.rhs.value);
      break;
    case EXPR_MODULO:
      printf("%s %% %s\n", instr.assign.expr.modulo.lhs.value,
             instr.assign.expr.modulo.rhs.value);
      break;
    }
    break;
  case INSTR_IF:
    if (instr.if_.rel.kind == REL_GREATER_THAN) {
      printf("if: %s > %s\n", instr.if_.rel.greater_than.lhs.value,
             instr.if_.rel.greater_than.rhs.value);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    } else {
      printf("if: %s < %s\n", instr.if_.rel.less_than.lhs.value,
             instr.if_.rel.less_than.rhs.value);
      printf("\t then: ");
      print_instr(*instr.if_.instr);
      break;
    }
  case INSTR_GOTO:
    printf("goto: %s\n", instr.goto_.label);
    break;
  case INSTR_OUTPUT:
    printf("output: %s\n", instr.output.term.value);
    break;
  case INSTR_LABEL:
    printf("label: %s\n", instr.label.label);
    break;
  }
}

void print_program(program_node *program) {
  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);
    print_instr(instr);
  }
}

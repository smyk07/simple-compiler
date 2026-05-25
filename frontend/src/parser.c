/*
 * parser: recursive-descent parser for the SCULL language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "frontend/parser.h"
#include "frontend/ast.h"
#include "frontend/token.h"
#include "frontend/types.h"
#include "frontend/var.h"

#include "core/common.h"
#include "core/ds/arena.h"
#include "core/ds/dynamic_array.h"
#include "core/ds/ht.h"
#include "core/utils.h"

#include <inttypes.h>

static mem_arena *ast_arena;

/*
 * @struct parser: represents the parser's internal state.
 */
typedef struct parser {
  dynamic_array tokens;
  u64 index;
} parser;

/*
 * @brief: Initializes the parser struct.
 *
 * @param tokens: pointer to dynamic_array of tokens.
 * @param p: pointer to an uninitialized parser struct.
 */
static void parser_init(dynamic_array *tokens, parser *p) {
  p->tokens = *tokens;
  p->index = 0;
}

/*
 * @brief: check the token at the current position of the parser.
 *
 * @param p: pointer to the parser state.
 * @param token: pointer to a new un-initialized token struct.
 */
static void parser_current(parser *p, token *token) {
  dynamic_array_get(&p->tokens, p->index, token);
}

/*
 * @brief: advance the parser state to the next token.
 *
 * @param p: pointer to the parser state.
 */
static void parser_advance(parser *p) { p->index++; }

/*
 * @brief: parse an instruction. (declaration)
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_instr(parser *p, instr_node *instr);

/*
 * @brief: parse a term. (declaration)
 *
 * @param p: pointer to the parser state.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_term(parser *p, arithmetic_expr_node **aexpr);

/*
 * @brief: parse a arithmetic expression. (declaration)
 *
 * @param p: pointer to the parser state.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_arithmetic_expr(parser *p,
                                        arithmetic_expr_node **aexpr);

/*
 * @brief: parse an individual term.
 *
 * @param p: pointer to the parser state.
 * @param term: pointer to an un-initialized term_node struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_term_for_expr(parser *p, term_node *term) {
  token token = {0};

  parser_current(p, &token);
  term->line = token.line;

  if (token.kind == TOKEN_INT_LITERAL) {
    term->kind = TERM_INT;
    term->value.integer = token.value.integer;
    parser_advance(p);
  }

  else if (token.kind == TOKEN_CHAR_LITERAL) {
    term->kind = TERM_CHAR;
    term->value.character = token.value.character;
    parser_advance(p);
  }

  else if (token.kind == TOKEN_STRING_LITERAL) {
    term->kind = TERM_STRING;
    term->value.str = token.value.str;
    parser_advance(p);
  }

  else if (token.kind == TOKEN_IDENTIFIER) {
    term->kind = TERM_IDENTIFIER;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;

    parser_advance(p);
    parser_current(p, &token);

    if (token.kind == TOKEN_LSQBR) {
      term->kind = TERM_ARRAY_ACCESS;
      term->array_access.array_var.name = term->identifier.name;
      term->array_access.array_var.line = term->identifier.line;
      parser_advance(p);

      SCU_TRY(parse_arithmetic_expr(p, &term->array_access.index_expr));

      parser_current(p, &token);
      if (token.kind != TOKEN_RSQBR) {
        scu_perror("Expected ']' at line %d\n", token.line);
        return SCU_ERR_PARSE;
      }
      parser_advance(p);
    }

    else if (token.kind == TOKEN_LPAREN) {
      term->kind = TERM_FUNCTION_CALL;
      term->fn_call.name = term->identifier.name;

      dynamic_array_init(&term->fn_call.parameters,
                         sizeof(arithmetic_expr_node));
      parser_advance(p);
      parser_current(p, &token);

      while (token.kind != TOKEN_RPAREN) {
        arithmetic_expr_node *arg = NULL;
        SCU_TRY(parse_arithmetic_expr(p, &arg));
        dynamic_array_append(&term->fn_call.parameters, arg);

        parser_current(p, &token);
        if (token.kind == TOKEN_COMMA) {
          parser_advance(p);
          parser_current(p, &token);
        }
      }

      if (token.kind != TOKEN_RPAREN) {
        scu_perror("Expected ')' at line %d\n", token.line);
        return SCU_ERR_PARSE;
      }

      parser_advance(p);
    }
  }

  else if (token.kind == TOKEN_ADDRESS_OF) {
    term->kind = TERM_ADDOF;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;
    parser_advance(p);
  }

  else if (token.kind == TOKEN_POINTER) {
    term->kind = TERM_DEREF;
    term->identifier.line = token.line;
    term->identifier.name = token.value.str;
    parser_advance(p);
  }

  else {
    scu_perror("Expected a term (input, int, char, identifier, addof, "
               "pointer), got %s [line %d]\n",
               token_kind_to_str(token.kind), token.line);
    return SCU_ERR_PARSE;
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse a factor inside an arithmetic expression.
 *
 * @param p: pointer to the parser state.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_factor(parser *p, arithmetic_expr_node **aexpr) {
  token token = {0};
  parser_current(p, &token);

  if (token.kind == TOKEN_SUBTRACT) {
    parser_advance(p);
    arithmetic_expr_node *operand = NULL;
    SCU_TRY(parse_term(p, &operand));

    arithmetic_expr_node *node =
        arena_push_struct(ast_arena, arithmetic_expr_node);
    node->kind = AR_EXPR_UNARY_MINUS;
    node->line = token.line;
    node->unary = operand;
    *aexpr = node;
    return SCU_SUCCESS;
  }

  if (token.kind == TOKEN_INT_LITERAL || token.kind == TOKEN_CHAR_LITERAL ||
      token.kind == TOKEN_IDENTIFIER || token.kind == TOKEN_POINTER ||
      token.kind == TOKEN_STRING_LITERAL || token.kind == TOKEN_ADDRESS_OF) {
    arithmetic_expr_node *node =
        arena_push_struct(ast_arena, arithmetic_expr_node);
    node->kind = AR_EXPR_TERM;
    node->line = token.line;

    if (token.kind == TOKEN_INT_LITERAL) {
      node->term.kind = TERM_INT;
      node->term.value.integer = token.value.integer;
      parser_advance(p);
      *aexpr = node;
      return SCU_SUCCESS;
    } else if (token.kind == TOKEN_CHAR_LITERAL) {
      node->term.kind = TERM_CHAR;
      node->term.value.character = token.value.character;
      parser_advance(p);
      *aexpr = node;
      return SCU_SUCCESS;
    } else if (token.kind == TOKEN_STRING_LITERAL) {
      node->term.kind = TERM_STRING;
      node->term.value.str = token.value.str;
      parser_advance(p);
      *aexpr = node;
      return SCU_SUCCESS;
    } else if (token.kind == TOKEN_IDENTIFIER) {
      node->term.kind = TERM_IDENTIFIER;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;

      parser_advance(p);
      parser_current(p, &token);

      if (token.kind == TOKEN_LSQBR) {
        node->term.kind = TERM_ARRAY_ACCESS;
        node->term.array_access.array_var.name = node->term.identifier.name;
        node->term.array_access.array_var.line = node->term.identifier.line;

        parser_advance(p);

        SCU_TRY(parse_arithmetic_expr(p, &node->term.array_access.index_expr));

        parser_current(p, &token);
        if (token.kind != TOKEN_RSQBR) {
          scu_perror("Expected ']' at line %d\n", token.line);
          return SCU_ERR_PARSE;
        }
        parser_advance(p);
      } else if (token.kind == TOKEN_LPAREN) {
        node->term.kind = TERM_FUNCTION_CALL;
        node->term.fn_call.name = node->term.identifier.name;

        dynamic_array_init(&node->term.fn_call.parameters,
                           sizeof(arithmetic_expr_node));
        parser_advance(p);
        parser_current(p, &token);

        while (token.kind != TOKEN_RPAREN) {
          arithmetic_expr_node *arg = NULL;
          SCU_TRY(parse_arithmetic_expr(p, &arg));
          dynamic_array_append(&node->term.fn_call.parameters, arg);

          parser_current(p, &token);
          if (token.kind == TOKEN_COMMA) {
            parser_advance(p);
            parser_current(p, &token);
          }
        }

        if (token.kind != TOKEN_RPAREN) {
          scu_perror("Expected ')' at line %d\n", token.line);
          return SCU_ERR_PARSE;
        }
        parser_advance(p);
      }
      *aexpr = node;
      return SCU_SUCCESS;
    } else if (token.kind == TOKEN_POINTER) {
      node->term.kind = TERM_DEREF;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;
      parser_advance(p);
      *aexpr = node;
      return SCU_SUCCESS;
    } else if (token.kind == TOKEN_ADDRESS_OF) {
      node->term.kind = TERM_ADDOF;
      node->term.identifier.line = token.line;
      node->term.identifier.name = token.value.str;
      parser_advance(p);
      *aexpr = node;
      return SCU_SUCCESS;
    }
  } else if (token.kind == TOKEN_LPAREN) {
    parser_advance(p);
    arithmetic_expr_node *node = NULL;
    SCU_TRY(parse_arithmetic_expr(p, &node));
    parser_current(p, &token);
    if (token.kind != TOKEN_RPAREN) {
      scu_perror("Syntax error: expected ')' at line %d\n", token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);
    *aexpr = node;
    return SCU_SUCCESS;
  }

  scu_perror("Syntax error: expected term or '(' at line %d\n", token.line);
  return SCU_ERR_PARSE;
}

/*
 * @brief: parse a term.
 *
 * @param p: pointer to the parser state.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_term(parser *p, arithmetic_expr_node **aexpr) {
  arithmetic_expr_node *left = NULL;
  SCU_TRY(parse_factor(p, &left));

  while (1) {
    token token = {0};
    parser_current(p, &token);

    if (token.kind == TOKEN_MULTIPLY || token.kind == TOKEN_DIVIDE ||
        token.kind == TOKEN_MODULO) {
      parser_advance(p);
      arithmetic_expr_node *right = NULL;
      SCU_TRY(parse_factor(p, &right));

      arithmetic_expr_node *parent =
          arena_push_struct(ast_arena, arithmetic_expr_node);
      parent->line = token.line;

      if (token.kind == TOKEN_MULTIPLY)
        parent->kind = AR_EXPR_MULTIPLY;
      else if (token.kind == TOKEN_DIVIDE)
        parent->kind = AR_EXPR_DIVIDE;
      else
        parent->kind = AR_EXPR_MODULO;

      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }

  *aexpr = left;
  return SCU_SUCCESS;
}

/*
 * @brief: parse a arithmetic expression. (definition)
 *
 * @param p: pointer to the parser state.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_arithmetic_expr(parser *p,
                                        arithmetic_expr_node **aexpr) {
  arithmetic_expr_node *left = NULL;
  SCU_TRY(parse_term(p, &left));

  while (1) {
    token token = {0};
    parser_current(p, &token);

    if (token.kind == TOKEN_ADD || token.kind == TOKEN_SUBTRACT) {
      parser_advance(p);
      arithmetic_expr_node *right = NULL;
      SCU_TRY(parse_term(p, &right));

      arithmetic_expr_node *parent =
          arena_push_struct(ast_arena, arithmetic_expr_node);
      parent->kind = (token.kind == TOKEN_ADD) ? AR_EXPR_ADD : AR_EXPR_SUBTRACT;
      parent->line = token.line;
      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }

  *aexpr = left;
  return SCU_SUCCESS;
}

static scu_result try_parse_rel(parser *p, rel_node *rel, term_node *lhs) {
  typedef struct {
    token_kind token_type;
    rel_kind rel_type;
  } rel_mapping;

  static const rel_mapping mappings[] = {
      {TOKEN_IS_EQUAL, REL_IS_EQUAL},
      {TOKEN_NOT_EQUAL, REL_NOT_EQUAL},
      {TOKEN_LESS_THAN, REL_LESS_THAN},
      {TOKEN_LESS_THAN_OR_EQUAL, REL_LESS_THAN_OR_EQUAL},
      {TOKEN_GREATER_THAN, REL_GREATER_THAN},
      {TOKEN_GREATER_THAN_OR_EQUAL, REL_GREATER_THAN_OR_EQUAL}};

  token token = {0};
  parser_current(p, &token);
  rel->line = token.line;

  for (u64 i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
    if (token.kind == mappings[i].token_type) {
      parser_advance(p);
      term_node rhs = {0};
      SCU_TRY(parse_term_for_expr(p, &rhs));
      rel->kind = mappings[i].rel_type;
      rel->comparison.lhs = *lhs;
      rel->comparison.rhs = rhs;
      return SCU_SUCCESS;
    }
  }
  return SCU_ERR_PARSE;
}

static scu_result parse_logical_or(parser *p, expr_node *expr);

static scu_result parse_logical_not(parser *p, expr_node *expr) {
  token token = {0};
  parser_current(p, &token);

  if (token.kind == TOKEN_LPAREN) {
    parser_advance(p);
    SCU_TRY(parse_logical_or(p, expr));
    parser_current(p, &token);
    if (token.kind != TOKEN_RPAREN) {
      scu_perror("Expected ')' [line %" PRIu64 "]\n", token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);
    return SCU_SUCCESS;
  }

  if (token.kind == TOKEN_NOT) {
    parser_advance(p);
    expr_node *operand = arena_push_struct(ast_arena, expr_node);
    SCU_TRY(parse_logical_not(p, operand));
    expr->kind = EXPR_LOGICAL;
    expr->logical.kind = LOG_NOT;
    expr->logical.line = token.line;
    expr->logical.unary.operand = operand;
    return SCU_SUCCESS;
  }

  if (token.kind == TOKEN_BOOL_LITERAL) {
    expr->kind = EXPR_BOOL;
    expr->boolean = token.value.boolean;
    parser_advance(p);
    return SCU_SUCCESS;
  }

  term_node lhs = {0};
  SCU_TRY(parse_term_for_expr(p, &lhs));

  if (try_parse_rel(p, &expr->relational, &lhs) == SCU_SUCCESS) {
    expr->kind = EXPR_RELATIONAL;
  } else {
    expr->kind = EXPR_TERM;
    expr->term = lhs;
  }

  return SCU_SUCCESS;
}

static scu_result parse_logical_and(parser *p, expr_node *expr) {
  SCU_TRY(parse_logical_not(p, expr));

  token token = {0};
  parser_current(p, &token);

  while (token.kind == TOKEN_AND) {
    parser_advance(p);

    expr_node *lhs = arena_push_struct(ast_arena, expr_node);
    expr_node *rhs = arena_push_struct(ast_arena, expr_node);
    *lhs = *expr;

    SCU_TRY(parse_logical_not(p, rhs));

    expr->kind = EXPR_LOGICAL;
    expr->logical.kind = LOG_AND;
    expr->logical.line = token.line;
    expr->logical.binary.lhs = lhs;
    expr->logical.binary.rhs = rhs;

    parser_current(p, &token);
  }

  return SCU_SUCCESS;
}

static scu_result parse_logical_or(parser *p, expr_node *expr) {
  SCU_TRY(parse_logical_and(p, expr));

  token token = {0};
  parser_current(p, &token);

  while (token.kind == TOKEN_OR) {
    parser_advance(p);

    expr_node *lhs = arena_push_struct(ast_arena, expr_node);
    expr_node *rhs = arena_push_struct(ast_arena, expr_node);
    *lhs = *expr;

    SCU_TRY(parse_logical_and(p, rhs));

    expr->kind = EXPR_LOGICAL;
    expr->logical.kind = LOG_OR;
    expr->logical.line = token.line;
    expr->logical.binary.lhs = lhs;
    expr->logical.binary.rhs = rhs;

    parser_current(p, &token);
  }

  return SCU_SUCCESS;
}

static scu_result parse_expr(parser *p, expr_node *expr) {
  SCU_TRY(parse_logical_or(p, expr));

  return SCU_SUCCESS;
}

/*
 * @brief: parse a variable initialize instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_initialize(parser *p, instr_node *instr, type _type,
                                   char *_name) {
  instr->kind = INSTR_INITIALIZE;
  instr->initialize_variable.var.type = _type;
  instr->initialize_variable.var.name = _name;
  parser_advance(p);

  if (_type == TYPE_BOOL) {
    SCU_TRY(parse_expr(p, &instr->initialize_variable.boolean));
  } else {
    SCU_TRY(parse_arithmetic_expr(p, &instr->initialize_variable.arithmetic));
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse an array initialize instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_initialize_array(parser *p, instr_node *instr,
                                         type _type, char *_name,
                                         arithmetic_expr_node *size_expr) {
  instr->kind = INSTR_INITIALIZE_ARRAY;
  instr->initialize_array.var.type = _type;
  instr->initialize_array.var.name = _name;
  instr->initialize_array.size_expr = size_expr;
  parser_advance(p);

  token token = {0};
  parser_current(p, &token);

  if (token.kind != TOKEN_LBRACE) {
    scu_perror("Expected '{' at line %d\n", token.line);
    return SCU_ERR_PARSE;
  }
  parser_advance(p);

  dynamic_array_init(&instr->initialize_array.literal.elements,
                     sizeof(arithmetic_expr_node));

  while (1) {
    parser_current(p, &token);
    if (token.kind == TOKEN_RBRACE) {
      break;
    }

    arithmetic_expr_node *elem = NULL;
    SCU_TRY(parse_arithmetic_expr(p, &elem));
    dynamic_array_append(&instr->initialize_array.literal.elements, elem);

    parser_current(p, &token);
    if (token.kind == TOKEN_COMMA) {
      parser_advance(p);
    } else if (token.kind == TOKEN_RBRACE) {
      break;
    } else {
      scu_perror("Expected '}' or ',' at line %d\n", token.line);
      return SCU_ERR_PARSE;
    }
  }

  parser_advance(p);

  return SCU_SUCCESS;
}

/*
 * @brief: parse a variable declare instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_declare(parser *p, instr_node *instr) {
  token token = {0};

  type _type = TYPE_VOID;
  char *_name;
  u32 _line;
  bool is_array = false;
  arithmetic_expr_node *size_expr = NULL;

  parser_current(p, &token);
  instr->line = token.line;
  _type = type_from_specifier_token(token.kind);
  parser_advance(p);

  parser_current(p, &token);
  if (_type == TYPE_CHAR && token.kind == TOKEN_POINTER)
    _type = TYPE_STRING;
  _name = token.value.str;
  _line = token.line;
  parser_advance(p);

  parser_current(p, &token);
  if (token.kind == TOKEN_LSQBR) {
    is_array = true;
    parser_advance(p);

    SCU_TRY(parse_arithmetic_expr(p, &size_expr));

    parser_current(p, &token);
    if (token.kind != TOKEN_RSQBR) {
      scu_perror("Expected ']' at line %d\n", token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);
  }

  parser_current(p, &token);

  if (token.kind == TOKEN_ASSIGN) {
    if (is_array) {
      SCU_TRY(parse_initialize_array(p, instr, _type, _name, size_expr));
    } else {
      SCU_TRY(parse_initialize(p, instr, _type, _name));
    }
  } else {
    if (is_array) {
      instr->kind = INSTR_DECLARE_ARRAY;
      instr->declare_array.var.type = _type;
      instr->declare_array.var.name = _name;
      instr->declare_array.var.line = _line;
      instr->declare_array.size_expr = size_expr;
    } else {
      instr->kind = INSTR_DECLARE;
      instr->declare_variable.type = _type;
      instr->declare_variable.name = _name;
      instr->declare_variable.line = _line;
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse a function call.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_fn_call(parser *p, instr_node *instr) {
  token token = {0};
  parser_current(p, &token);

  instr->kind = INSTR_FN_CALL;
  instr->line = token.line;
  instr->fn_call.name = token.value.str;

  parser_advance(p);
  parser_current(p, &token);

  if (token.kind != TOKEN_LPAREN) {
    scu_perror("Expected '(' after function name [line %d]\n", token.line);
    return SCU_ERR_PARSE;
  }

  parser_advance(p);
  parser_current(p, &token);

  dynamic_array_init(&instr->fn_call.parameters, sizeof(arithmetic_expr_node));

  while (token.kind != TOKEN_RPAREN) {
    arithmetic_expr_node *arg = NULL;
    SCU_TRY(parse_arithmetic_expr(p, &arg));
    dynamic_array_append(&instr->fn_call.parameters, arg);

    parser_current(p, &token);
    if (token.kind == TOKEN_COMMA) {
      parser_advance(p);
      parser_current(p, &token);
    }
  }

  if (token.kind != TOKEN_RPAREN) {
    scu_perror("Expected ')' after function arguments [line %d]\n", token.line);
    return SCU_ERR_PARSE;
  }

  parser_advance(p);

  return SCU_SUCCESS;
}

/*
 * @brief: parse an assign instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_assign(parser *p, instr_node *instr) {
  token token = {0};

  parser_current(p, &token);

  u64 ident_line = instr->line = token.line;
  char *ident_name = token.value.str;

  if (token.kind == TOKEN_POINTER) {
    instr->assign.identifier.type = TYPE_POINTER;
  }

  parser_advance(p);
  parser_current(p, &token);

  if (token.kind == TOKEN_LSQBR) {
    instr->kind = INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT;
    instr->line = token.line;
    instr->assign_to_array_subscript.var.name = ident_name;
    instr->assign_to_array_subscript.var.line = ident_line;

    parser_advance(p);

    SCU_TRY(
        parse_arithmetic_expr(p, &instr->assign_to_array_subscript.index_expr));

    parser_current(p, &token);
    if (token.kind != TOKEN_RSQBR) {
      scu_perror("Expected ], found %s [line %d]\n",
                 token_kind_to_str(token.kind), token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);
    parser_current(p, &token);

    if (token.kind != TOKEN_ASSIGN) {
      scu_perror("Expected assign, found %s [line %d]\n",
                 token_kind_to_str(token.kind), token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);

    SCU_TRY(parse_arithmetic_expr(
        p, &instr->assign_to_array_subscript.expr_to_assign));
  } else if (token.kind == TOKEN_LPAREN) {
    p->index--;
    SCU_TRY(parse_fn_call(p, instr));
  } else {
    instr->kind = INSTR_ASSIGN;
    instr->line = ident_line;
    instr->assign.identifier.name = ident_name;

    if (token.kind != TOKEN_ASSIGN) {
      scu_perror("Expected assign, found %s [line %d]\n",
                 token_kind_to_str(token.kind), token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);

    SCU_TRY(parse_arithmetic_expr(p, &instr->assign.expr));
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse a conditional block.
 *
 * @param p: pointer to the parser state.
 * @param block: pointer to the condiitonal block.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_cond_block(parser *p, cond_block_node *block) {
  token token = {0};
  parser_current(p, &token);

  if (token.kind == TOKEN_LBRACE) {
    block->kind = COND_MULTI_INSTR;
    parser_advance(p);

    dynamic_array_init(&block->multi, sizeof(instr_node));

    parser_current(p, &token);
    while (token.kind != TOKEN_RBRACE && token.kind != TOKEN_END) {
      instr_node *new_instr = arena_push_struct(ast_arena, instr_node);
      SCU_TRY(parse_instr(p, new_instr));
      dynamic_array_append(&block->multi, new_instr);

      parser_current(p, &token);
    }

    parser_advance(p);
    return SCU_SUCCESS;
  } else {
    block->kind = COND_SINGLE_INSTR;

    block->single = arena_push_struct(ast_arena, instr_node);
    SCU_TRY(parse_instr(p, block->single));

    return SCU_SUCCESS;
  }

  scu_perror("Expected a statement or '{', found %s [line %d]\n",
             token_kind_to_str(token.kind), token.line);
  return SCU_ERR_PARSE;
}

/*
 * @brief: parse an if instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_if(parser *p, instr_node *instr) {
  token token = {0};

  instr->kind = INSTR_IF;
  instr->if_.else_ = NULL;

  parser_advance(p);
  SCU_TRY(parse_expr(p, &instr->if_.condition));

  parser_current(p, &token);
  instr->line = token.line;

  SCU_TRY(parse_cond_block(p, &instr->if_.then));

  parser_current(p, &token);

  dynamic_array_init(&instr->if_.else_ifs, sizeof(if_node));

  while (token.kind == TOKEN_ELSE) {
    parser_advance(p);
    parser_current(p, &token);

    if (token.kind == TOKEN_IF) {
      if_node else_if = {0};
      parser_advance(p);
      SCU_TRY(parse_expr(p, &else_if.condition));
      SCU_TRY(parse_cond_block(p, &else_if.then));
      dynamic_array_append(&instr->if_.else_ifs, &else_if);
      parser_current(p, &token);
    } else {
      instr->if_.else_ = arena_push_struct(ast_arena, cond_block_node);
      SCU_TRY(parse_cond_block(p, instr->if_.else_));
      break;
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse a match statement.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_match(parser *p, instr_node *instr) {
  token token = {0};

  instr->kind = INSTR_MATCH;
  instr->match.expr = arena_push_struct(ast_arena, arithmetic_expr_node);

  parser_advance(p);

  SCU_TRY(parse_arithmetic_expr(p, &instr->match.expr));

  parser_current(p, &token);
  instr->line = token.line;

  if (token.kind != TOKEN_LBRACE) {
    scu_perror("expected '{' after match expression [line %d]\n", token.line);
    return SCU_ERR_PARSE;
  }
  parser_advance(p);

  dynamic_array_init(&instr->match.cases, sizeof(match_case_node));

  parser_current(p, &token);
  while (token.kind != TOKEN_RBRACE && token.kind != TOKEN_END) {
    match_case_node case_node = {0};

    if (token.kind == TOKEN_UNDERSCORE) {
      case_node.kind = MATCH_CASE_DEFAULT;
      parser_advance(p);
    } else {
      arithmetic_expr_node *first_expr =
          arena_push_struct(ast_arena, arithmetic_expr_node);
      SCU_TRY(parse_arithmetic_expr(p, &first_expr));

      parser_current(p, &token);

      if (token.kind == TOKEN_ELLIPSIS) {
        case_node.kind = MATCH_CASE_RANGE;
        case_node.range.start = first_expr;

        parser_advance(p);

        case_node.range.end =
            arena_push_struct(ast_arena, arithmetic_expr_node);
        SCU_TRY(parse_arithmetic_expr(p, &case_node.range.end));
      } else {
        case_node.kind = MATCH_CASE_VALUES;
        dynamic_array_init(&case_node.values.values,
                           sizeof(arithmetic_expr_node *));
        dynamic_array_append(&case_node.values.values, &first_expr);

        parser_current(p, &token);
        while (token.kind == TOKEN_COMMA) {
          parser_advance(p);

          arithmetic_expr_node *next_expr =
              arena_push_struct(ast_arena, arithmetic_expr_node);

          SCU_TRY(parse_arithmetic_expr(p, &next_expr));
          dynamic_array_append(&case_node.values.values, &next_expr);

          parser_current(p, &token);
        }
      }
    }

    parser_current(p, &token);
    if (token.kind != TOKEN_DARROW) {
      scu_perror("Expected '=>' after match case pattern [line %d]\n",
                 token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);

    SCU_TRY(parse_cond_block(p, &case_node.body));

    dynamic_array_append(&instr->match.cases, &case_node);

    parser_current(p, &token);
  }

  if (token.kind != TOKEN_RBRACE) {
    scu_perror("expected '}' to close match block [line %d]\n", token.line);
    return SCU_ERR_PARSE;
  }

  parser_advance(p);

  return SCU_SUCCESS;
}

/*
 * @brief: parse a goto instruction.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_goto(parser *p, instr_node *instr) {
  token token = {0};

  instr->kind = INSTR_GOTO;

  parser_advance(p);

  parser_current(p, &token);
  instr->line = token.line;
  if (token.kind != TOKEN_LABEL) {
    scu_perror("Expected label, found %s [line %d]\n",
               token_kind_to_str(token.kind), token.line);
    return SCU_ERR_PARSE;
  }
  parser_advance(p);

  instr->goto_.label = token.value.str;

  return SCU_SUCCESS;
}

/*
 * @brief: parse a label.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_label(parser *p, instr_node *instr) {
  token token = {0};

  instr->kind = INSTR_LABEL;

  parser_current(p, &token);
  instr->line = token.line;
  instr->label.label = token.value.str;

  parser_advance(p);

  return SCU_SUCCESS;
}

/*
 * @brief: parse loops.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 * @param kind: the kind of loop to parse (UNCONDITIONAL or WHILE).
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_loop(parser *p, instr_node *instr, loop_kind kind) {
  token token = {0};

  parser_current(p, &token);
  instr->kind = INSTR_LOOP;
  instr->line = token.line;
  instr->loop.kind = kind;

  parser_advance(p);

  if (kind == LOOP_FOR) {
    parser_current(p, &token);

    if (token.kind != TOKEN_IDENTIFIER) {
      scu_perror("expected identifier after 'for' [line %d]\n", token.line);
      return SCU_ERR_PARSE;
    }

    instr->loop._for.iterator.name = token.value.str;
    instr->loop._for.iterator.type = TYPE_I32;

    parser_advance(p);
    parser_current(p, &token);

    if (token.kind != TOKEN_IN) {
      scu_perror("expected 'in' after loop variable [line %d]\n", token.line);
      return SCU_ERR_PARSE;
    }

    parser_advance(p);

    instr->loop._for.range_start =
        arena_push_struct(ast_arena, arithmetic_expr_node);
    SCU_TRY(parse_arithmetic_expr(p, &instr->loop._for.range_start));

    parser_current(p, &token);

    if (token.kind != TOKEN_ELLIPSIS) {
      scu_perror("expected '...' in for loop range [line %d]\n", token.line);
      return SCU_ERR_PARSE;
    }

    parser_advance(p);

    instr->loop._for.range_end =
        arena_push_struct(ast_arena, arithmetic_expr_node);
    SCU_TRY(parse_arithmetic_expr(p, &instr->loop._for.range_end));

  } else if (kind == LOOP_WHILE) {
    parser_current(p, &token);
    SCU_TRY(parse_expr(p, &instr->loop.conditional.break_condition));
  }

  instr->loop.variables = ht_create(sizeof(variable));
  dynamic_array_init(&instr->loop.instrs, sizeof(instr_node));

  parser_current(p, &token);
  if (token.kind != TOKEN_LBRACE) {
    const char *loop_type;
    switch (kind) {
    case LOOP_FOR:
      loop_type = "for";
      break;
    case LOOP_WHILE:
      loop_type = "while";
      break;
    case LOOP_DO_WHILE:
      loop_type = "dowhile";
      break;
    default:
      loop_type = "unconditional";
      break;
    }
    scu_perror("no opening brace for %s loop at %d\n", loop_type, token.line);
    return SCU_ERR_PARSE;
  }

  parser_advance(p);
  parser_current(p, &token);

  while (token.kind != TOKEN_RBRACE) {
    instr_node *_instr = arena_push_struct(ast_arena, instr_node);
    SCU_TRY(parse_instr(p, _instr));
    dynamic_array_append(&instr->loop.instrs, _instr);
    parser_current(p, &token);
  }

  parser_advance(p);

  if (kind == LOOP_DO_WHILE) {
    parser_current(p, &token);
    SCU_TRY(parse_expr(p, &instr->loop.conditional.break_condition));
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse functions.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_fn(parser *p, instr_node *instr) {
  token token = {0};
  parser_current(p, &token);
  instr->kind = INSTR_FN_DECLARE;
  instr->line = token.line;
  instr->fn_declare_node.kind = FN_DECLARED;

  parser_advance(p);
  parser_current(p, &token);
  instr->fn_declare_node.name = token.value.str;
  parser_advance(p);

  parser_current(p, &token);
  if (token.kind != TOKEN_LPAREN) {
    scu_perror("Syntax error: expected '('\n");
    return SCU_ERR_PARSE;
  }
  parser_advance(p);

  dynamic_array_init(&instr->fn_declare_node.parameters, sizeof(variable));

  while (1) {
    parser_current(p, &token);
    if (token.kind == TOKEN_RPAREN) {
      break;
    }

    if (token.kind == TOKEN_ELLIPSIS) {
      instr->fn_declare_node.is_variadic = true;
      parser_advance(p);
      break;
    }

    variable param = {0};
    param.type = type_from_specifier_token(token.kind);
    if (param.type == TYPE_INVALID) {
      scu_perror("Expected type, got %s line %d\n",
                 token_kind_to_str(token.kind), token.line);
      return SCU_ERR_PARSE;
    }
    parser_advance(p);

    parser_current(p, &token);
    if (token.kind == TOKEN_POINTER) {
      param.type = TYPE_POINTER;
      parser_advance(p);
    }

    parser_current(p, &token);
    param.name = token.value.str;
    dynamic_array_append(&instr->fn_declare_node.parameters, &param);
    parser_advance(p);

    parser_current(p, &token);
    if (token.kind == TOKEN_COMMA) {
      parser_advance(p);
    }
  }

  parser_advance(p);

  dynamic_array_init(&instr->fn_declare_node.returntypes, sizeof(type));
  parser_current(p, &token);

  if (token.kind == TOKEN_COLON) {
    parser_advance(p);
    parser_current(p, &token);

    while (token.kind != TOKEN_LBRACE && token.kind != TOKEN_END) {
      type ret_type = type_from_specifier_token(token.kind);
      if (ret_type == TYPE_INVALID)
        ret_type = TYPE_VOID;

      dynamic_array_append(&instr->fn_declare_node.returntypes, &ret_type);

      parser_advance(p);
      parser_current(p, &token);

      if (token.kind == TOKEN_COMMA) {
        parser_advance(p);
      } else {
        break;
      }
    }
  }

  parser_current(p, &token);
  if (token.kind == TOKEN_LBRACE) {
    instr->kind = INSTR_FN_DEFINE;
    instr->fn_define_node.kind = FN_DEFINED;
    instr->fn_define_node.defined.variables = ht_create(sizeof(variable));
    dynamic_array_init(&instr->fn_define_node.defined.instrs,
                       sizeof(instr_node));

    parser_advance(p);
    parser_current(p, &token);
    while (token.kind != TOKEN_RBRACE && token.kind != TOKEN_END) {
      instr_node *_instr = arena_push_struct(ast_arena, instr_node);
      SCU_TRY(parse_instr(p, _instr));
      dynamic_array_append(&instr->fn_define_node.defined.instrs, _instr);
      parser_current(p, &token);
    }
    parser_advance(p);
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse return statements.
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_ret(parser *p, instr_node *instr) {
  token token = {0};
  parser_current(p, &token);
  instr->kind = INSTR_RETURN;
  instr->line = token.line;
  dynamic_array_init(&instr->ret_node.returnvals, sizeof(arithmetic_expr_node));

  parser_advance(p);

  parser_current(p, &token);

  while (token.kind != TOKEN_RBRACE) {
    arithmetic_expr_node *expr = NULL;
    SCU_TRY(parse_arithmetic_expr(p, &expr));
    dynamic_array_append(&instr->ret_node.returnvals, expr);

    parser_current(p, &token);

    if (token.kind == TOKEN_COMMA) {
      parser_advance(p);
      parser_current(p, &token);
    } else {
      break;
    }
  }

  return SCU_SUCCESS;
}

/*
 * @brief: parse an instruction. (definition)
 *
 * @param p: pointer to the parser state.
 * @param instr: pointer to a newly malloc'd instr struct.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
static scu_result parse_instr(parser *p, instr_node *instr) {
  token token = {0};

  parser_current(p, &token);

  switch (token.kind) {
  case TOKEN_TYPE_I8:
  case TOKEN_TYPE_I16:
  case TOKEN_TYPE_I32:
  case TOKEN_TYPE_I64:
  case TOKEN_TYPE_I128:
  case TOKEN_TYPE_U8:
  case TOKEN_TYPE_U16:
  case TOKEN_TYPE_U32:
  case TOKEN_TYPE_U64:
  case TOKEN_TYPE_U128:
  case TOKEN_TYPE_BOOL:
  case TOKEN_TYPE_CHAR:
    SCU_TRY(parse_declare(p, instr));
    break;

  case TOKEN_IDENTIFIER:
  case TOKEN_POINTER:
    SCU_TRY(parse_assign(p, instr));
    break;

  case TOKEN_IF:
    SCU_TRY(parse_if(p, instr));
    break;

  case TOKEN_MATCH:
    SCU_TRY(parse_match(p, instr));
    break;

  case TOKEN_GOTO:
    SCU_TRY(parse_goto(p, instr));
    break;

  case TOKEN_LABEL:
    SCU_TRY(parse_label(p, instr));
    break;

  case TOKEN_LOOP:
    SCU_TRY(parse_loop(p, instr, LOOP_UNCONDITIONAL));
    break;

  case TOKEN_WHILE:
    SCU_TRY(parse_loop(p, instr, LOOP_WHILE));
    break;

  case TOKEN_DO_WHILE:
    SCU_TRY(parse_loop(p, instr, LOOP_DO_WHILE));
    break;

  case TOKEN_FOR:
    SCU_TRY(parse_loop(p, instr, LOOP_FOR));
    break;

  case TOKEN_BREAK:
    instr->kind = INSTR_LOOP_BREAK;
    instr->line = token.line;
    parser_advance(p);
    break;

  case TOKEN_CONTINUE:
    instr->kind = INSTR_LOOP_CONTINUE;
    instr->line = token.line;
    parser_advance(p);
    break;

  case TOKEN_FN:
    SCU_TRY(parse_fn(p, instr));
    break;

  case TOKEN_RETURN:
    SCU_TRY(parse_ret(p, instr));
    break;

  default:
    scu_perror("unexpected token: %s - '%s' [line %d]\n",
               token_kind_to_str(token.kind), token_get_value(token),
               token.line);
    return SCU_ERR_PARSE;
  }

  return SCU_SUCCESS;
}

scu_result parser_parse_program(dynamic_array *tokens, ast *program) {
  ast_arena = &program->arena;
  parser p;
  parser_init(tokens, &p);

  token token = {0};
  parser_current(&p, &token);

  while (token.kind != TOKEN_END) {
    instr_node *instr = arena_push_struct(ast_arena, instr_node);
    SCU_TRY(parse_instr(&p, instr));
    dynamic_array_append(&program->instrs, instr);
    parser_current(&p, &token);
  }

  ast_arena = NULL;

  return SCU_SUCCESS;
}

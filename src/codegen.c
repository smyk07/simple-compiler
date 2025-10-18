#include "codegen.h"
#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/ht.h"
#include "ds/stack.h"
#include "fasm.h"
#include "utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * @brief: generate assembly for arithmetic expressions. (declaration)
 *
 * @param expr: pointer to an expr_node.
 * @param variables: hash table of variables.
 */
static void expr_asm(expr_node *expr, ht *variables, program_node *program);

int evaluate_const_expr(expr_node *expr) {
  if (expr == NULL) {
    return 0;
  }

  switch (expr->kind) {
  case EXPR_TERM:
    if (expr->term.kind == TERM_INT) {
      return expr->term.value.integer;
    }
    fprintf(stderr, "Error: Array size must be a constant expression\n");
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
    int right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      fprintf(stderr, "Error: Division by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) / right;
  }

  case EXPR_MODULO: {
    int right = evaluate_const_expr(expr->binary.right);
    if (right == 0) {
      fprintf(stderr, "Error: Modulo by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left) % right;
  }

  default:
    fprintf(stderr, "Error: Unknown expression kind in array size\n");
    return 0;
  }
}

/*
 * @brief: get array size from declare_array_node
 *
 * @param node: pointer to declare_array_node
 * @return: size of the array in elements
 */
static int get_array_size_declare(declare_array_node *node) {
  if (node == NULL || node->size_expr == NULL) {
    return 0;
  }
  return evaluate_const_expr(node->size_expr);
}

/*
 * @brief: get array size from initialize_array_node
 *
 * @param node: pointer to initialize_array_node
 * @return: size of the array in elements
 */
static int get_array_size_initialize(initialize_array_node *node) {
  if (node == NULL || node->size_expr == NULL) {
    return 0;
  }
  return evaluate_const_expr(node->size_expr);
}

/*
 * @brief: get the absolute stack offset for an array
 */
static size_t get_array_base_offset(program_node *program, variable *array_var,
                                    ht *variables) {
  size_t offset = variables->count * 8;

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_DECLARE_ARRAY) {
      if (strcmp(instr.declare_array.var.name, array_var->name) == 0) {
        int size = get_array_size_declare(&instr.declare_array);
        return offset + (size * 4); // Return end of array
      }
      int size = get_array_size_declare(&instr.declare_array);
      offset += size * 4; // 4 bytes per int element
    } else if (instr.kind == INSTR_INITIALIZE_ARRAY) {
      if (strcmp(instr.initialize_array.var.name, array_var->name) == 0) {
        int size = get_array_size_initialize(&instr.initialize_array);
        return offset + (size * 4);
      }
      int size = get_array_size_initialize(&instr.initialize_array);
      offset += size * 4;
    }
  }
  return offset;
}

/*
 * @brief: generate assembly for terms.
 *
 * @param term: pointer to a term_node.
 * @param variables: hash table of variables.
 */
static void term_asm(term_node *term, ht *variables, program_node *program) {
  switch (term->kind) {
  case TERM_INT:
    printf("    mov rax, %d\n", term->value.integer);
    break;
  case TERM_CHAR: {
    printf("    mov rax, %d\n", term->value.character);
    break;
  }
  case TERM_IDENTIFIER: {
    int index = get_var_stack_offset(variables, &term->identifier, NULL);
    printf("    mov rax, qword [rbp - %d]\n", index * 8 + 8);
    break;
  }
  case TERM_POINTER:
    break;
  case TERM_DEREF: {
    int index = get_var_stack_offset(variables, &term->identifier, NULL);
    printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
    printf("    mov rax, qword [rbx]\n");
    break;
  }
  case TERM_ADDOF: {
    int index = get_var_stack_offset(variables, &term->identifier, NULL);
    printf("    lea rax, [rbp - %d]\n", index * 8 + 8);
    break;
  }

  case TERM_ARRAY_ACCESS: {
    size_t array_base = get_array_base_offset(
        program, &term->array_access.array_var, variables);
    expr_asm(term->array_access.index_expr, variables, program);
    printf("    cdqe\n");
    printf("    lea rdx, [rbp - %zu]\n", array_base);
    printf("    mov eax, dword [rdx + rax*4]\n");
    break;
  }

  case TERM_ARRAY_LITERAL: {
    // nothing here cuz its done at initialization itself
    break;
  }
  }
}

/*
 * @brief: generate assembly for arithmetic expressions. (definition)
 *
 * @param expr: pointer to an expr_node.
 * @param variables: hash table of variables.
 */
static void expr_asm(expr_node *expr, ht *variables, program_node *program) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_asm(&expr->term, variables, program);
    break;
  case EXPR_ADD:
    expr_asm(expr->binary.left, variables, program);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program);
    printf("    pop rdx\n");
    printf("    add rax, rdx\n");
    break;
  case EXPR_SUBTRACT:
    expr_asm(expr->binary.left, variables, program);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program);
    printf("    mov rdx, rax\n");
    printf("    pop rax\n");
    printf("    sub rax, rdx\n");
    break;
  case EXPR_MULTIPLY:
    expr_asm(expr->binary.left, variables, program);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program);
    printf("    pop rdx\n");
    printf("    imul rax, rdx\n");
    break;
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    expr_asm(expr->binary.left, variables, program);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program);
    printf("    mov rcx, rax\n");
    printf("    pop rax\n");
    printf("    cqo\n");
    printf("    idiv rcx\n");
    if (expr->kind == EXPR_MODULO) {
      printf("    mov rax, rdx\n");
    }
    break;
  }
}

/*
 * @brief: generate assembly for relational expressions
 *
 * @param rel: pointer to a rel_node.
 * @param variables: hash table of variables.
 */
static void rel_asm(rel_node *rel, ht *variables, program_node *program) {
  switch (rel->kind) {
  case REL_IS_EQUAL:
    term_asm(&rel->is_equal.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->is_equal.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    sete al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_NOT_EQUAL:
    term_asm(&rel->not_equal.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->not_equal.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    setne al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_LESS_THAN:
    term_asm(&rel->less_than.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->less_than.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    setl al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_LESS_THAN_OR_EQUAL:
    term_asm(&rel->less_than_or_equal.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->less_than_or_equal.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    setle al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_GREATER_THAN:
    term_asm(&rel->greater_than.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->greater_than.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    setg al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    term_asm(&rel->greater_than_or_equal.lhs, variables, program);
    printf("    mov rdx, rax\n");
    term_asm(&rel->greater_than_or_equal.rhs, variables, program);
    printf("    cmp rdx, rax\n");
    printf("    setge al\n");
    printf("    movzx rax, al\n");
    break;
  }
}

/*
 * @brief: generate assembly for individual expressions.
 *
 * @param instr: pointer ot an instr_node.
 * @param variables: hash table of variables.
 * @param if_count: counter for if instructions.
 */
static void instr_asm(instr_node *instr, ht *variables, unsigned int *if_count,
                      stack *loops, program_node *program) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    break;

  case INSTR_INITIALIZE: {
    int index =
        get_var_stack_offset(variables, &instr->initialize_variable.var, NULL);
    expr_asm(&instr->initialize_variable.expr, variables, program);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }

  case INSTR_ASSIGN: {
    int index =
        get_var_stack_offset(variables, &instr->assign.identifier, NULL);
    expr_asm(&instr->assign.expr, variables, program);
    if (instr->assign.identifier.type == TYPE_POINTER) {
      printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
      printf("    mov qword [rbx], rax\n");
    } else {
      printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    }
    break;
  }

  case INSTR_DECLARE_ARRAY: {
    break;
  }

  case INSTR_INITIALIZE_ARRAY: {
    size_t array_base =
        get_array_base_offset(program, &instr->initialize_array.var, variables);
    printf("    lea rdx, [rbp - %zu]\n", array_base);

    for (size_t i = 0; i < instr->initialize_array.literal.elements.count;
         i++) {
      expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);

      expr_asm(&elem, variables, program);
      printf("    mov dword [rdx + %zu], eax\n", i * 4);
    }
    break;
  }

  case INSTR_IF: {
    rel_asm(&instr->if_.rel, variables, program);
    int label = (*if_count)++;
    printf("    test rax, rax\n");
    printf("    jz .endif%d\n", label);
    instr_asm(instr->if_.instr, variables, if_count, loops, program);
    printf("    .endif%d:\n", label);
    break;
  }

  case INSTR_GOTO:
    printf("    jmp .%s\n", instr->goto_.label);
    break;

  case INSTR_LABEL:
    printf(".%s:\n", instr->label.label);
    break;

  case INSTR_FASM_DEFINE:
    // This is supposed to do nothing as it is already handled
    break;

  case INSTR_FASM:
    if (instr->fasm.kind == ARG) {
      int index = get_var_stack_offset(variables, &instr->fasm.argument, NULL);
      char *stmt =
          scu_format_string((char *)instr->fasm.content, index * 8 + 8);
      printf("    %s\n", stmt);
      free(stmt);
    } else {
      printf("    %s\n", instr->fasm.content);
    }
    break;

  case INSTR_LOOP:
    loop_node new_loop = {.loop_id = instr->loop.loop_id};
    stack_push(loops, &new_loop);
    printf(".loop_%zu_start:\n", instr->loop.loop_id);
    for (unsigned int i = 0; i < instr->loop.instrs.count; i++) {
      struct instr_node _instr;
      dynamic_array_get(&instr->loop.instrs, i, &_instr);
      instr_asm(&_instr, variables, if_count, loops, program);
    }
    loop_node *loop = malloc(sizeof(loop_node));
    stack_pop(loops, loop);
    printf(".loop_%zu_end:\n", instr->loop.loop_id);
    free(loop);
    break;

  case INSTR_LOOP_BREAK:
    loop = stack_top(loops);
    printf("    jmp .loop_%zu_end\n", loop->loop_id);
    break;

  case INSTR_LOOP_CONTINUE:
    loop = stack_top(loops);
    printf("    jmp .loop_%zu_start\n", loop->loop_id);
    break;
  }
}

static size_t calculate_total_stack_size(ht *variables, program_node *program) {

  size_t total = variables->count * 8;

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_DECLARE_ARRAY) {
      total += get_array_size_declare(&instr.declare_array) * 4; // FIXED: * 4
    } else if (instr.kind == INSTR_INITIALIZE_ARRAY) {
      total +=
          get_array_size_initialize(&instr.initialize_array) * 4; // FIXED: * 4
    }
  }

  if (total % 16 != 0) {
    total += 16 - (total % 16);
  }
  return total;
}

void instrs_to_asm(program_node *program, ht *variables, stack *loops,
                   const char *filename) {
  unsigned int if_count = 0;

  char *output_asm_file = scu_format_string("%s.s", filename);
  freopen(output_asm_file, "w", stdout);

  printf("format ELF64 executable\n");
  printf("LINE_MAX equ 1024\n");
  printf("segment readable executable\n");

  // fasm definitions
  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_FASM_DEFINE) {
      printf("%s\n", instr.fasm_def.content);
      dynamic_array_remove(&program->instrs, i);
    }
  }

  printf("entry _start\n");

  printf("segment readable writeable\n");

  printf("segment readable executable\n");
  printf("_start:\n");
  printf("    mov rbp, rsp\n");

  size_t stack_size = calculate_total_stack_size(variables, program);
  printf("    sub rsp, %zu\n", stack_size);

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    instr_asm(&instr, variables, &if_count, loops, program);
  }

  printf("    add rsp, %zu\n", stack_size);

  printf("    mov rax, 60\n");
  printf("    xor rdi, rdi\n");
  printf("    syscall\n");

  printf("segment readable writeable\n");
  printf("line rb LINE_MAX\n");
  printf("newline db 10, 0\n");
  printf("char_buf db 0, 0\n");

  fflush(stdout);
  fclose(stdout);

  fasm_assemble(output_asm_file, filename);
  free(output_asm_file);
}

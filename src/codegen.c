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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void expr_asm(expr_node *expr, ht *variables, program_node *program,
                     unsigned int *errors);

int evaluate_const_expr(expr_node *expr, unsigned int *errors) {
  if (expr == NULL) {
    return 0;
  }

  switch (expr->kind) {
  case EXPR_TERM:
    if (expr->term.kind == TERM_INT) {
      return expr->term.value.integer;
    }
    scu_perror(errors, "Array size must be a constant expression\n");
    return 0;

  case EXPR_ADD:
    return evaluate_const_expr(expr->binary.left, errors) +
           evaluate_const_expr(expr->binary.right, errors);

  case EXPR_SUBTRACT:
    return evaluate_const_expr(expr->binary.left, errors) -
           evaluate_const_expr(expr->binary.right, errors);

  case EXPR_MULTIPLY:
    return evaluate_const_expr(expr->binary.left, errors) *
           evaluate_const_expr(expr->binary.right, errors);

  case EXPR_DIVIDE: {
    int right = evaluate_const_expr(expr->binary.right, errors);
    if (right == 0) {
      scu_perror(errors, "Division by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left, errors) / right;
  }

  case EXPR_MODULO: {
    int right = evaluate_const_expr(expr->binary.right, errors);
    if (right == 0) {
      scu_perror(errors, "Division by zero in array size\n");
      return 0;
    }
    return evaluate_const_expr(expr->binary.left, errors) % right;
  }
  }
}

/*
 * @brief: get array size from declare_array_node
 *
 * @param node: pointer to declare_array_node
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: size of the array in elements
 */
static int get_array_size_declare(declare_array_node *node,
                                  unsigned int *errors) {
  if (node == NULL || node->size_expr == NULL) {
    return 0;
  }
  return evaluate_const_expr(node->size_expr, errors);
}

/*
 * @brief: get array size from initialize_array_node
 *
 * @param node: pointer to initialize_array_node
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: size of the array in elements
 */
static int get_array_size_initialize(initialize_array_node *node,
                                     unsigned int *errors) {
  if (node == NULL || node->size_expr == NULL) {
    return 0;
  }
  return evaluate_const_expr(node->size_expr, errors);
}

/*
 * @brief: get the absolute stack offset for an array
 */
static size_t get_array_base_offset(program_node *program, variable *array_var,
                                    ht *variables, unsigned int *errors) {
  size_t offset = variables->count * 8;

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_DECLARE_ARRAY) {
      if (strcmp(instr.declare_array.var.name, array_var->name) == 0) {
        int size = get_array_size_declare(&instr.declare_array, errors);
        return offset + (size * 4);
      }
      int size = get_array_size_declare(&instr.declare_array, errors);
      offset += size * 4;
    } else if (instr.kind == INSTR_INITIALIZE_ARRAY) {
      if (strcmp(instr.initialize_array.var.name, array_var->name) == 0) {
        int size = get_array_size_initialize(&instr.initialize_array, errors);
        return offset + (size * 4);
      }
      int size = get_array_size_initialize(&instr.initialize_array, errors);
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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void term_asm(term_node *term, ht *variables, program_node *program,
                     unsigned int *errors) {
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
    if (term->identifier.is_array)
      printf("    lea rax, [rbp - %d]\n", index * 8 + 8);
    else
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
        program, &term->array_access.array_var, variables, errors);
    expr_asm(term->array_access.index_expr, variables, program, errors);
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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void expr_asm(expr_node *expr, ht *variables, program_node *program,
                     unsigned int *errors) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_asm(&expr->term, variables, program, errors);
    break;
  case EXPR_ADD:
    expr_asm(expr->binary.left, variables, program, errors);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program, errors);
    printf("    pop rdx\n");
    printf("    add rax, rdx\n");
    break;
  case EXPR_SUBTRACT:
    expr_asm(expr->binary.left, variables, program, errors);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program, errors);
    printf("    mov rdx, rax\n");
    printf("    pop rax\n");
    printf("    sub rax, rdx\n");
    break;
  case EXPR_MULTIPLY:
    expr_asm(expr->binary.left, variables, program, errors);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program, errors);
    printf("    pop rdx\n");
    printf("    imul rax, rdx\n");
    break;
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    expr_asm(expr->binary.left, variables, program, errors);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables, program, errors);
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
 * @param errors: counter variable to increment when an error is encountered.
 */
static void rel_asm(rel_node *rel, ht *variables, program_node *program,
                    unsigned int *errors) {
  term_asm(&rel->comparison.lhs, variables, program, errors);
  printf("    push rax\n");
  term_asm(&rel->comparison.rhs, variables, program, errors);
  printf("    pop rdx\n");
  printf("    cmp rdx, rax\n");

  switch (rel->kind) {
  case REL_IS_EQUAL:
    printf("    sete al\n");
    break;
  case REL_NOT_EQUAL:
    printf("    setne al\n");
    break;
  case REL_LESS_THAN:
    printf("    setl al\n");
    break;
  case REL_LESS_THAN_OR_EQUAL:
    printf("    setle al\n");
    break;
  case REL_GREATER_THAN:
    printf("    setg al\n");
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    printf("    setge al\n");
    break;
  }

  printf("    movzx rax, al\n");
}

/*
 * @brief: generate assembly for individual expressions.
 *
 * @param instr: pointer ot an instr_node.
 * @param variables: hash table of variables.
 * @param if_count: counter for if instructions.
 * @param errors: counter variable to increment when an error is encountered.
 */
static void instr_asm(instr_node *instr, ht *variables, unsigned int *if_count,
                      stack *loops, program_node *program,
                      unsigned int *errors) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    break;

  case INSTR_INITIALIZE: {
    int index =
        get_var_stack_offset(variables, &instr->initialize_variable.var, NULL);
    expr_asm(&instr->initialize_variable.expr, variables, program, errors);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }

  case INSTR_ASSIGN: {
    int index =
        get_var_stack_offset(variables, &instr->assign.identifier, NULL);
    expr_asm(&instr->assign.expr, variables, program, errors);
    if (instr->assign.identifier.type == TYPE_POINTER) {
      printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
      printf("    mov qword [rbx], rax\n");
    } else {
      printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    }
    break;
  }

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    size_t array_base = get_array_base_offset(
        program, &instr->assign_to_array_subscript.var, variables, errors);
    expr_asm(&instr->assign_to_array_subscript.expr_to_assign, variables,
             program, errors);
    printf("    push rax\n");
    expr_asm(instr->assign_to_array_subscript.index_expr, variables, program,
             errors);
    printf("    mov rcx, rax\n");
    printf("    lea rdx, [rbp - %zu]\n", array_base);
    printf("    pop rax\n");
    printf("    mov dword [rdx + rcx * 4], eax\n");
    break;

  case INSTR_DECLARE_ARRAY: {
    break;
  }

  case INSTR_INITIALIZE_ARRAY: {
    size_t array_base = get_array_base_offset(
        program, &instr->initialize_array.var, variables, errors);
    printf("    lea rdx, [rbp - %zu]\n", array_base);

    for (size_t i = 0; i < instr->initialize_array.literal.elements.count;
         i++) {
      expr_node elem;
      dynamic_array_get(&instr->initialize_array.literal.elements, i, &elem);

      expr_asm(&elem, variables, program, errors);
      printf("    mov dword [rdx + %zu], eax\n", i * 4);
    }
    break;
  }

  case INSTR_IF: {
    rel_asm(&instr->if_.rel, variables, program, errors);
    int label = (*if_count)++;
    printf("    test rax, rax\n");
    printf("    jz .endif%d\n", label);
    switch (instr->if_.kind) {
    case IF_SINGLE_INSTR:
      instr_asm(instr->if_.instr, variables, if_count, loops, program, errors);
      break;

    case IF_MULTI_INSTR:
      for (size_t i = 0; i < instr->if_.instrs.count; i++) {
        struct instr_node _instr;
        dynamic_array_get(&instr->if_.instrs, i, &_instr);
        instr_asm(&_instr, variables, if_count, loops, program, errors);
      }
      break;
    }
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
    if (instr->fasm.kind == FASM_PAR) {
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
    loop_node new_loop = {.kind = instr->loop.kind,
                          .loop_id = instr->loop.loop_id,
                          .instrs = {0},
                          .break_condition = {0}};
    stack_push(loops, &new_loop);

    switch (instr->loop.kind) {
    case LOOP_UNCONDITIONAL:
      printf(".loop_%zu_start:\n", instr->loop.loop_id);
      for (unsigned int i = 0; i < instr->loop.instrs.count; i++) {
        struct instr_node _instr;
        dynamic_array_get(&instr->loop.instrs, i, &_instr);
        instr_asm(&_instr, variables, if_count, loops, program, errors);
      }
      printf(".loop_%zu_end:\n", instr->loop.loop_id);
      break;

    case LOOP_WHILE:
      printf("    jmp .loop_%zu_test\n", instr->loop.loop_id);

    case LOOP_DO_WHILE:
      printf(".loop_%zu_start:\n", instr->loop.loop_id);
      for (unsigned int i = 0; i < instr->loop.instrs.count; i++) {
        struct instr_node _instr;
        dynamic_array_get(&instr->loop.instrs, i, &_instr);
        instr_asm(&_instr, variables, if_count, loops, program, errors);
      }
      printf(".loop_%zu_test:\n", instr->loop.loop_id);
      rel_asm(&instr->loop.break_condition, variables, program, errors);
      printf("    test rax, rax\n");
      printf("    jz .loop_%zu_end\n", instr->loop.loop_id);
      printf("    jmp .loop_%zu_start\n", instr->loop.loop_id);
      printf(".loop_%zu_end:\n", instr->loop.loop_id);
      break;
    }

    loop_node loop;
    stack_pop(loops, &loop);
    break;

  case INSTR_LOOP_BREAK: {
    loop_node *_loop = stack_top(loops);
    printf("    jmp .loop_%zu_end\n", _loop->loop_id);
    break;
  }

  case INSTR_LOOP_CONTINUE:
    loop_node *_loop = stack_top(loops);
    switch (_loop->kind) {
    case LOOP_UNCONDITIONAL:
      printf("    jmp .loop_%zu_start\n", _loop->loop_id);
      break;
    case LOOP_WHILE:
    case LOOP_DO_WHILE:
      printf("    jmp .loop_%zu_test\n", _loop->loop_id);
      break;
    }
    break;
  }
}

static size_t calculate_total_stack_size(ht *variables, program_node *program,
                                         unsigned int *errors) {
  size_t total = variables->count * 8;

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_DECLARE_ARRAY) {
      total += get_array_size_declare(&instr.declare_array, errors) * 4;
    } else if (instr.kind == INSTR_INITIALIZE_ARRAY) {
      total += get_array_size_initialize(&instr.initialize_array, errors) * 4;
    }
  }

  if (total % 16 != 0) {
    total += 16 - (total % 16);
  }
  return total;
}

void instrs_to_asm(program_node *program, ht *variables, stack *loops,
                   const char *filename, unsigned int *errors) {
  unsigned int if_count = 0;

  char *output_asm_file = scu_format_string("%s.s", filename);
  freopen(output_asm_file, "w", stdout);

  // Initialization and fasm definitions
  printf("format ELF64 executable\n");
  printf("LINE_MAX equ 1024\n");

  printf("entry _start\n");
  printf("segment readable executable\n");

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    if (instr.kind == INSTR_FASM_DEFINE) {
      printf("%s\n", instr.fasm_def.content);
      dynamic_array_remove(&program->instrs, i);
    }
  }

  // main function
  printf("\nmain:\n");
  printf("    push rbp\n");
  printf("    mov rbp, rsp\n");

  size_t stack_size = calculate_total_stack_size(variables, program, errors);
  printf("    sub rsp, %zu\n", stack_size);

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    instr_asm(&instr, variables, &if_count, loops, program, errors);
  }

  printf("    add rsp, %zu\n", stack_size);
  printf("    pop rbp\n");
  printf("    ret\n");

  // entrypoint
  printf("\n_start:\n");
  printf("    call main\n");
  printf("    mov rax, 60\n");
  printf("    xor rdi, rdi\n");
  printf("    syscall\n\n");

  printf("segment readable writeable\n");
  printf("line rb LINE_MAX\n");
  printf("newline db 10, 0\n");
  printf("char_buf db 0, 0\n");

  fflush(stdout);
  fclose(stdout);

  fasm_assemble(output_asm_file, filename);
  free(output_asm_file);
}

#include "codegen.h"
#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/stack.h"
#include "fasm.h"
#include "semantic.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * @brief: generate assembly for terms.
 *
 * @param term: pointer to a term_node.
 * @param variables: dynamic_array of variables.
 */
static void term_asm(term_node *term, dynamic_array *variables) {
  switch (term->kind) {
  case TERM_INPUT: {
    printf("    read 0, line, LINE_MAX\n");
    printf("    mov rdi, line\n");
    printf("    call strlen\n");
    printf("    mov rdi, line\n");
    printf("    mov rsi, rax\n");
    printf("    call parse_uint\n");
    break;
  }
  case TERM_INT:
    printf("    mov rax, %d\n", term->value.integer);
    break;
  case TERM_CHAR: {
    printf("    mov rax, %d\n", term->value.character);
    break;
  }
  case TERM_IDENTIFIER: {
    int index = find_variables(variables, &term->identifier, NULL);
    printf("    mov rax, qword [rbp - %d]\n", index * 8 + 8);
    break;
  }
  case TERM_POINTER:
    break;
  case TERM_DEREF: {
    int index = find_variables(variables, &term->identifier, NULL);
    printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
    printf("    mov rax, qword [rbx]\n");
    break;
  }
  case TERM_ADDOF: {
    int index = find_variables(variables, &term->identifier, NULL);
    printf("    lea rax, [rbp - %d]\n", index * 8 + 8);
    break;
  }
  }
}

/*
 * @brief: generate assembly for arithmetic expressions.
 *
 * @param expr: pointer to an expr_node.
 * @param variables: dynamic_array of variables.
 */
static void expr_asm(expr_node *expr, dynamic_array *variables) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_asm(&expr->term, variables);
    break;
  case EXPR_ADD:
    expr_asm(expr->binary.left, variables);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables);
    printf("    pop rdx\n");
    printf("    add rax, rdx\n");
    break;
  case EXPR_SUBTRACT:
    expr_asm(expr->binary.left, variables);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables);
    printf("    mov rdx, rax\n");
    printf("    pop rax\n");
    printf("    sub rax, rdx\n");
    break;
  case EXPR_MULTIPLY:
    expr_asm(expr->binary.left, variables);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables);
    printf("    pop rdx\n");
    printf("    imul rax, rdx\n");
    break;
  case EXPR_DIVIDE:
  case EXPR_MODULO:
    expr_asm(expr->binary.left, variables);
    printf("    push rax\n");
    expr_asm(expr->binary.right, variables);
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
 * @param variables: dynamic_array of variables.
 */
static void rel_asm(rel_node *rel, dynamic_array *variables) {
  switch (rel->kind) {
  case REL_IS_EQUAL:
    term_asm(&rel->is_equal.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->is_equal.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    sete al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_NOT_EQUAL:
    term_asm(&rel->not_equal.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->not_equal.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    setne al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_LESS_THAN:
    term_asm(&rel->less_than.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->less_than.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    setl al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_LESS_THAN_OR_EQUAL:
    term_asm(&rel->less_than_or_equal.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->less_than_or_equal.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    setle al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_GREATER_THAN:
    term_asm(&rel->greater_than.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->greater_than.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    setg al\n");
    printf("    movzx rax, al\n");
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    term_asm(&rel->greater_than_or_equal.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->greater_than_or_equal.rhs, variables);
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
 * @param variables: dynamic_array of variables.
 * @param if_count: counter for if instructions.
 */
static void instr_asm(instr_node *instr, dynamic_array *variables,
                      unsigned int *if_count, stack *loops) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    break;

  case INSTR_INITIALIZE: {
    int index =
        find_variables(variables, &instr->initialize_variable.var, NULL);
    expr_asm(&instr->initialize_variable.expr, variables);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }

  case INSTR_ASSIGN: {
    int index = find_variables(variables, &instr->assign.identifier, NULL);
    expr_asm(&instr->assign.expr, variables);
    if (instr->assign.identifier.type == TYPE_POINTER) {
      printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
      printf("    mov qword [rbx], rax\n");
    } else {
      printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    }
    break;
  }

  case INSTR_IF: {
    rel_asm(&instr->if_.rel, variables);
    int label = (*if_count)++;
    printf("    test rax, rax\n");
    printf("    jz .endif%d\n", label);
    instr_asm(instr->if_.instr, variables, if_count, loops);
    printf("    .endif%d:\n", label);
    break;
  }

  case INSTR_GOTO:
    printf("    jmp .%s\n", instr->goto_.label);
    break;

  case INSTR_OUTPUT: {
    switch (instr->output.term.kind) {
    case TERM_INPUT:
      break;
    case TERM_INT:
      printf("    mov rsi, %ld\n", (long)instr->output.term.value.integer);
      printf("    mov rdi, 1\n");
      printf("    call write_uint\n");
      printf("    mov rsi, newline\n");
      printf("    mov rdi, 1\n");
      printf("    call write_cstr\n");
      break;
    case TERM_CHAR:
      printf("    mov al, %d\n", instr->output.term.value.character);
      printf("    mov [char_buf], al\n");
      printf("    mov rdi, 1\n");
      printf("    mov rsi, char_buf\n");
      printf("    mov rdx, 2\n");
      printf("    mov rax, 1\n");
      printf("    syscall\n");
      break;
    case TERM_IDENTIFIER: {
      int index =
          find_variables(variables, &instr->output.term.identifier, NULL);
      variable var;
      dynamic_array_get(variables, index, &var);
      if (var.type == TYPE_CHAR) {
        printf("    mov al, byte [rbp - %d]\n", index * 8 + 8);
        printf("    mov [char_buf], al\n");
        printf("    mov rdi, 1\n");
        printf("    mov rsi, char_buf\n");
        printf("    mov rdx, 2\n");
        printf("    mov rax, 1\n");
        printf("    syscall\n");
      } else if (var.type == TYPE_INT) {
        printf("    mov rsi, qword [rbp - %d]\n", index * 8 + 8);
        printf("    mov rdi, 1\n");
        printf("    call write_uint\n");
        printf("    mov rsi, newline\n");
        printf("    mov rdi, 1\n");
        printf("    call write_cstr\n");
      }
      break;
    }
    case TERM_POINTER:
      break;
    case TERM_DEREF: {
      // Only prints ints for now
      term_asm(&instr->output.term, variables);
      printf("    mov rsi, rax\n");
      printf("    mov rdi, 1\n");
      printf("    call write_uint\n");
      printf("    mov rsi, newline\n");
      printf("    mov rdi, 1\n");
      printf("    call write_cstr\n");
      break;
    }
    case TERM_ADDOF: {
      int index =
          find_variables(variables, &instr->output.term.identifier, NULL);
      printf("    lea rsi, [rbp - %d]\n", index * 8 + 8);
      printf("    mov rdi, 1\n");
      printf("    call write_uint\n");
      printf("    mov rsi, newline\n");
      printf("    mov rdi, 1\n");
      printf("    call write_cstr\n");
      break;
    }
    }
  } break;

  case INSTR_LABEL:
    printf(".%s:\n", instr->label.label);
    break;

  case INSTR_FASM_DEFINE:
    // This is supposed to do nothing as it is already handled
    break;

  case INSTR_FASM:
    if (instr->fasm.kind == ARG) {
      int index = find_variables(variables, &instr->fasm.argument, NULL);
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
    printf("loop_%zu_start:\n", instr->loop.loop_id);
    for (unsigned int i = 0; i < instr->loop.instrs.count; i++) {
      struct instr_node _instr;
      dynamic_array_get(&instr->loop.instrs, i, &_instr);
      instr_asm(&_instr, variables, if_count, loops);
    }
    loop_node *loop = malloc(sizeof(loop_node *));
    stack_pop(loops, loop);
    printf("loop_%zu_end:\n", instr->loop.loop_id);
    free(loop);
    break;

  case INSTR_LOOP_BREAK:
    loop = stack_top(loops);
    printf("    jmp loop_%zu_end\n", loop->loop_id);
    break;

  case INSTR_LOOP_CONTINUE:
    loop = stack_top(loops);
    printf("    jmp loop_%zu_start\n", loop->loop_id);
    break;
  }
}

void instrs_to_asm(program_node *program, dynamic_array *variables,
                   stack *loops, const char *filename) {
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
  printf("    sub rsp, %zu\n", variables->count * 8);

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    instr_asm(&instr, variables, &if_count, loops);
  }

  printf("    add rsp, %zu\n", variables->count * 8);

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

#include "codegen.h"
#include "data_structures.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

#include <stdio.h>

void term_asm(term_node *term, dynamic_array *variables) {
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
    int index = find_variables(variables, &term->identifier);
    printf("    mov rax, qword [rbp - %d]\n", index * 8 + 8);
    break;
  }
  }
}

void expr_asm(expr_node *expr, dynamic_array *variables) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_asm(&expr->term, variables);
    break;
  case EXPR_ADD:
    term_asm(&expr->add.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->add.rhs, variables);
    printf("    add rax, rdx\n");
    break;
  case EXPR_SUBTRACT:
    term_asm(&expr->subtract.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->subtract.rhs, variables);
    printf("    sub rdx, rax\n");
    printf("    mov rax, rdx\n");
    break;
  case EXPR_MULTIPLY:
    term_asm(&expr->multiply.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->multiply.rhs, variables);
    printf("    imul rax, rdx\n");
    break;
  case EXPR_DIVIDE:
    term_asm(&expr->divide.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->divide.rhs, variables);
    printf("    mov rcx, rax\n");
    printf("    mov rax, rdx\n");
    printf("    xor rdx, rdx\n");
    printf("    div rcx\n");
    break;
  case EXPR_MODULO:
    term_asm(&expr->modulo.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->modulo.rhs, variables);
    printf("    mov rcx, rax\n");
    printf("    mov rax, rdx\n");
    printf("    xor rdx, rdx\n");
    printf("    div rcx\n");
    printf("    mov rax, rdx\n");
    break;
  }
}

void rel_asm(rel_node *rel, dynamic_array *variables) {
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

void instr_asm(instr_node *instr, dynamic_array *variables, int *if_count) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    break;
  case INSTR_INITIALIZE: {
    int index = find_variables(variables, &instr->initialize_variable.var);
    expr_asm(&instr->initialize_variable.expr, variables);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }
  case INSTR_ASSIGN: {
    int index = find_variables(variables, &instr->assign.identifier);
    expr_asm(&instr->assign.expr, variables);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }
  case INSTR_IF: {
    rel_asm(&instr->if_.rel, variables);
    int label = (*if_count)++;
    printf("    test rax, rax\n");
    printf("    jz .endif%d\n", label);
    instr_asm(instr->if_.instr, variables, if_count);
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
      int index = find_variables(variables, &instr->output.term.identifier);
      variable var;
      dynamic_array_get(variables, index, &var);
      if (var.type == TOKEN_TYPE_CHAR) {
        printf("    mov al, byte [rbp - %d]\n", index * 8 + 8);
        printf("    mov [char_buf], al\n");
        printf("    mov rdi, 1\n");
        printf("    mov rsi, char_buf\n");
        printf("    mov rdx, 2\n");
        printf("    mov rax, 1\n");
        printf("    syscall\n");
      } else {
        printf("    mov rsi, qword [rbp - %d]\n", index * 8 + 8);
        printf("    mov rdi, 1\n");
        printf("    call write_uint\n");
        printf("    mov rsi, newline\n");
        printf("    mov rdi, 1\n");
        printf("    call write_cstr\n");
      }
      break;
    }
    }
  } break;
  case INSTR_LABEL:
    printf(".%s:\n", instr->label.label);
    break;
  }
}

void program_asm(program_node *program, dynamic_array *variables) {
  int if_count = 0;

  printf("format ELF64 executable\n");
  printf("LINE_MAX equ 1024\n");
  printf("segment readable executable\n");
  printf("include \"linux.inc\"\n");
  printf("include \"utils.inc\"\n");
  printf("entry _start\n");

  printf("segment readable writeable\n");

  printf("segment readable executable\n");
  printf("_start:\n");
  printf("    mov rbp, rsp\n");
  printf("    sub rsp, %d\n", variables->count * 8);

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    instr_asm(&instr, variables, &if_count);
  }

  printf("    add rsp, %d\n", variables->count * 8);

  printf("    mov rax, 60\n");
  printf("    xor rdi, rdi\n");
  printf("    syscall\n");

  printf("segment readable writeable\n");
  printf("line rb LINE_MAX\n");
  printf("newline db 10, 0\n");
  printf("char_buf db 0, 0\n");
}

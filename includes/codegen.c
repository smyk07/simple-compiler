#include "codegen.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int find_variables(dynamic_array *variables, char *ident) {
  for (unsigned int i = 0; i < variables->count; i++) {
    char *variable = NULL;
    dynamic_array_get(variables, i, &variable);

    if (strcmp(ident, variable) == 0)
      return i;
  }

  return -1;
}

void term_asm(term_node *term, dynamic_array *variables) {
  switch (term->kind) {
  case TERM_INPUT:
    printf("   read 0, line, LINE_MAX\n");
    printf("   mov rdi, line\n");
    printf("   call strlen\n");
    printf("   mov rdi, line\n");
    printf("   mov rsi, rax\n");
    printf("   call parse_uint\n");
    break;
  case TERM_INT:
    printf("    mov rax, %d\n", term->value.integer);
    break;
  case TERM_CHAR: {
    printf("    mov rax, %d\n", term->value.character);
    break;
  }
  case TERM_IDENTIFIER: {
    int index = find_variables(variables, term->value.str);
    if (index == -1) {
      printf("Use of undeclared variable '%s'\n", term->value.str);
      exit(1);
    }
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
    printf("    sub rax, rcx\n");
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
  case INSTR_ASSIGN: {
    expr_asm(&instr->assign.expr, variables);
    int index = find_variables(variables, instr->assign.identifier);
    if (index == -1) {
      printf("Use of undeclared variable '%s'\n", instr->assign.identifier);
      exit(1);
    }
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
  case INSTR_OUTPUT:
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
      printf("    mov rax, %d\n", instr->output.term.value.character);
      printf("    mov [char_buf], al\n");
      printf("    mov byte [char_buf+1], 10\n");
      printf("    mov rdi, 1\n");
      printf("    mov rsi, char_buf\n");
      printf("    mov rdx, 2\n");
      printf("    syscall\n");
      break;
    case TERM_IDENTIFIER: {
      int index = find_variables(variables, instr->output.term.value.str);
      if (index == -1) {
        printf("Use of undeclared variable '%s'\n",
               instr->output.term.value.str);
        exit(1);
      }
      printf("    ;; TERM_IDENTIFIER output\n");
      printf("    mov rsi, qword [rbp - %d]\n", index * 8 + 8);
      printf("    mov rdi, 1\n"); // stdout
      printf("    call write_uint\n");
      printf("    mov rsi, newline\n");
      printf("    mov rdi, 1\n");
      printf("    call write_cstr\n");
      break;
    }
    }
    break;
  case INSTR_LABEL:
    printf(".%s:\n", instr->label.label);
    break;
  }
}

void declare_variables(char **identifier, dynamic_array *variables) {
  for (unsigned int i = 0; i < variables->count; i++) {
    char *variable = NULL;
    dynamic_array_get(variables, i, &variable);

    if (strcmp(*identifier, variable) == 0) {
      return;
    }
  }

  dynamic_array_append(variables, identifier);
}

void term_check_variables(term_node *term, dynamic_array *variables) {
  switch (term->kind) {
  case TERM_INPUT:
    break;
  case TERM_INT:
    break;
  case TERM_CHAR:
    break;
  case TERM_IDENTIFIER:
    if (find_variables(variables, term->value.str) == -1) {
      fprintf(stderr, "Use of undeclared variable: %s\n", term->value.str);
      exit(1);
    }
    break;
  }
}

void expr_check_variables(expr_node *expr, dynamic_array *variables) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_check_variables(&expr->term, variables);
    break;
  case EXPR_ADD:
    term_check_variables(&expr->add.lhs, variables);
    term_check_variables(&expr->add.rhs, variables);
    break;
  case EXPR_SUBTRACT:
    term_check_variables(&expr->subtract.lhs, variables);
    term_check_variables(&expr->subtract.rhs, variables);
    break;
  case EXPR_MULTIPLY:
    term_check_variables(&expr->subtract.lhs, variables);
    term_check_variables(&expr->subtract.rhs, variables);
    break;
  case EXPR_DIVIDE:
    term_check_variables(&expr->divide.lhs, variables);
    term_check_variables(&expr->divide.rhs, variables);
    break;
  case EXPR_MODULO:
    term_check_variables(&expr->modulo.lhs, variables);
    term_check_variables(&expr->modulo.rhs, variables);
    break;
  }
}

void rel_check_variables(rel_node *rel, dynamic_array *variables) {
  switch (rel->kind) {
  case REL_IS_EQUAL:
    term_check_variables(&rel->is_equal.lhs, variables);
    term_check_variables(&rel->is_equal.rhs, variables);
    break;
  case REL_NOT_EQUAL:
    term_check_variables(&rel->not_equal.lhs, variables);
    term_check_variables(&rel->not_equal.rhs, variables);
    break;
  case REL_LESS_THAN:
    term_check_variables(&rel->less_than.lhs, variables);
    term_check_variables(&rel->less_than.rhs, variables);
    break;
  case REL_LESS_THAN_OR_EQUAL:
    term_check_variables(&rel->less_than_or_equal.lhs, variables);
    term_check_variables(&rel->less_than_or_equal.rhs, variables);
    break;
  case REL_GREATER_THAN:
    term_check_variables(&rel->greater_than.lhs, variables);
    term_check_variables(&rel->greater_than.rhs, variables);
    break;
  case REL_GREATER_THAN_OR_EQUAL:
    term_check_variables(&rel->greater_than_or_equal.lhs, variables);
    term_check_variables(&rel->greater_than_or_equal.rhs, variables);
    break;
  }
}

void instr_declare_variables(instr_node *instr, dynamic_array *variables) {
  switch (instr->kind) {
  case INSTR_DECLARE:
    declare_variables(&instr->declare_variable.identifier, variables);
    break;
  case INSTR_ASSIGN:
    expr_check_variables(&instr->assign.expr, variables);
    if (find_variables(variables, instr->assign.identifier) == -1) {
      printf("Use of undeclared variable: %s\n", instr->assign.identifier);
      exit(1);
    }
    break;
  case INSTR_IF:
    rel_check_variables(&instr->if_.rel, variables);
    instr_declare_variables(instr->if_.instr, variables);
    break;
  case INSTR_GOTO:
    break;
  case INSTR_OUTPUT:
    term_check_variables(&instr->output.term, variables);
    break;
  case INSTR_LABEL:
    break;
  }
}

void program_asm(program_node *program) {
  int if_count = 0;

  dynamic_array variables;
  dynamic_array_init(&variables, sizeof(char *));

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);
    instr_declare_variables(&instr, &variables);
  }

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
  printf("    sub rsp, %d\n", variables.count * 8);

  for (unsigned int i = 0; i < program->instrs.count; i++) {
    struct instr_node instr;
    dynamic_array_get(&program->instrs, i, &instr);

    instr_asm(&instr, &variables, &if_count);
  }

  printf("    add rsp, %d\n", variables.count * 8);

  printf("    mov rax, 60\n");
  printf("    xor rdi, rdi\n");
  printf("    syscall\n");

  printf("segment readable writeable\n");
  printf("line rb LINE_MAX\n");
  printf("newline db 10, 0\n");
  printf("char_buf db 0, 0\n");
}

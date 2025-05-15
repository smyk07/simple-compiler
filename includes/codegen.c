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
    printf("    mov rax, %s\n", term->value);
    break;
  case TERM_CHAR: {
    char ch = term->value[0];
    int char_value;

    if (ch == '\\' && term->value[1] != '\0') {
      switch (term->value[1]) {
      case 'n':
        char_value = '\n';
        break;
      case 't':
        char_value = '\t';
        break;
      case 'r':
        char_value = '\r';
        break;
      case '0':
        char_value = '\0';
        break;
      case '\\':
        char_value = '\\';
        break;
      case '\'':
        char_value = '\'';
        break;
      default:
        char_value = term->value[1];
      }
    } else {
      char_value = ch;
    }

    printf("    mov rax, %d\n", char_value);
    break;
  }
  case TERM_IDENTIFIER: {
    int index = find_variables(variables, term->value);
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
    printf("    sub rax, rdx\n");
    break;
  case EXPR_MULTIPLY:
    term_asm(&expr->multiply.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&expr->multiply.rhs, variables);
    printf("    imul rax, rdx\n");
    break;
  case EXPR_DIVIDE:
    term_asm(&expr->divide.lhs, variables);
    printf("    push rdx, rax\n");
    term_asm(&expr->divide.rhs, variables);
    printf("    mov rcx, rax\n");
    printf("    pop rax\n");
    printf("    xor rdx, rdx\n");
    printf("    div rcx\n");
    break;
  case EXPR_MODULO:
    term_asm(&expr->modulo.lhs, variables);
    printf("    push rax\n");
    term_asm(&expr->modulo.rhs, variables);
    printf("    mov rcx, rax\n");
    printf("    pop rax\n");
    printf("    xor rdx, rdx\n");
    printf("    div rcx\n");
    printf("    mov rax, rdx\n");
    // TODO
    break;
  }
}

void rel_asm(rel_node *rel, dynamic_array *variables) {
  switch (rel->kind) {
  case REL_LESS_THAN:
    term_asm(&rel->less_than.lhs, variables);
    printf("    mov rdx, rax\n");
    term_asm(&rel->less_than.rhs, variables);
    printf("    cmp rdx, rax\n");
    printf("    setl al\n");
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
  }
}

void instr_asm(instr_node *instr, dynamic_array *variables, int *if_count) {
  switch (instr->kind) {
  case INSTR_ASSIGN: {
    expr_asm(&instr->assign.expr, variables);
    int index = find_variables(variables, instr->assign.identifier);
    printf("    mov qword [rbp - %d], rax\n", index * 8 + 8);
    break;
  }
  case INSTR_IF: {
    rel_asm(&instr->if_.rel, variables);
    int label = (*if_count)++;
    printf("    test rax, rax\n");
    printf("    jz .endif%d\n", label);
    instr_asm(instr->if_.instr, variables, if_count);
    printf(".endif%d:\n", label);
    break;
  }
  case INSTR_GOTO:
    printf("    jmp .%s\n", instr->goto_.label);
    break;
  case INSTR_OUTPUTI:
    term_asm(&instr->output.term, variables);
    printf("    push rdi\n");
    printf("    push rsi\n");
    printf("    mov rdi, 1\n");
    printf("    mov rsi, rax\n");
    printf("    call write_uint\n");
    printf("    pop rsi\n");
    printf("    pop rdi\n");
    break;
  case INSTR_OUTPUTC:
    term_asm(&instr->output.term, variables);
    printf("    push rax\n");
    printf("    sub rsp, 8\n");
    printf("    mov rdi, 1\n");
    printf("    mov rsi, rsp\n");
    printf("    add rsi, 8\n");
    printf("    mov rdx, 1\n");
    printf("    mov rax, 1\n");
    printf("    syscall\n");
    printf("    add rsp, 8\n");
    printf("    pop rax\n");
    break;
  case INSTR_LABEL:
    printf(".%s:\n", instr->label.label);
    break;
  }
}

void term_declare_variables(term_node *term, dynamic_array *variables) {
  switch (term->kind) {
  case TERM_INPUT:
    break;
  case TERM_INT:
    break;
  case TERM_CHAR:
    break;
  case TERM_IDENTIFIER:
    for (unsigned int i = 0; i < variables->count; i++) {
      char *variable = NULL;
      dynamic_array_get(variables, i, &variable);

      if (strcmp(term->value, variable) == 0) {
        return;
      }
    }

    printf("Error: Identifier is not defined: %s\n", term->value);
    exit(1);

    break;
  }
}

void expr_declare_variables(expr_node *expr, dynamic_array *variables) {
  switch (expr->kind) {
  case EXPR_TERM:
    term_declare_variables(&expr->term, variables);
    break;
  case EXPR_ADD:
    term_declare_variables(&expr->add.lhs, variables);
    term_declare_variables(&expr->add.lhs, variables);
    break;
  case EXPR_SUBTRACT:
    term_declare_variables(&expr->subtract.lhs, variables);
    term_declare_variables(&expr->subtract.lhs, variables);
    break;
  case EXPR_MULTIPLY:
    term_declare_variables(&expr->subtract.lhs, variables);
    term_declare_variables(&expr->subtract.lhs, variables);
    break;
  case EXPR_DIVIDE:
    term_declare_variables(&expr->divide.lhs, variables);
    term_declare_variables(&expr->divide.lhs, variables);
    break;
  case EXPR_MODULO:
    term_declare_variables(&expr->modulo.lhs, variables);
    term_declare_variables(&expr->modulo.lhs, variables);
    break;
  }
}

void rel_declare_variables(rel_node *rel, dynamic_array *variables) {
  switch (rel->kind) {
  case REL_LESS_THAN:
    term_declare_variables(&rel->less_than.lhs, variables);
    term_declare_variables(&rel->less_than.rhs, variables);
    break;
  case REL_GREATER_THAN:
    term_declare_variables(&rel->greater_than.lhs, variables);
    term_declare_variables(&rel->greater_than.rhs, variables);
    break;
  }
}

void instr_declare_variables(instr_node *instr, dynamic_array *variables) {
  switch (instr->kind) {
  case INSTR_ASSIGN:
    expr_declare_variables(&instr->assign.expr, variables);

    for (unsigned int i = 0; i < variables->count; i++) {
      char *variable = NULL;
      dynamic_array_get(variables, i, &variable);

      if (strcmp(instr->assign.identifier, variable) == 0) {
        return;
      }
    }

    dynamic_array_append(variables, &instr->assign.identifier);

    break;
  case INSTR_IF:
    rel_declare_variables(&instr->if_.rel, variables);
    instr_declare_variables(instr->if_.instr, variables);
    break;
  case INSTR_GOTO:
    break;
  case INSTR_OUTPUTI:
  case INSTR_OUTPUTC:
    term_declare_variables(&instr->output.term, variables);
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
}

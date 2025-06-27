#include "codegen.h"
#include "data_structures.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

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
  case TERM_POINTER:
    break;
  case TERM_DEREF: {
    int index = find_variables(variables, &term->identifier);
    printf("    mov rbx, qword [rbp - %d]\n", index * 8 + 8);
    printf("    mov rax, qword [rbx]\n");
    break;
  }
  case TERM_ADDOF: {
    int index = find_variables(variables, &term->identifier);
    printf("    lea rax, [rbp - %d]\n", index * 8 + 8);
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
      int index = find_variables(variables, &instr->output.term.identifier);
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
  }
}

void embed_runtime() {
  printf("\n");

  printf("SYS_read equ 0\n");
  printf("SYS_write equ 1\n");

  printf("macro syscall3 number, a, b, c\n");
  printf("{\n");
  printf("    mov rax, number\n");
  printf("    mov rdi, a\n");
  printf("    mov rsi, b\n");
  printf("    mov rdx, c\n");
  printf("    syscall\n");
  printf("}\n");

  printf("macro read fd, buf, count\n");
  printf("{\n");
  printf("    syscall3 SYS_read, fd, buf, count\n");
  printf("}\n");
  printf("\n");

  printf("macro write fd, buf, count\n");
  printf("{\n");
  printf("    syscall3 1, fd, buf, count\n");
  printf("}\n");

  printf("strlen:\n");
  printf("    push rdi\n");
  printf("    xor rax, rax\n");
  printf(".next_char:\n");
  printf("    mov al, byte [rdi]\n");
  printf("    cmp rax, 0\n");
  printf("    je .done\n");
  printf("\n");
  printf("    inc rdi\n");
  printf("    jmp .next_char\n");
  printf(".done:\n");
  printf("    pop rsi\n");
  printf("    sub rdi, rsi\n");
  printf("    mov rax, rdi\n");
  printf("    ret\n");

  printf("parse_uint:\n");
  printf("    xor rax, rax\n");
  printf("    xor rbx, rbx\n");
  printf("    mov rcx, 10\n");
  printf(".next_digit:\n");
  printf("    cmp rsi, 0\n");
  printf("    jle .done\n");
  printf("\n");
  printf("    mov bl, byte [rdi]\n");
  printf("    cmp rbx, '0'\n");
  printf("    jl .done\n");
  printf("    cmp rbx, '9'\n");
  printf("    jg .done\n");
  printf("    sub rbx, '0'\n");
  printf("\n");
  printf("    mul rcx\n");
  printf("    add rax, rbx\n");
  printf("\n");
  printf("    inc rdi\n");
  printf("    dec rsi\n");
  printf("    jmp .next_digit\n");
  printf(".done:\n");
  printf("    ret\n");

  printf("write_uint:\n");
  printf("    test rsi, rsi\n");
  printf("    jz .base_zero\n");
  printf("\n");
  printf("    mov rcx, 10\n");
  printf("    mov rax, rsi\n");
  printf("    mov r10, 0\n");
  printf(".next_digit:\n");
  printf("    test rax, rax\n");
  printf("    jz .done\n");
  printf("    mov rdx, 0\n");
  printf("    div rcx\n");
  printf("    add rdx, '0'\n");
  printf("    dec rsp\n");
  printf("    mov byte [rsp], dl\n");
  printf("    inc r10\n");
  printf("    jmp .next_digit\n");
  printf(".done:\n");
  printf("    write rdi, rsp, r10\n");
  printf("    add rsp, r10\n");
  printf("    ret\n");
  printf(".base_zero:\n");
  printf("    dec rsp\n");
  printf("    mov byte [rsp], '0'\n");
  printf("    write rdi, rsp, 1\n");
  printf("    inc rsp\n");
  printf("    ret\n");

  printf("write_cstr:\n");
  printf("    push rsi\n");
  printf("    push rdi\n");
  printf("    mov rdi, rsi\n");
  printf("    call strlen\n");
  printf("\n");
  printf("    mov rdx, rax\n");
  printf("    mov rax, SYS_write\n");
  printf("    pop rdi\n");
  printf("    pop rsi\n");
  printf("    syscall\n");
  printf("    ret\n");
  printf("\n");
}

void program_asm(program_node *program, dynamic_array *variables,
                 char *filename, unsigned int *errors) {
  int if_count = 0;

  char *output_asm_file = scu_format_string("%s.asm", filename);
  freopen(output_asm_file, "w", stdout);

  printf("format ELF64 executable\n");
  printf("LINE_MAX equ 1024\n");
  printf("segment readable executable\n");

  // embed runtime needed for the input and output instructions
  embed_runtime();

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

  fflush(stdout);
  fclose(stdout);

  scu_assemble(output_asm_file, filename, errors);
  free(output_asm_file);
}

#include "tests/prova.h"

#include "core/ds/arena.h"

#include <stddef.h>

#define ARENA_ALIGNMENT (alignof(max_align_t))

PTEST(arena_initialization_and_free) {
  mem_arena arena;
  arena_init(&arena);

  PROVA_ASSERT_NOT_NULL(arena.first);
  PROVA_ASSERT_EQUAL_PTR(arena.first, arena.current);
  PROVA_ASSERT_EQUAL(1 << 20, arena.first->capacity);
  PROVA_ASSERT_EQUAL(0, arena.first->pos);
  PROVA_ASSERT_NOT_NULL(arena.first->buffer);
  PROVA_ASSERT_NULL(arena.first->next);

  arena_free(&arena);
  PROVA_ASSERT_NULL(arena.first);
  PROVA_ASSERT_NULL(arena.current);
}

PTEST(arena_basic_push_and_alignment) {
  mem_arena arena;
  arena_init(&arena);

  u32 *num = arena_push_struct(&arena, u32);
  PROVA_ASSERT_NOT_NULL(num);

  PROVA_ASSERT_EQUAL(4, arena.current->pos);

  char *c = arena_push_struct(&arena, char);
  PROVA_ASSERT_NOT_NULL(c);
  PROVA_ASSERT_EQUAL(ARENA_ALIGNMENT + 1, arena.current->pos);

  arena_free(&arena);
}

PTEST(arena_block_chaining_on_overflow) {
  mem_arena arena;
  arena_init(&arena);

  arena.default_block_size = 32;
  arena.first->capacity = 32;

  mem_arena_block *block1 = arena.first;

  void *p1 = arena_push(&arena, 20);
  PROVA_ASSERT_NOT_NULL(p1);
  PROVA_ASSERT_EQUAL_PTR(block1, arena.current);

  void *p2 = arena_push(&arena, 20);
  PROVA_ASSERT_NOT_NULL(p2);

  mem_arena_block *block2 = arena.current;
  PROVA_ASSERT_NOT_EQUAL_PTR(block1, block2);
  PROVA_ASSERT_EQUAL_PTR(block1->next, block2);

  PROVA_ASSERT_EQUAL(20, block2->pos);
  PROVA_ASSERT_NULL(block2->next);

  arena_free(&arena);
}

PTEST(arena_pop_across_blocks) {
  mem_arena arena;
  arena_init(&arena);

  arena.default_block_size = 32;
  arena.first->capacity = 32;

  mem_arena_block *block1 = arena.first;

  arena_push(&arena, 20); // block 1 pos = 20
  arena_push(&arena, 20); // block 2 pos = 20

  mem_arena_block *block2 = arena.current;

  arena_pop(&arena, 5);
  PROVA_ASSERT_EQUAL_PTR(block2, arena.current);
  PROVA_ASSERT_EQUAL(15, arena.current->pos);

  arena_pop(&arena, 20);
  PROVA_ASSERT_EQUAL_PTR(block1, arena.current);
  PROVA_ASSERT_EQUAL(15, arena.current->pos);
  PROVA_ASSERT_EQUAL(0, block2->pos);

  arena_free(&arena);
}

PTEST(arena_clear_mechanics) {
  mem_arena arena;
  arena_init(&arena);

  arena.default_block_size = 32;
  arena.first->capacity = 32;

  arena_push(&arena, 20);
  arena_push(&arena, 20);
  arena_push(&arena, 20);

  mem_arena_block *block1 = arena.first;
  mem_arena_block *block2 = block1->next;
  mem_arena_block *block3 = block2->next;

  PROVA_ASSERT_EQUAL_PTR(block3, arena.current);

  arena_clear(&arena);

  PROVA_ASSERT_EQUAL_PTR(block1, arena.current);
  PROVA_ASSERT_EQUAL(0, block1->pos);
  PROVA_ASSERT_EQUAL(0, block2->pos);
  PROVA_ASSERT_EQUAL(0, block3->pos);

  PROVA_ASSERT_EQUAL_PTR(block2, block1->next);
  PROVA_ASSERT_EQUAL_PTR(block3, block2->next);

  arena_free(&arena);
}

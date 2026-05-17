#include "tests/prova.h"

#include "core/ds/stack.h"

typedef struct {
  i32 id;
  f64 data;
} StackItem;

PTEST(stack_init) {
  stack s;
  u32 result = stack_init(&s, sizeof(int));

  PROVA_ASSERT_EQUAL(0, result);
  PROVA_ASSERT_EQUAL(sizeof(i32), s.item_size);
  PROVA_ASSERT_EQUAL(0, s.count);
}

PTEST(stack_push_top) {
  stack s;
  stack_init(&s, sizeof(int));

  int v1 = 10;
  int v2 = 20;

  PROVA_ASSERT_EQUAL(0, stack_push(&s, &v1));
  PROVA_ASSERT_EQUAL(1, s.count);

  int *top_ptr = (int *)stack_top(&s);
  PROVA_ASSERT_NOT_NULL(top_ptr);
  PROVA_ASSERT_EQUAL(10, *top_ptr);

  PROVA_ASSERT_EQUAL(0, stack_push(&s, &v2));
  PROVA_ASSERT_EQUAL(2, s.count);

  top_ptr = (int *)stack_top(&s);
  PROVA_ASSERT_NOT_NULL(top_ptr);
  PROVA_ASSERT_EQUAL(20, *top_ptr);

  stack_free(&s);
}

PTEST(stack_pop) {
  stack s;
  stack_init(&s, sizeof(StackItem));

  StackItem item1 = {1, 1.1};
  StackItem item2 = {2, 2.2};
  StackItem item3 = {3, 3.3};

  stack_push(&s, &item1);
  stack_push(&s, &item2);
  stack_push(&s, &item3);
  PROVA_ASSERT_EQUAL(3, s.count);

  StackItem out;

  PROVA_ASSERT_EQUAL(0, stack_pop(&s, &out));
  PROVA_ASSERT_EQUAL(3, out.id);
  PROVA_ASSERT_EQUAL(2, s.count);

  PROVA_ASSERT_EQUAL(0, stack_pop(&s, &out));
  PROVA_ASSERT_EQUAL(2, out.id);
  PROVA_ASSERT_EQUAL(1, s.count);

  PROVA_ASSERT_EQUAL(0, stack_pop(&s, &out));
  PROVA_ASSERT_EQUAL(1, out.id);
  PROVA_ASSERT_EQUAL(0, s.count);

  stack_free(&s);
}

PTEST(stack_empty) {
  stack s;
  stack_init(&s, sizeof(int));

  int out_val = 0;

  PROVA_MUTE_STDERR;
  PROVA_ASSERT_NULL(stack_top(&s));
  u32 pop_result = stack_pop(&s, &out_val);
  PROVA_UNMUTE_STDERR;

  PROVA_ASSERT_EQUAL(1, pop_result);

  stack_free(&s);
}

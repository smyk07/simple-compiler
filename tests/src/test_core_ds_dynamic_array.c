#include "tests/prova.h"

#include "core/ds/dynamic_array.h"
#include <unistd.h>

#define MUTE_STDERR                                                            \
  int _orig_stderr_fd __attribute__((unused)) = dup(STDERR_FILENO);            \
  do {                                                                         \
    FILE *__err = freopen("/dev/null", "w", stderr);                           \
    (void)__err;                                                               \
  } while (0);

#define UNMUTE_STDERR                                                          \
  do {                                                                         \
    if (_orig_stderr_fd != -1) {                                               \
      fflush(stderr);                                                          \
      dup2(_orig_stderr_fd, STDERR_FILENO);                                    \
      close(_orig_stderr_fd);                                                  \
    }                                                                          \
  } while (0);

typedef struct {
  u32 id;
  char name[20];
} TestItem;

PTEST(init) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(int));

  PROVA_ASSERT_NULL(da.items);
  PROVA_ASSERT_EQUAL(sizeof(int), da.item_size);
  PROVA_ASSERT_EQUAL(0, da.count);
  PROVA_ASSERT_EQUAL(0, da.capacity);
}

PTEST(append) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(u32));

  u32 val1 = 42;
  u32 val2 = 84;

  PROVA_ASSERT_EQUAL(0, dynamic_array_append(&da, &val1));
  PROVA_ASSERT_EQUAL(1, da.count);
  PROVA_ASSERT_GREATER_THAN(0, da.capacity);

  PROVA_ASSERT_EQUAL(0, dynamic_array_append(&da, &val2));
  PROVA_ASSERT_EQUAL(2, da.count);

  u32 out1 = 0, out2 = 0;
  PROVA_ASSERT_EQUAL(0, dynamic_array_get(&da, 0, &out1));
  PROVA_ASSERT_EQUAL(0, dynamic_array_get(&da, 1, &out2));
  PROVA_ASSERT_EQUAL(42, out1);
  PROVA_ASSERT_EQUAL(84, out2);

  dynamic_array_free(&da);
}

PTEST(get_set) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(TestItem));

  TestItem item1 = {1, "Alice"};
  TestItem item2 = {2, "Bob"};

  dynamic_array_append(&da, &item1);
  dynamic_array_append(&da, &item2);

  TestItem *ptr = (TestItem *)dynamic_array_get_ptr(&da, 1);
  PROVA_ASSERT_NOT_NULL(ptr);
  PROVA_ASSERT_EQUAL(2, ptr->id);
  PROVA_ASSERT_EQUAL_STRING("Bob", ptr->name);

  TestItem item3 = {3, "Charlie"};
  PROVA_ASSERT_EQUAL(0, dynamic_array_set(&da, 0, &item3));

  TestItem out;
  dynamic_array_get(&da, 0, &out);
  PROVA_ASSERT_EQUAL(3, out.id);
  PROVA_ASSERT_EQUAL_STRING("Charlie", out.name);

  MUTE_STDERR

  PROVA_ASSERT_EQUAL(1, dynamic_array_set(&da, 5, &item3));
  PROVA_ASSERT_EQUAL(1, dynamic_array_get(&da, 5, &out));

  UNMUTE_STDERR

  dynamic_array_free(&da);
}

PTEST(insert) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(int));

  u32 v1 = 10, v2 = 20, v3 = 30;
  dynamic_array_append(&da, &v1);
  dynamic_array_append(&da, &v2);

  PROVA_ASSERT_EQUAL(0, dynamic_array_insert(&da, 1, &v3));
  PROVA_ASSERT_EQUAL(3, da.count);

  u32 r0, r1, r2;
  dynamic_array_get(&da, 0, &r0);
  dynamic_array_get(&da, 1, &r1);
  dynamic_array_get(&da, 2, &r2);

  PROVA_ASSERT_EQUAL(10, r0);
  PROVA_ASSERT_EQUAL(30, r1);
  PROVA_ASSERT_EQUAL(20, r2);

  MUTE_STDERR

  PROVA_ASSERT_EQUAL(1, dynamic_array_insert(&da, 5, &v3));

  UNMUTE_STDERR

  dynamic_array_free(&da);
}

PTEST(remove) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(int));

  u32 v1 = 10, v2 = 20, v3 = 30;
  dynamic_array_append(&da, &v1);
  dynamic_array_append(&da, &v2);
  dynamic_array_append(&da, &v3);

  PROVA_ASSERT_EQUAL(0, dynamic_array_remove(&da, 1));
  PROVA_ASSERT_EQUAL(2, da.count);

  u32 r0, r1;
  dynamic_array_get(&da, 0, &r0);
  dynamic_array_get(&da, 1, &r1);

  PROVA_ASSERT_EQUAL(10, r0);
  PROVA_ASSERT_EQUAL(30, r1);

  MUTE_STDERR

  PROVA_ASSERT_EQUAL(1, dynamic_array_remove(&da, 5));

  UNMUTE_STDERR

  dynamic_array_free(&da);
}

PTEST(pop) {
  dynamic_array da;
  dynamic_array_init(&da, sizeof(int));

  u32 v1 = 100, v2 = 200;
  dynamic_array_append(&da, &v1);
  dynamic_array_append(&da, &v2);

  u32 pop_val = 0;
  PROVA_ASSERT_EQUAL(0, dynamic_array_pop(&da, &pop_val));
  PROVA_ASSERT_EQUAL(200, pop_val);
  PROVA_ASSERT_EQUAL(1, da.count);

  PROVA_ASSERT_EQUAL(0, dynamic_array_pop(&da, NULL));

  PROVA_ASSERT_EQUAL(0, da.count);

  MUTE_STDERR

  PROVA_ASSERT_EQUAL(1, dynamic_array_pop(&da, &pop_val));

  UNMUTE_STDERR

  dynamic_array_free(&da);
}

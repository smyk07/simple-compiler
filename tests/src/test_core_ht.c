#include "tests/prova.h"

#include "core/ds/ht.h"

typedef struct {
  i32 x;
  i32 y;
} Point;

PTEST(ht_create_destroy) {
  ht *table = ht_create(sizeof(int));

  PROVA_ASSERT_NOT_NULL(table);
  PROVA_ASSERT_GREATER_THAN(0, table->base_capacity);
  PROVA_ASSERT_GREATER_THAN(0, table->capacity);
  PROVA_ASSERT_EQUAL(0, table->count);
  PROVA_ASSERT_EQUAL(sizeof(int), table->value_size);
  PROVA_ASSERT_NOT_NULL(table->items);

  ht_destroy(table);
}

PTEST(ht_init_free) {
  ht table;
  ht_init(&table, sizeof(Point));

  PROVA_ASSERT_GREATER_THAN(0, table.base_capacity);
  PROVA_ASSERT_EQUAL(0, table.count);
  PROVA_ASSERT_EQUAL(sizeof(Point), table.value_size);
  PROVA_ASSERT_NOT_NULL(table.items);

  ht_free(&table);
}

PTEST(ht_insert_search) {
  ht *table = ht_create(sizeof(int));

  int val1 = 100;
  int val2 = 200;

  ht_insert(table, "key_one", &val1);
  ht_insert(table, "key_two", &val2);

  PROVA_ASSERT_EQUAL(2, table->count);

  int *res1 = (int *)ht_search(table, "key_one");
  int *res2 = (int *)ht_search(table, "key_two");

  PROVA_ASSERT_NOT_NULL(res1);
  PROVA_ASSERT_NOT_NULL(res2);
  PROVA_ASSERT_EQUAL(100, *res1);
  PROVA_ASSERT_EQUAL(200, *res2);

  ht_destroy(table);
}

PTEST(ht_update) {
  ht *table = ht_create(sizeof(int));

  int initial_val = 5;
  int updated_val = 99;

  ht_insert(table, "config_param", &initial_val);
  PROVA_ASSERT_EQUAL(1, table->count);

  ht_insert(table, "config_param", &updated_val);
  PROVA_ASSERT_EQUAL(1, table->count);

  int *res = (int *)ht_search(table, "config_param");
  PROVA_ASSERT_NOT_NULL(res);
  PROVA_ASSERT_EQUAL(99, *res);

  ht_destroy(table);
}

PTEST(ht_delete) {
  ht *table = ht_create(sizeof(Point));

  Point p1 = {10, 20};
  Point p2 = {30, 40};

  ht_insert(table, "point_a", &p1);
  ht_insert(table, "point_b", &p2);
  PROVA_ASSERT_EQUAL(2, table->count);

  ht_delete(table, "point_a");
  PROVA_ASSERT_EQUAL(1, table->count);

  PROVA_MUTE_STDERR;
  void *deleted_search = ht_search(table, "point_a");
  PROVA_UNMUTE_STDERR;

  PROVA_ASSERT_NULL(deleted_search);

  Point *remaining = (Point *)ht_search(table, "point_b");
  PROVA_ASSERT_NOT_NULL(remaining);
  PROVA_ASSERT_EQUAL(30, remaining->x);
  PROVA_ASSERT_EQUAL(40, remaining->y);

  ht_destroy(table);
}

PTEST(ht_missing_keys) {
  ht *table = ht_create(sizeof(int));

  int val = 42;
  ht_insert(table, "present_key", &val);

  PROVA_MUTE_STDERR;
  void *missing_search = ht_search(table, "ghost_key");
  ht_delete(table, "ghost_key");
  PROVA_UNMUTE_STDERR;

  PROVA_ASSERT_NULL(missing_search);
  PROVA_ASSERT_EQUAL(1, table->count); // Count remains unchanged

  ht_destroy(table);
}

#include "tests/prova.h"

int main() {
  prova_run_tests(p_registry);
  prova_print_summary(p_registry);
  return (p_metadata.failing_tests + p_metadata.crashing_tests) > 0 ? 1 : 0;
}

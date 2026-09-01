#include <cassert>

#include "test_assert.hpp"

int main() {
  assert(false && "the non-elidable test assertion must terminate with failure");
  return EXIT_SUCCESS;
}

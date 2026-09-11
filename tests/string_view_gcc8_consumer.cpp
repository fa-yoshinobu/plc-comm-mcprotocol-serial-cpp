#include <string_view>
#include <type_traits>
// Reproduce GCC 8's feature-test value on the host compiler.
#undef __cpp_lib_string_view
#define __cpp_lib_string_view 201603L
#include "mcprotocol/serial/string_view_compat.hpp"
#include "test_assert.hpp"
int main() {
  static_assert(std::is_same_v<mcprotocol::serial::StringView, std::string_view>);
  std::string_view value = "D100";
  assert(value.substr(1) == "100");
  value.remove_prefix(1);
  assert(value.find('0') == 1);
}

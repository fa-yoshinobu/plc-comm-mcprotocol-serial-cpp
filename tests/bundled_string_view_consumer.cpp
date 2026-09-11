// Select the fallback before loading the standard header, then prove coexistence.
#define MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT 1
#include "mcprotocol/serial/string_view_compat.hpp"
#undef MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#include <string_view>
#include <type_traits>
#include "test_assert.hpp"

int main() {
  using mcprotocol::serial::StringView;
  static_assert(!std::is_same_v<StringView, std::string_view>);
  constexpr StringView text("D100");
  static_assert(text.size() == 4 && text.front() == 'D');
  static_assert(text.substr(1) == StringView("100"));
  static_assert(text.find('0') == 2);
  static_assert(text.find('X') == StringView::npos);
  static_assert(text.substr(99).empty());
  static_assert(StringView().empty());
  StringView value = text;
  value.remove_prefix(1);
  assert(value == "100");
  assert(value != "101");
  assert(value.end() - value.begin() == 3);
  value.remove_prefix(99);
  assert(value.empty());
  // Standard and fallback types can exist in the same translation unit.
  const std::string_view standard(text.data(), text.size());
  assert(standard == "D100");
}

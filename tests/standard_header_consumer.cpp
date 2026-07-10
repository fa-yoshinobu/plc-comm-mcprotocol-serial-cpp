#include <array>
#include <stdexcept>

#include "mcprotocol_serial.hpp"

namespace {
struct PolymorphicBase {
  virtual ~PolymorphicBase() = default;
};

struct PolymorphicDerived final : PolymorphicBase {};
}  // namespace

int main() {
  // This consumer intentionally exercises members that the old include-root fallback did not
  // provide. On MSVC, <array> must resolve to the toolchain header instead of a library file named
  // "array".
  std::array<int, 2> values {};
  values.fill(7);
  if (values.at(0) != 7 || values.at(1) != 7) {
    return 1;
  }

  // Library-internal size flags must not leak through the CMake target and disable exception or
  // RTTI support in an otherwise normal consumer.
  try {
    throw std::runtime_error("consumer exception support");
  } catch (const std::runtime_error&) {
  }

  PolymorphicDerived derived;
  PolymorphicBase* base = &derived;
  return dynamic_cast<PolymorphicDerived*>(base) != nullptr ? 0 : 2;
}

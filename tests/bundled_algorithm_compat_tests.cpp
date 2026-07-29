#define MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT 1
#include "mcprotocol/serial/compat/algorithm.hpp"

int main() {
  const int input[] = {1, 2, 3, 4};
  int output[] = {0, 0, 0, 0};

  const auto output_end = std::transform(
      input,
      input + 4,
      output,
      [](int value) { return value * 3; });

  if (output_end != output + 4) {
    return 1;
  }
  for (int index = 0; index < 4; ++index) {
    if (output[index] != input[index] * 3) {
      return 2;
    }
  }
  return 0;
}

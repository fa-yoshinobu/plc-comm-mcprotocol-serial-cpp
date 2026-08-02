#pragma once

#include "mcprotocol/serial/compat/cstdint.hpp"
#include "mcprotocol/serial/status.hpp"

namespace mcprotocol::serial::detail {

enum class YieldFirstWaitAction : std::uint8_t {
  Yield,
  SleepOneMillisecond,
};

/// Deterministic wait-phase policy shared by the POSIX and Win32 physical TX drain loops.
class YieldFirstWaitPolicy {
 public:
  [[nodiscard]] YieldFirstWaitAction observe(std::uint64_t pending_bytes) noexcept {
    const bool made_progress = have_previous_ && pending_bytes < previous_pending_;
    const bool should_sleep = !made_progress && previous_wait_was_yield_;
    previous_pending_ = pending_bytes;
    have_previous_ = true;
    previous_wait_was_yield_ = !should_sleep;
    return should_sleep ? YieldFirstWaitAction::SleepOneMillisecond
                        : YieldFirstWaitAction::Yield;
  }

 private:
  std::uint64_t previous_pending_ = 0U;
  bool have_previous_ = false;
  bool previous_wait_was_yield_ = false;
};

/// Deadline-bounded TX-drain loop shared by host transports and deterministic tests.
template <typename RemainingTimeout, typename QueryPending, typename YieldNow, typename SleepOne>
[[nodiscard]] Status drain_tx_with_yield_first(
    std::uint32_t absolute_deadline_ms,
    const char* timeout_message,
    RemainingTimeout& remaining_timeout,
    QueryPending& query_pending,
    YieldNow& yield_now,
    SleepOne& sleep_one) noexcept {
  YieldFirstWaitPolicy wait_policy;
  for (;;) {
    std::uint32_t remaining = remaining_timeout(absolute_deadline_ms);
    if (remaining == 0U) {
      return make_status(StatusCode::Timeout, timeout_message);
    }

    std::uint64_t pending = 0U;
    Status status = query_pending(pending);
    if (!status.ok()) {
      return status;
    }

    // The queue query itself can cross the exact deadline boundary. Timeout wins even when that
    // query reports an empty queue, matching every other transaction boundary in this library.
    remaining = remaining_timeout(absolute_deadline_ms);
    if (remaining == 0U) {
      return make_status(StatusCode::Timeout, timeout_message);
    }
    if (pending == 0U) {
      return ok_status();
    }

    const YieldFirstWaitAction action = wait_policy.observe(pending);
    if (action == YieldFirstWaitAction::SleepOneMillisecond) {
      sleep_one(remaining > 1U ? 1U : remaining);
    } else {
      yield_now();
    }
  }
}

}  // namespace mcprotocol::serial::detail

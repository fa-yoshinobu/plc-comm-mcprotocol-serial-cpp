#include <array>
#include <stdexcept>
#include <type_traits>

#include "mcprotocol_serial.hpp"
#if MCPROTOCOL_SERIAL_ENABLE_HOST_API
#include "mcprotocol/serial/host_serial.hpp"
#endif

namespace {

#if MCPROTOCOL_SERIAL_ENABLE_HOST_API
using mcprotocol::serial::BitValue;
using mcprotocol::serial::HostSerialConfig;
using mcprotocol::serial::HostSerialPort;
using mcprotocol::serial::HostSyncClient;
using mcprotocol::serial::LinkDirectMonitorRegistration;
using mcprotocol::serial::LinkDirectMultiBlockReadRequest;
using mcprotocol::serial::LinkDirectMultiBlockWriteRequest;
using mcprotocol::serial::LinkDirectRandomReadWordItem;
using mcprotocol::serial::LinkDirectRandomWriteBitItem;
using mcprotocol::serial::LinkDirectRandomWriteWordItem;
using mcprotocol::serial::MultiBlockReadBlockResult;
using mcprotocol::serial::MultiBlockReadRequest;
using mcprotocol::serial::MultiBlockWriteRequest;
using mcprotocol::serial::Span;
using mcprotocol::serial::Status;

using ReadRandomLinkDirectWords = Status (HostSyncClient::*)(
    Span<const LinkDirectRandomReadWordItem>, Span<std::uint16_t>) noexcept;
using WriteRandomLinkDirectWords =
    Status (HostSyncClient::*)(Span<const LinkDirectRandomWriteWordItem>) noexcept;
using WriteRandomLinkDirectBits =
    Status (HostSyncClient::*)(Span<const LinkDirectRandomWriteBitItem>) noexcept;
using SelfTestLoopback =
    Status (HostSyncClient::*)(Span<const char>, Span<char>) noexcept;
using ReadBlock = Status (HostSyncClient::*)(
    const MultiBlockReadRequest&,
    Span<std::uint16_t>,
    Span<BitValue>,
    Span<MultiBlockReadBlockResult>) noexcept;
using WriteBlock = Status (HostSyncClient::*)(const MultiBlockWriteRequest&) noexcept;
using ReadLinkDirectBlock = Status (HostSyncClient::*)(
    const LinkDirectMultiBlockReadRequest&,
    Span<std::uint16_t>,
    Span<BitValue>,
    Span<MultiBlockReadBlockResult>) noexcept;
using WriteLinkDirectBlock =
    Status (HostSyncClient::*)(const LinkDirectMultiBlockWriteRequest&) noexcept;
using RegisterLinkDirectMonitorDevices =
    Status (HostSyncClient::*)(const LinkDirectMonitorRegistration&) noexcept;

static_assert(std::is_same_v<
              decltype(static_cast<ReadRandomLinkDirectWords>(
                  &HostSyncClient::read_random_link_direct_words)),
              ReadRandomLinkDirectWords>);
static_assert(std::is_same_v<
              decltype(static_cast<WriteRandomLinkDirectWords>(
                  &HostSyncClient::write_random_link_direct_words)),
              WriteRandomLinkDirectWords>);
static_assert(std::is_same_v<
              decltype(static_cast<WriteRandomLinkDirectBits>(
                  &HostSyncClient::write_random_link_direct_bits)),
              WriteRandomLinkDirectBits>);
static_assert(std::is_same_v<
              decltype(static_cast<SelfTestLoopback>(&HostSyncClient::self_test_loopback)),
              SelfTestLoopback>);
static_assert(std::is_same_v<
              decltype(static_cast<ReadBlock>(&HostSyncClient::read_block)),
              ReadBlock>);
static_assert(std::is_same_v<
              decltype(static_cast<WriteBlock>(&HostSyncClient::write_block)),
              WriteBlock>);
static_assert(std::is_same_v<
              decltype(static_cast<ReadLinkDirectBlock>(
                  &HostSyncClient::read_link_direct_block)),
              ReadLinkDirectBlock>);
static_assert(std::is_same_v<
              decltype(static_cast<WriteLinkDirectBlock>(
                  &HostSyncClient::write_link_direct_block)),
              WriteLinkDirectBlock>);
static_assert(std::is_same_v<
              decltype(static_cast<RegisterLinkDirectMonitorDevices>(
                  &HostSyncClient::register_link_direct_monitor_devices)),
              RegisterLinkDirectMonitorDevices>);

static_assert(!std::is_default_constructible_v<HostSerialConfig>);
static_assert(!std::is_copy_constructible_v<HostSerialPort>);
static_assert(!std::is_copy_constructible_v<HostSyncClient>);
#endif

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

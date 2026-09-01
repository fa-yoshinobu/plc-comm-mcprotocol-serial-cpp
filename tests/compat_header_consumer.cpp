#include <cstdint>
#include <type_traits>

#include "mcprotocol/serial/host_sync.hpp"
#include "mcprotocol/serial/posix_serial.hpp"

namespace {

using namespace mcprotocol::serial;

static_assert(std::is_same_v<PosixSerialConfig, HostSerialConfig>);
static_assert(std::is_same_v<PosixSerialPort, HostSerialPort>);
static_assert(std::is_same_v<PosixSyncClient, HostSyncClient>);
static_assert(std::is_same_v<UserFrameReadRequest, UserFrameRegistrationReadRequest>);
static_assert(std::is_same_v<UserFrameWriteRequest, UserFrameRegistrationWriteRequest>);
static_assert(std::is_same_v<UserFrameDeleteRequest, UserFrameRegistrationDeleteRequest>);

using AsyncQualifiedRead = Status (MelsecSerialClient::*)(
    std::uint32_t,
    const QualifiedBufferWordDevice&,
    std::uint16_t,
    Span<std::uint16_t>,
    CompletionHandler,
    void*) noexcept;
using AsyncQualifiedWrite = Status (MelsecSerialClient::*)(
    std::uint32_t,
    const QualifiedBufferWordDevice&,
    Span<const std::uint16_t>,
    CompletionHandler,
    void*) noexcept;
using AsyncMonitorRegister = Status (MelsecSerialClient::*)(
    std::uint32_t, const MonitorRegistration&, CompletionHandler, void*) noexcept;
using AsyncMonitorCycle = Status (MelsecSerialClient::*)(
    std::uint32_t,
    Span<std::uint16_t>,
    Span<std::uint32_t>,
    CompletionHandler,
    void*) noexcept;
using AsyncUserFrameRead = Status (MelsecSerialClient::*)(
    std::uint32_t,
    const UserFrameRegistrationReadRequest&,
    UserFrameRegistrationData&,
    CompletionHandler,
    void*) noexcept;
using AsyncUserFrameWrite = Status (MelsecSerialClient::*)(
    std::uint32_t,
    const UserFrameRegistrationWriteRequest&,
    CompletionHandler,
    void*) noexcept;
using AsyncUserFrameDelete = Status (MelsecSerialClient::*)(
    std::uint32_t,
    const UserFrameRegistrationDeleteRequest&,
    CompletionHandler,
    void*) noexcept;

using QualifiedRead = Status (HostSyncClient::*)(
    std::string_view, std::uint16_t, Span<std::uint16_t>) noexcept;
using QualifiedWrite =
    Status (HostSyncClient::*)(std::string_view, Span<const std::uint16_t>) noexcept;
using QualifiedBitWrite =
    Status (HostSyncClient::*)(std::string_view, int, bool) noexcept;
using DirectRead = Status (HostSyncClient::*)(
    const ExtendedFileRegisterDirectBatchReadWordsRequest&,
    Span<std::uint16_t>) noexcept;
using DirectWrite = Status (HostSyncClient::*)(
    const ExtendedFileRegisterDirectBatchWriteWordsRequest&) noexcept;
using DirectBitWrite =
    Status (HostSyncClient::*)(std::uint32_t, int, bool) noexcept;
using LongStateReadWithPoints =
    Status (HostSyncClient::*)(std::string_view, std::uint16_t, Span<BitValue>) noexcept;
using LongStateReadFromSpan =
    Status (HostSyncClient::*)(std::string_view, Span<BitValue>) noexcept;
using UserFrameRead = Status (HostSyncClient::*)(
    const UserFrameRegistrationReadRequest&, UserFrameRegistrationData&) noexcept;
using UserFrameWrite =
    Status (HostSyncClient::*)(const UserFrameRegistrationWriteRequest&) noexcept;
using UserFrameDelete =
    Status (HostSyncClient::*)(const UserFrameRegistrationDeleteRequest&) noexcept;
using MonitorRegister = Status (HostSyncClient::*)(
    Span<const highlevel::RandomReadWordSpec>,
    Span<const highlevel::RandomReadDWordSpec>) noexcept;
using MonitorCycle =
    Status (HostSyncClient::*)(Span<std::uint16_t>, Span<std::uint32_t>) noexcept;
using RandomRead = Status (HostSyncClient::*)(
    Span<const highlevel::RandomReadWordSpec>,
    Span<const highlevel::RandomReadDWordSpec>,
    Span<std::uint16_t>,
    Span<std::uint32_t>) noexcept;
using RandomReadWord =
    Status (HostSyncClient::*)(std::string_view, std::uint16_t&) noexcept;
using RandomReadDWord =
    Status (HostSyncClient::*)(std::string_view, std::uint32_t&) noexcept;
using RandomWriteWords =
    Status (HostSyncClient::*)(Span<const highlevel::RandomWriteWordSpec>) noexcept;
using RandomWriteDWords =
    Status (HostSyncClient::*)(Span<const highlevel::RandomWriteDWordSpec>) noexcept;
using RandomWriteExtendedFileRegisterWords = Status (HostSyncClient::*)(
    Span<const ExtendedFileRegisterRandomWriteWordItem>) noexcept;
using RandomWriteWord =
    Status (HostSyncClient::*)(std::string_view, std::uint16_t) noexcept;
using RandomWriteDWord =
    Status (HostSyncClient::*)(std::string_view, std::uint32_t) noexcept;
using RandomWriteBits =
    Status (HostSyncClient::*)(Span<const highlevel::RandomWriteBitSpec>) noexcept;
using RandomWriteBit =
    Status (HostSyncClient::*)(std::string_view, BitValue) noexcept;

#define ASSERT_MEMBER_SIGNATURE(pointer_type, owner_type, member_name) \
  static_assert(std::is_same_v<                                           \
                decltype(static_cast<pointer_type>(&owner_type::member_name)), \
                pointer_type>)

ASSERT_MEMBER_SIGNATURE(
    AsyncQualifiedRead, MelsecSerialClient, async_extended_batch_read_words);
ASSERT_MEMBER_SIGNATURE(
    AsyncQualifiedWrite, MelsecSerialClient, async_extended_batch_write_words);
ASSERT_MEMBER_SIGNATURE(QualifiedRead, HostSyncClient, read_native_qualified_words);
ASSERT_MEMBER_SIGNATURE(QualifiedWrite, HostSyncClient, write_native_qualified_words);
ASSERT_MEMBER_SIGNATURE(
    QualifiedBitWrite, HostSyncClient, write_native_qualified_bit_in_word);

ASSERT_MEMBER_SIGNATURE(
    DirectRead, HostSyncClient, direct_read_extended_file_register_words);
ASSERT_MEMBER_SIGNATURE(
    DirectWrite, HostSyncClient, direct_write_extended_file_register_words);
ASSERT_MEMBER_SIGNATURE(
    DirectBitWrite, HostSyncClient, direct_write_extended_file_register_bit_in_word);

ASSERT_MEMBER_SIGNATURE(LongStateReadWithPoints, HostSyncClient, read_long_state_bits);
ASSERT_MEMBER_SIGNATURE(LongStateReadFromSpan, HostSyncClient, read_long_state_bits);

ASSERT_MEMBER_SIGNATURE(AsyncUserFrameRead, MelsecSerialClient, async_read_user_frame);
ASSERT_MEMBER_SIGNATURE(AsyncUserFrameWrite, MelsecSerialClient, async_write_user_frame);
ASSERT_MEMBER_SIGNATURE(AsyncUserFrameDelete, MelsecSerialClient, async_delete_user_frame);
ASSERT_MEMBER_SIGNATURE(UserFrameRead, HostSyncClient, read_user_frame);
ASSERT_MEMBER_SIGNATURE(UserFrameWrite, HostSyncClient, write_user_frame);
ASSERT_MEMBER_SIGNATURE(UserFrameDelete, HostSyncClient, delete_user_frame);

ASSERT_MEMBER_SIGNATURE(AsyncMonitorRegister, MelsecSerialClient, async_register_monitor);
ASSERT_MEMBER_SIGNATURE(AsyncMonitorCycle, MelsecSerialClient, async_read_monitor);
ASSERT_MEMBER_SIGNATURE(MonitorRegister, HostSyncClient, register_monitor);
ASSERT_MEMBER_SIGNATURE(MonitorCycle, HostSyncClient, read_monitor);

ASSERT_MEMBER_SIGNATURE(RandomRead, HostSyncClient, random_read);
ASSERT_MEMBER_SIGNATURE(RandomReadWord, HostSyncClient, random_read_word);
ASSERT_MEMBER_SIGNATURE(RandomReadDWord, HostSyncClient, random_read_dword);
ASSERT_MEMBER_SIGNATURE(RandomWriteWords, HostSyncClient, random_write_words);
ASSERT_MEMBER_SIGNATURE(RandomWriteDWords, HostSyncClient, random_write_dwords);
ASSERT_MEMBER_SIGNATURE(
    RandomWriteExtendedFileRegisterWords,
    HostSyncClient,
    random_write_extended_file_register_words);
ASSERT_MEMBER_SIGNATURE(RandomWriteWord, HostSyncClient, random_write_word);
ASSERT_MEMBER_SIGNATURE(RandomWriteDWord, HostSyncClient, random_write_dword);
ASSERT_MEMBER_SIGNATURE(RandomWriteBits, HostSyncClient, random_write_bits);
ASSERT_MEMBER_SIGNATURE(RandomWriteBit, HostSyncClient, random_write_bit);

#undef ASSERT_MEMBER_SIGNATURE

}  // namespace

int main() {
  return 0;
}

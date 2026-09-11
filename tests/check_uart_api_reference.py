"""Check the public UART reference against real Doxygen output, including inheritance."""
from pathlib import Path
import shutil
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from generate_api_reference import run_doxygen
from update_api_reference import INPUTS, PREDEFINED

if not shutil.which("doxygen"):
    print("SKIP: Doxygen required")
    sys.exit(77)

compounds = run_doxygen(
    title="UART API regression", root=ROOT,
    inputs=[ROOT / path for path in INPUTS], predefined=list(PREDEFINED),
)
uart = next(c for c in compounds if c.name == "mcprotocol::serial::Esp32UartClient")
expected = {
    "Esp32UartClient", "~Esp32UartClient", "begin", "configure", "recover", "end",
    "busy", "requires_transport_reset", "update", "cancel",
    "async_read_word", "async_read_words", "async_read_bits", "async_write_words", "async_write_bits",
    "read_word", "read_words", "read_bit", "read_bits", "write_word", "write_words", "write_bit", "write_bits",
    "random_read", "async_random_read", "random_write_words", "async_random_write_words",
    "random_write_bits", "async_random_write_bits", "multi_block_read", "async_multi_block_read",
    "multi_block_write", "async_multi_block_write",
}
assert {m.name for m in uart.members} == expected
assert len(uart.members) == len(expected) + 1  # Both read_word overloads; no duplicate overrides.
reads = [m.signature for m in uart.members if m.name == "read_word"]
assert len(reads) == 2
assert any("std::uint16_t &out" in signature for signature in reads)
assert any("std::int16_t &out" in signature for signature in reads)
assert all("Esp32UartClient::" in m.signature and "detail::" not in m.signature for m in uart.members)
assert not any("::detail" in c.name for c in compounds)
config = next(c for c in compounds if c.name == "mcprotocol::serial::Esp32UartConfig")
assert {m.name for m in config.members} == {
    "baud", "format", "rx_pin", "tx_pin", "direction", "rts_pin", "rx_buffer_bytes",
}
namespace = next(c for c in compounds if c.name == "mcprotocol::serial")
direction = next(m for m in namespace.members if m.name == "Esp32Direction")
assert {v.name for v in direction.enum_values} == {"External", "Rs485Rts"}
print("UART API: public methods, overloads, configuration and visibility passed")

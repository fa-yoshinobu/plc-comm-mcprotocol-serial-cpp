#!/usr/bin/env python3
"""Build native and Arduino consumers from the packed PlatformIO tarball."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ENVIRONMENTS = ("native-core", "mega2560-core")
REQUIRED_SOURCES = ("src/client.cpp", "src/codec.cpp")
FORBIDDEN_SOURCE_STEMS = ("host_sync", "posix_serial", "win32_serial")
SHADOW_HEADERS = (
    "include/algorithm",
    "include/array",
    "include/cctype",
    "include/cstddef",
    "include/cstdint",
    "include/cstring",
    "include/type_traits",
)


def platformio_command(explicit: str | None) -> list[str]:
    if explicit:
        executable = Path(explicit).resolve()
        if not executable.is_file():
            raise SystemExit(f"PlatformIO executable does not exist: {executable}")
        return [str(executable)]

    executable = shutil.which("pio") or shutil.which("platformio")
    if executable:
        return [executable]
    return [sys.executable, "-m", "platformio"]


def run(command: list[str], *, cwd: Path) -> None:
    print(f"[platformio-consumer] {' '.join(command)}", flush=True)
    completed = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.stdout:
        print(completed.stdout, end="" if completed.stdout.endswith("\n") else "\n")
    if completed.returncode != 0:
        raise SystemExit(completed.returncode)


def pack_package(pio: list[str], output_dir: Path) -> Path:
    output_dir.mkdir(parents=True)
    run([*pio, "pkg", "pack", str(ROOT), "--output", str(output_dir)], cwd=ROOT)
    packages = sorted(output_dir.glob("mcprotocol-serial-cpp-*.tar.gz"))
    if len(packages) != 1:
        names = ", ".join(package.name for package in packages) or "none"
        raise SystemExit(f"Expected exactly one packed package, found: {names}")
    return packages[0]


def check_archive(package: Path) -> None:
    with tarfile.open(package, "r:gz") as archive:
        names = {member.name.removeprefix("./") for member in archive.getmembers()}
    missing = [source for source in REQUIRED_SOURCES if source not in names]
    if missing:
        raise SystemExit(f"Packed package is missing required sources: {', '.join(missing)}")
    shadowed = [header for header in SHADOW_HEADERS if header in names]
    if shadowed:
        raise SystemExit(
            f"Packed package shadows standard headers from its public include root: {', '.join(shadowed)}"
        )


def write_consumer(consumer: Path, package: Path) -> None:
    (consumer / "src").mkdir(parents=True)
    local_package = consumer.parent / "mcprotocol-serial-cpp.tar.gz"
    shutil.copy2(package, local_package)
    # A relative file dependency works on both POSIX and Windows. PlatformIO currently parses an
    # absolute Windows file:///C:/... archive URI as a POSIX /C:/... path.
    package_uri = "file://../mcprotocol-serial-cpp.tar.gz"
    (consumer / "platformio.ini").write_text(
        f"""[platformio]
default_envs = {', '.join(ENVIRONMENTS)}

[env]
lib_deps =
    {package_uri}
lib_ldf_mode = deep+
build_unflags =
    -std=gnu++11
    -std=gnu++14
    -std=gnu++17
    -std=gnu++2a
    -std=gnu++20
build_flags =
    -std=gnu++17

[env:native-core]
platform = native

[env:mega2560-core]
platform = atmelavr
board = megaatmega2560
framework = arduino
""",
        encoding="utf-8",
        newline="\n",
    )
    (consumer / "src" / "main.cpp").write_text(
        r'''#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <array>
#endif

#include "mcprotocol_serial.hpp"

#if !defined(PLATFORMIO)
#error "This smoke consumer must be built by PlatformIO"
#endif

#if MCPROTOCOL_SERIAL_ENABLE_HOST_API
#error "The core-only PlatformIO package must not expose the unbuilt host facade"
#endif

int exercise_core_api() {
  auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(
      mcprotocol::serial::PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});

  mcprotocol::serial::MelsecSerialClient client;
  const mcprotocol::serial::Status status = client.configure(protocol);
  if (!status.ok()) {
    return 1;
  }

#if !defined(ARDUINO)
  // A package must not shadow the host toolchain's complete standard <array> header.
  std::array<int, 2> values {};
  values.fill(7);
  if (values.at(0) != 7 || values.at(1) != 7) {
    return 2;
  }
#endif
  return 0;
}

#if defined(ARDUINO)
volatile int smoke_result = -1;

void setup() {
  smoke_result = exercise_core_api();
}

void loop() {}
#else
int main() {
  return exercise_core_api();
}
#endif
''',
        encoding="utf-8",
        newline="\n",
    )


def check_objects(consumer: Path, environment: str) -> None:
    build_dir = consumer / ".pio" / "build" / environment
    object_names = {path.name for path in build_dir.rglob("*.o")}
    missing = []
    for source in REQUIRED_SOURCES:
        source_name = Path(source).name
        candidates = {Path(source).stem + ".o", source_name + ".o"}
        if object_names.isdisjoint(candidates):
            missing.append(source_name)
    if missing:
        raise SystemExit(
            f"{environment} did not compile required package objects: {', '.join(missing)}"
        )
    unexpected = [
        stem
        for stem in FORBIDDEN_SOURCE_STEMS
        if not object_names.isdisjoint({stem + ".o", stem + ".cpp.o"})
    ]
    if unexpected:
        raise SystemExit(
            f"{environment} compiled host-only package objects: {', '.join(unexpected)}"
        )
    print(
        f"[platformio-consumer] {environment}: linked client.cpp and codec.cpp; "
        "host-only objects absent"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, help="existing PlatformIO .tar.gz to verify")
    parser.add_argument("--pio", help="path to the PlatformIO executable")
    parser.add_argument(
        "--environment",
        action="append",
        choices=ENVIRONMENTS,
        dest="environments",
        help="environment to build; repeat to select more than one (default: both)",
    )
    args = parser.parse_args()

    pio = platformio_command(args.pio)
    environments = tuple(args.environments or ENVIRONMENTS)

    with tempfile.TemporaryDirectory(prefix="mcprotocol-platformio-consumer-") as temp:
        temp_dir = Path(temp)
        if args.package:
            package = args.package.resolve()
            if not package.is_file():
                raise SystemExit(f"Packed package does not exist: {package}")
        else:
            package = pack_package(pio, temp_dir / "package")

        check_archive(package)
        consumer = temp_dir / "consumer"
        write_consumer(consumer, package)
        for environment in environments:
            run([*pio, "run", "-e", environment], cwd=consumer)
            check_objects(consumer, environment)

    print("[platformio-consumer] packed-package consumer checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())

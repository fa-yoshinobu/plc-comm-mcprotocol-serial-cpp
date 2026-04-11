#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "usage: $0 <doxyfile>" >&2
  exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
bundled_bin="${repo_root}/.tools/doxygen/usr/bin/doxygen"
bundled_lib="${repo_root}/.tools/doxygen/usr/lib/x86_64-linux-gnu"

if [ -x "${bundled_bin}" ]; then
  export LD_LIBRARY_PATH="${bundled_lib}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
  exec "${bundled_bin}" "$1"
fi

if command -v doxygen >/dev/null 2>&1; then
  exec doxygen "$1"
fi

echo "Doxygen not found." >&2
echo "Install it system-wide or place a local bundle under .tools/doxygen/." >&2
exit 1

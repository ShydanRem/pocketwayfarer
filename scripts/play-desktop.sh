#!/usr/bin/env bash
# Interactive native UI, with the same rules and generated data as the ESP32 build.
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v cmake >/dev/null && [ -x /workspace/.pocketwayfarer-tools/python/bin/cmake ]; then
  export PATH="/workspace/.pocketwayfarer-tools/python/bin:$PATH"
fi
python3 scripts/desktop-deps.py
cmake -S firmware/host -B build/host/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/host/cmake --target play_lantern -j4
exec build/host/cmake/play_lantern "$@"

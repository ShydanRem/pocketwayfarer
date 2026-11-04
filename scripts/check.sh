#!/usr/bin/env bash
# Host checks: art up to date, core tests (sanitizers), real UI previews.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -d firmware/.deps/lvgl ] || python3 scripts/firmware-deps.py
python3 scripts/build_art.py --check
OUT=build/host
mkdir -p "$OUT"
cc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -Ifirmware/core \
   firmware/core/*.c firmware/host/test_core.c -o "$OUT/test_core"
"$OUT/test_core"
cmake -S firmware/host -B "$OUT/cmake" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$OUT/cmake" --target preview -j4 > /dev/null
rm -rf "$OUT/shots" && mkdir -p "$OUT/shots"
"$OUT/cmake/preview" "$OUT/shots" > /dev/null
python3 scripts/previews.py "$OUT/shots" docs/img

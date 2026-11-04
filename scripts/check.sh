#!/usr/bin/env bash
# Host checks for both games: generated art up to date, core tests with
# sanitizers (RPG includes the balance simulation), real-UI previews.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -d firmware/.deps/lvgl ] || python3 scripts/firmware-deps.py
python3 scripts/build_art.py --check
python3 scripts/build_rpg_art.py --check
OUT=build/host
mkdir -p "$OUT"
SAN="-std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -Ifirmware/common"
cc $SAN -Ifirmware/rpg/core firmware/rpg/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/rpg/host/test_rpg.c -o "$OUT/test_rpg"
"$OUT/test_rpg"
cc $SAN -Ifirmware/pet/core firmware/pet/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/pet/host/test_core.c -o "$OUT/test_pet"
"$OUT/test_pet" | tail -1
cmake -S firmware/host -B "$OUT/cmake" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$OUT/cmake" --target preview_rpg preview_pet -j4 > /dev/null
for game in rpg pet; do
  rm -rf "$OUT/shots-$game" && mkdir -p "$OUT/shots-$game"
  "$OUT/cmake/preview_$game" "$OUT/shots-$game" > /dev/null
  python3 scripts/previews.py "$OUT/shots-$game" "docs/img/$game"
done

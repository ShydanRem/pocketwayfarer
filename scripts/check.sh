#!/usr/bin/env bash
# Host checks for both games: generated art up to date, core tests with
# sanitizers (RPG includes the balance simulation), real-UI previews.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -d firmware/.deps/lvgl ] || python3 scripts/firmware-deps.py
python3 scripts/build_art.py --check
python3 scripts/build_rpg_art.py --check
python3 scripts/build_lantern_art.py --check
python3 scripts/build_class_art.py --check
python3 scripts/build_hero_rig.py --check
python3 scripts/class_inventory.py --check
OUT=build/host
PREVIEW_ROOT=${POCKET_PREVIEW_ROOT:-docs/img}
mkdir -p "$OUT"
SAN="-std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -Ifirmware/common"
cc $SAN firmware/host/test_device_input.c firmware/common/device_input.c -o "$OUT/test_device_input"
"$OUT/test_device_input"
cc $SAN -Ifirmware/rpg/core firmware/rpg/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/rpg/host/test_rpg.c -o "$OUT/test_rpg"
"$OUT/test_rpg"
cc $SAN -Ifirmware/rpg/core firmware/rpg/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/rpg/host/test_lantern.c -o "$OUT/test_lantern"
"$OUT/test_lantern"
cc $SAN -UNDEBUG -Ifirmware/rpg/core firmware/rpg/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/rpg/host/test_reliability.c -o "$OUT/test_reliability"
"$OUT/test_reliability"
cc $SAN -Ifirmware/pet/core firmware/pet/core/*.c firmware/common/rng.c firmware/common/timing.c \
   firmware/pet/host/test_core.c -o "$OUT/test_pet"
"$OUT/test_pet" | tail -1
cmake -S firmware/host -B "$OUT/cmake" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$OUT/cmake" --target preview_rpg preview_pet preview_lantern preview_classes preview_hero_rig test_device_ui test_hero_cache test_hero_rig -j4 > /dev/null
"$OUT/cmake/test_hero_cache"
"$OUT/cmake/test_hero_rig" --full
mkdir -p "$OUT/device-regressions"
"$OUT/cmake/test_device_ui" "$OUT/device-regressions"
for game in rpg pet lantern classes hero_rig; do
  rm -rf "$OUT/shots-$game" && mkdir -p "$OUT/shots-$game"
  "$OUT/cmake/preview_$game" "$OUT/shots-$game" > /dev/null
  python3 scripts/previews.py "$OUT/shots-$game" "$PREVIEW_ROOT/$game"
done

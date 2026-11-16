# Verification evidence — 2026-11-16

Current implementation: [LANTERNBOUND.md](../LANTERNBOUND.md).

- `model.txt`: 432 deterministic expeditions, 38,075 save/resume snapshots and 37,639 rule transitions; all seven enemies encountered.
- `reliability.txt`: regressions for the defects reproduced in the baseline audit, plus v2/v3 migration and starting blueprint/bestiary persistence.
- `regression.txt`: complete `scripts/check.sh` output (generated assets, sanitizer core tests, native previews).
- `ui-sanitizers.txt`: all 45 native LVGL captures and 306 validated saves, with UI/LVGL AddressSanitizer and UBSan; leak scan disabled because the final screen intentionally stays alive. Includes repeated classic PWR/countdown checks, coaching/FX/reduced-motion paths, pause-edge input exclusion, and exact incremental/full-frame comparisons.
- `firmware-build.txt`, `firmware.json`: successful ESP-IDF 5.5.1 build, exact app bytes/hash, save v4 format and explicit untested-device flags.
- `desktop.txt`, `desktop-build.txt`, `desktop-smoke.txt`: observed native interactions and final headless SDL2 launch/save/close smoke. No browser port involved.

These logs verify the implemented paths. They do not measure physical-device performance, battery life, session duration, player retention or enjoyment. Historical baseline audit evidence stays under `docs/audit/evidence/`.

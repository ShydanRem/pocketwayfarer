# Verification evidence — 2026-11-16–17

Current implementation: [LANTERNBOUND.md](../LANTERNBOUND.md).

- `model.txt`: 600 deterministic expeditions, 52,393 save/resume snapshots and 51,509 rule transitions; all seven enemies encountered. Includes all 20 full relic loadouts and both swap/skip policies, plus 24 no-relic wins.
- `reliability.txt`: regressions for the defects reproduced in the baseline audit, plus v2/v3 migration and starting blueprint/bestiary persistence.
- `regression.txt`: complete `scripts/check.sh` output (generated assets, sanitizer core tests, native previews).
- `ui-sanitizers.txt`: all 49 native LVGL captures and 430 validated saves, with UI/LVGL AddressSanitizer and UBSan; leak scan disabled because the final screen intentionally stays alive. Includes classic PWR/countdown checks, coaching/FX/reduced-motion paths, pause-edge input exclusion, incremental/full-frame comparisons, IT/EN relic exchanges and a complete no-relic win without duplicate rewards.
- `firmware-build.txt`, `firmware.json`: successful ESP-IDF 5.5.1 build, exact app bytes/hash, save v4 format and explicit untested-device flags.
- `desktop.txt`, `desktop-build.txt`, `desktop-smoke.txt`: observed native interactions and final headless SDL2 launch/save/close smoke. No browser port involved.
- `relic-desktop.txt`: the relic preview's SDL dummy launch/save check and the blocked manual desktop attempt on November 17; it does not claim a manual interaction pass.

These logs verify the implemented paths. They do not measure physical-device performance, battery life, session duration, player retention or enjoyment. Historical baseline audit evidence stays under `docs/audit/evidence/`.

# Verification evidence — 2026-11-16–17

These logs describe the 0.4.0 clarity increment; current R1 atlas evidence is under [roadmap/evidence](../roadmap/evidence/).

Current implementation: [LANTERNBOUND.md](../LANTERNBOUND.md). New corrections: [device audit](../DEVICE-AUDIT-2026-11-17.md) and [art clarity research](../ART-CLARITY.md). Firmware metadata describes the local ESP-IDF build; CI builds its own exact downloadable package from the published commit.

- `model.txt`: 600 deterministic expeditions, 52,393 save/resume snapshots and 51,509 rule transitions; all seven enemies encountered. Includes all 20 full relic loadouts and both swap/skip policies, plus 24 no-relic wins.
- `reliability.txt`: regressions for the defects reproduced in the baseline audit, plus v2/v3 migration and starting blueprint/bestiary persistence.
- `regression.txt`: complete `scripts/check.sh` output (generated assets, sanitizer core tests, native previews).
- `ui-sanitizers.txt`: all 49 native LVGL captures and 432 validated saves, with UI/LVGL AddressSanitizer and UBSan; leak scan disabled because the final screen intentionally stays alive. Includes classic PWR/countdown checks, coaching/FX/reduced-motion paths, pause-edge input exclusion, incremental/full-frame comparisons, IT/EN relic exchanges and a complete no-relic win without duplicate rewards.
- `firmware-build.txt`, `firmware.json`: successful ESP-IDF 5.5.1 build, exact app bytes/hash, save v4 format and explicit untested-device flags.
- `device-ui-sanitizers.txt`: three native timing captures, 120ms frame regression, press-edge timing, stale-input exclusion, slow/failed save, wake readiness, 220 stable image descriptors, toast lifetime, 44 item dialogs and 42 gem species. Portable input sampling is also tested with sanitizers in `regression.txt`.
- `desktop.txt`, `desktop-build.txt`, `desktop-smoke.txt`: historical observed native interactions and headless SDL2 launch/save/close smoke from the earlier preview; not a new manual pass of 0.4.0-preview.1. No browser port involved.
- `clarity-desktop-build.txt`, `clarity-desktop-smoke.txt`: current 0.4.0 native SDL2 compilation and clean headless launch/save/close; no manual interaction claim.
- `relic-desktop.txt`: the relic preview's SDL dummy launch/save check and the blocked manual desktop attempt on November 17; it does not claim a manual interaction pass.

These logs verify the implemented paths. They do not measure physical-device performance, battery life, session duration, player retention or enjoyment. Historical baseline audit evidence stays under `docs/audit/evidence/`.

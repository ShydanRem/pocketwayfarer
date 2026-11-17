# Pocket Wayfarer development contract

Read `docs/ROADMAP.md` and `docs/CLASS-ART-BIBLE.md` before expanding classes,
art or progression. Follow milestone dependencies; keep the current active
batch and evidence in `docs/roadmap/STATUS.md`. Do not mark a milestone done
from generated source art or a design-tool score alone.

The 40 classic classes and three Lanternbound loadouts are distinct systems.
Class illustrations are representatives, not replacements for the saved
customizable avatar. Preserve class IDs, progression, cosmetics, visible
equipment channels, input fixes and save v4 unless a later scoped migration
is explicitly implemented and tested.

Review art in actual 466×466 LVGL captures. Maintain silhouettes, quiet actor
areas, visible feet, distinct touch regions and IT/EN readability. Record
generation source, prompts and import hashes. Missing art must stay explicit.

Run checks appropriate to the changed subsystem and the release workflow.
Host regressions do not certify physical latency, battery, frame rate or
hardware stability. Physical testing gates stable release, not unrelated
offline art production. Do not add persistent arrays or new class families
without first reviewing save/schema and equipment-mask constraints.

# Modular player rig — all forty classic classes

Original component sheets generated with OpenAI ImageGen on 2026-11-17 (bases) and 2026-11-18 (R3.1). The
unaltered RGBA sources and exact prompts are retained under `source/`. This is
generated artwork, not hand-drawn production or extracted commercial game art.
Use is recorded under the generation tool's applicable terms; no independent
stock-library license is asserted. The project takes direction from illustrated
pixel RPGs without including their characters, sprites or trademarks as assets.

| File | Generation identity | Layout |
|---|---|---|
| `outfits.png` | `exec-7fa961ed-3196-4435-bb9d-2b4ef7f2a087.png` | 1774 × 887; 4 columns × 2 rows, headless costumes |
| `heads.png` | `exec-1967ed7e-d830-4a19-aa32-dc382ff98aef.png` | 887 × 1774; 4 expressions × 8 race/face rows |
| `hair.png` | `exec-029b93b6-4fec-4e39-a1c8-87cf2c812533.png` | 1536 × 1024; 3 columns × 2 rows |
| `gear.png` | `exec-d34e40af-3545-4232-8158-694d6c41a30e.png` | 1254 × 1254; 4 columns × 4 rows |
| `hats.png` | `exec-f4f9811d-6ac5-494c-8e2b-15c50c3270ec.png` | 1774 × 887; 4 columns × 2 rows |
| `advanced-a-outfits.png` | `exec-cf2f6ff0-215e-4707-ad48-e6d7bc4ea9b9.png` | 1774 × 887; 4 × 2 costumes, IDs 8–15 |
| `advanced-a-hats.png` | `exec-7607961a-bc90-4e06-8222-7557c6171fa1.png` | 1774 × 887; 4 × 2 headwear, IDs 8–15 |
| `advanced-b-outfits.png` | `exec-382386fd-5a1b-46eb-8193-b46382e491ea.png` | 1774 × 887; 4 × 2 costumes, IDs 16–23 |
| `advanced-b-hats.png` | `exec-6466ddd8-1a85-4142-8cf7-6ef7a29d62e3.png` | 1774 × 887; 4 × 2 headwear, IDs 16–23 |
| `master-a-outfits.png` | `exec-748732ce-2318-4395-b3ea-aea551554c8b.png` | 1774 × 887; 4 × 2 costumes, IDs 24–31 |
| `master-a-hats.png` | `exec-229f6d10-a5f4-454f-9dae-194654200c30.png` | 1774 × 887; 4 × 2 headwear, IDs 24–31 |
| `master-b-outfits.png` | `exec-322cfc8d-0583-482f-bb2e-bd5a3f4626f5.png` | 1774 × 887; 4 × 2 costumes, IDs 32–39 |
| `master-b-hats.png` | `exec-9ac7ab5a-eff0-47e6-b03c-361f22552453.png` | 1774 × 887; 4 × 2 headwear, IDs 32–39 |

Costume/default-hat order is exact base class ID: Warrior, Mage, Archer, Rogue,
Cleric, Monk, Artificer, Occultist. Heads use Human, Elf, Dwarf, Beastkin, each
with standard/long-lash variants; columns are idle, blink, hurt, happy. Hair is
short, spiky, long, ponytail, bob, braids. Eye, skin and hair colors come from
the saved cosmetic channels. Expressions and component poses are independent.

Gear follows the **actual `IT_*` enum**, not legacy offhand-relative indices:
sword, axe, staff, bow, dagger, mace, knuckles, hammer, scythe, shield, quiver,
tome, charm, helmet, hood, circlet. The five owned tiers tint metal and leather
channels. The knuckle pair is separated mechanically into left/right mounts.
Class hats are defaults; equipped headgear replaces them. Torso, feet and the
two accessory slots still affect game statistics without separate visual
layers. They are not claimed as completed visual channels.

`scripts/build_hero_rig.py` performs deterministic import only: transparent cell
validation, registration from technical magenta neck/grip markers, one nearest
neighbor affine resample, alpha threshold 128, cropped indexed RGB565 palettes
(112 quantization colors plus transparent entry) and semantic palette roles.
The 88 × 88 rig uses neck `(44, 32)` and nominal feet row 84; dwarf compression
preserves the feet while moving the neck. Head and hair scales are shared.
The hat sheet's uneven transparent gutters are measured in `source/layout.json`;
the source is not cropped across opaque pixels. Local peg removal is restricted
to the registration marker so purple fabric/gems remain intact.

Palette roles: original, skin, iris, hair/fur, tier metal, tier leather/wood.
Magenta skin on the source costumes is a technical recolor channel, not a
visible runtime color. The Beastkin tail reuses the existing legacy component;
the rest of the visible rig and gear comes from these new sheets. Hair is
drawn behind the head and again above its front cutoff to preserve long styles.

Idle, breath, attack, guard and hit use **parametric component offsets** on the
registered costumes. These are not five separately authored full-body animation
strips. Feet stay fixed; quivers follow the torso rather than the shield hand.
Reduced motion forces the idle pose. Animation polish remains R7.

The scythe is composited after clothing and before the head/hair so its crescent
cannot hide the player's expression during attack. Other held items keep their
foreground order. This is a renderer occlusion rule; the source PNG is intact.

```sh
python3 scripts/build_hero_rig.py
python3 scripts/build_hero_rig.py --check
```

`manifest.json` records hashes, transforms, hand mounts and canvas geometry.
Source conversion alone is not acceptance: see the native-renderer tests and
[R2 evidence](../../docs/roadmap/rig-evidence/README.md). All forty classic class IDs are now admitted. The defensive renderer for inconsistent looks remains, but no valid classic class ID needs a family/tier fallback. This rig does not change save v4, class mechanics or Lanternbound's
three separate loadouts.

R3.1 appends Knight, Berserker, Elementalist, Arcanist, Ranger, Sniper, Assassin and Duelist in exact ID order 8–15. Generated `HERO_RIG_CLASS_COUNT`, family and zero-based tier metadata validate each look before admission. The common heads, hair and owned gear stay shared; no save array is enlarged. [R3.1 evidence](../../docs/roadmap/advanced-a-evidence/README.md) records silhouette comparisons under identical helmets, native views and actual touch promotions.

## R3.2 — IDs 16–23 and fixed decorative color

The third sheets append Paladin, Priest, Champion, Mystic, Blacksmith, Alchemist, Necromancer and Warlock, generated on 2026-11-18. The common cosmetic and gear layers remain shared. No save array, family or class rule is added. [R3.2 evidence](../../docs/roadmap/advanced-b-evidence/README.md) records the 24-class matrix and all 16 advanced touch promotions in both languages.

The Alchemist's purple flask glass shared a quantized RGB565 color with technical magenta hand shadows. Seven waist pixels incorrectly received the skin role. `source/layout.json` now marks the logical half-open flask-belt rectangle `[36,45,55,64]` as fixed decoration. Palette identity is `(RGB565, semantic_role)`, so equal colors in hands and glass can retain different behavior. This changes role/index metadata only; no PNG pixel is painted or removed. The test changes all five skin tones and checks 1,444 waist pixels; the previous palette fails 28 checks.

The generation briefs use the descriptive names Smith/Sorcerer for IDs 20/23; canonical UI names are Blacksmith/Warlock from the existing core table. Exact prompts remain unaltered.

## R4.1 — first master IDs 24–31

Dragon Knight, Warlord, Archmage, Chronomancer, Beastmaster, Stormbow, Nightblade and Blademaster were generated on 2026-11-18. Exact `master-a-outfits-prompt.txt` and `master-a-hats-prompt.txt` are retained. Each uses the corresponding approved advanced-A sheet as registration/world reference. Original PNGs are retained unaltered; imports use the same neck/hand/foot registration and semantic palettes. No local painting, missing-part reconstruction or commercial artwork is included.

At R4.1 coverage was 32 exact costumes/default hats, with master family metadata 0–3 and zero-based tier2. The last eight masters still used legacy fallbacks. The 245,760 cosmetic combinations and 1,200 sibling checks covered that baseline; the Alchemist regression stays active. [R4.1 evidence](../../docs/roadmap/master-a-evidence/README.md) records its native scenes, promotion paths, sanitizers and limits. These are still preliminary parametric poses, not final animation strips.

Native QA found that rectangular attack-arm motion sliced the continuous capes of IDs24/30. `layout.json` now records `rigid_arm_classes` for those two only, emitted as `rig_rigid_arms`. Their attack/guard/hit translate the connected costume coherently above logical row78, with matching held-item grips and planted soles. No PNG pixels are reconstructed or painted; this deliberately simpler provisional pose avoids a visual tear until R7 introduces actual limb/cape masks.

The master generation briefs use descriptive names Beast Lord/Storm Archer/Night Blade/Blade Master for IDs 28–31; canonical English UI names remain Beastmaster/Stormbow/Nightblade/Blademaster from the core table. Exact prompts are retained unaltered.

## R4.2 — final master IDs 32–39

Templar, Hierophant, Grandmaster, Ascendant, Runesmith, Transmuter, Deathlord and Voidcaller were generated with ImageGen on 2026-11-18. The exact `master-b-outfits-prompt.txt` and `master-b-hats-prompt.txt` remain under `source/`, together with the unaltered RGBA PNGs. Each used the corresponding advanced-B sheet as the registration/world reference. The importer performs only the recorded mechanical registration, alpha threshold and palette conversion.

Coverage is now **40 exact costumes/default hats**. All 307,200 cosmetic combinations and 1,600 sibling comparisons are exercised; class IDs, owned equipment, save v4 and inherited skill rules remain unchanged. The three visible owned equipment channels stay weapon/offhand/head; this does not add torso/feet/accessory graphics.

R7.2a uses the existing Paladin and Dragon Knight pixels. The runtime applies a calibrated mesh in `firmware/rpg/ui/hero_rig.c` over the registered costume, hands and neck. Weapons with a short swing rotate about their existing grip, while the cape follows the body deformation. Foot pixels use the original sampler. The [source manifest](manifest.json) covers the original atlases, prompts and registration. [Native captures and checks](../../docs/roadmap/classic-combat-evidence/README.md)

Native QA identified rectangular arm cuts in robes33/35/38/39. `rigid_arm_classes` now covers24/30/33/35/38/39 only: connected fabric and grips lean together while soles remain fixed after race compression. Other new costumes retain their existing arm motion. New pixel regressions extend the old cape checks; this deliberately simpler provisional pose does not claim final articulated animation. [R4.2 evidence](../../docs/roadmap/master-b-evidence/README.md) records the final matrix, native review and limits.

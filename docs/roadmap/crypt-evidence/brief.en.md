# Crypt of Lights design brief

Extend the existing original Pocket Wayfarer turn-based RPG for handheld players with one Classic Crypt of Lights navigation and art pass. Platform: native 466×466 circular physical touch display, LVGL, ESP32-S3. Its existing game has 10 floors, 4 rows of route choices and 3 columns. Preserve generation, species IDs, rewards, and save v4.
Goal: choose rooms, guard supplies and reach the bosses on floors 5 and 10.
Core mechanics:
1. Touch a highlighted room on the next connected row to enter. Ordinary room entry spends 2 stamina. One touch resolves a trap, fountain, chest or merchant, or starts a battle. Locked rooms stay inert. Future rooms are obscured. Reachable, current, unavailable and visited states have distinct border/shape cues.
2. Touch Attack, then tap the gold timing window, or press BOOT. Tap or press BOOT when the parry wave reaches the shield. On victory, Continue returns to the route while keeping the earned loot.
3. Touch Supplies to use a held potion or ether. Touch Exit, then Leave to spend one Return Scroll and go to town. Staying closes the dialog. After the floor 5 boss a new descent may start at floor 6.
4. On a cleared stair the player chooses Descend or Town. A cold restart during a fight restores the saved choice of room and reproduces the encounter.
Win condition: Defeat the Bone King on floor 10 to unlock the next zone. Lose condition: At 0 HP the descent ends with a game over dialog, the hero returns to the inn, and half the descent gold is lost. One life per descent. Restart a new descent after healing at the inn.
Scoring & progression: Progression: 10 floors. The HUD shows floor N/10 and descent gold. Scoring consists of existing XP, job XP, gold and items awarded for encounters; preserve thresholds.
Screens: Existing start menu, zone entry, floor graph HUD, battle HUD, reward and game over dialogs, merchant and stair/exit dialogs. The device can pause in sleep and resume.
Design and sound: Violet basalt and funerary teal glass, quiet central floor, amber accessible route, vivid Mimic and Bone King silhouettes. The Classic preview is silent.
How it should feel: Input feedback: 1 released tap on a reachable room causes 1 transition; 0 transitions for locked rooms. All active controls fit the 466px circle and stay separate from the next or previous row.
Build ONLY scope: native UI, one crypt scene, two monster sprites and acceptance checks.
Done when: touch input visits a valid room and ignores a locked one, a fight and its reward survive a restart with exact same outcome semantics, win and game over remain reachable, floor 6 works, and the graph renders inside a round 466x466 viewport in Italian and English.

# More PC boxes: plan

## Where the space goes

Emerald's flash is 32 sectors of 4 KB, and all of them are in use:

| Sectors | Use |
|---------|-----|
| 0-13 | Save slot 1 (SaveBlock2: 1, SaveBlock1: 4, PC storage: 9) |
| 14-27 | Save slot 2 (the backup: saves alternate between the slots) |
| 28-29 | Hall of Fame (50 teams) |
| 30 | e-Reader Trainer Hill challenge |
| 31 | Recorded battle (Battle Frontier) |

The PC storage (`struct PokemonStorage`, 33,744 bytes) gets 9 × 3,968 = 35,712 bytes, so 1,968 bytes are free.
One box needs 2,410 bytes (30 × 80 for the Pokémon, plus its name and wallpaper). **Not even one more box
fits** without a tenth storage sector in *both* save slots.

## Options

1. **Keep 14 boxes (recommended).** 14 × 30 = 420 slots, which already holds a full Gen 3 living dex
   (386) with 34 to spare. Platinum went from 18 to 21 boxes for its 493 Pokémon; Emerald doesn't need it.
   No save-format risk.
2. **16 boxes.** Give each slot a fifteenth sector (sectors 0-14 and 15-29) and move the Hall of Fame to
   sector 30:
   - The Hall of Fame shrinks from 50 teams to 33 (one sector). The oldest teams fall off.
   - The e-Reader Trainer Hill challenge can't be saved (it needs e-Reader cards, so most players never see
     it). The built-in Trainer Hill is unaffected.
   - Recorded battles stay.
   - Old saves can't load: the sector layout changes. Fine before the user's playthrough, not after.
   - Touches `save.c` (layout, slot size, the special-sector checks), `hall_of_fame.c` (team count),
     `ereader_helpers.c`, `pokemon_storage_system.h`, plus every place that assumes 14 boxes (box
     selection menu, PokéNav condition list size, `TOTAL_BOXES_COUNT` users).
3. **Dropping the backup slot** for more room: rejected. It's what makes a crash during a save survivable.

## Decision

Waiting on the user. Until then the change list marks this one "Plan".

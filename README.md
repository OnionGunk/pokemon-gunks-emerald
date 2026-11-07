# Pokémon Gunk's Emerald Version

A quality-of-life hack of Pokémon Emerald, built on [pret's Emerald decompilation](https://github.com/pret/pokeemerald).
It keeps the game's difficulty and story as they are and takes the grind and friction out, for long playthroughs.

> **Status: in development.** No release yet. The first release comes once the hack is playable start to
> finish. Progress on each change is tracked in [`docs/GUNK_CHANGES.md`](docs/GUNK_CHANGES.md).

## Features

- **DexNav.** A key item Birch gives you with the Pokédex (open it from the Bag, the START menu or SELECT
  once registered).
  - The screen shows the area's Pokémon and how many you've caught. Caught ones are in colour with a Poké
    Ball mark, seen ones a silhouette, unseen ones "?". The info panel shows status, where it's found, the
    area's level range, search level, chain, types and abilities.
  - Pick one with **R**, then press **R** in the grass or while surfing and sneak up on it.
  - The search bar shows its level, nature, ability, a SHINY mark and one star per perfect IV.
  - Every DexNav Pokémon is an ordinary, legal wild roll: search levels, chains and stars give extra rolls for
    good IVs and shinies instead of editing the Pokémon.
  - The chain grows when you catch or defeat the Pokémon, and resets when you run, take too long or meet
    another species. The START menu and the Bag pause a search.
  - It doesn't search in the Safari Zone (Safari Pokémon take Safari Balls).
- **Info:** a growing berry tree says how many hours until it bears fruit; on Route 119 an empty cast says how
  many steps the nearest Feebas spot is; during a swarm the DexNav's title bar says which Pokémon and where.
- **Pokémon follow you**, with emotes and messages, for all 386 species.
- **16 PC boxes** (14 in the original game).
- **Faster play:** instant text (INST. after FAST in Options), a faster EXP bar, auto-run (hold B to walk),
  eggs hatch in half the steps. New games start on FAST text.
- **Quick Run and Quick Ball:** in battle, **R** runs; in wild battles **L** throws your ball (hold L and
  press Left/Right to pick another one).
- **Items:** reusable TMs, a second registered item on **L**, **Start** sorts the bag, a prompt to use
  another Repel when one runs out, and two key items Mom gives you with the Running Shoes: the
  **Endless Repel** and the **Exp. All** (USE turns them on and off).
- **Pokémon:** **A** on the summary's Skills page shows IVs, then EVs. Vitamins work up to 252 EVs. Trade
  evolutions happen without trading: at Lv. 36, or by using the trade item like an evolution stone (the
  Everstone still stops them). Field moves work without teaching the move if a Pokémon could learn it from
  a TM or HM in your bag (badges still needed), and every PC has a free **Move Tutor**.

Battle mechanics, trainers, wild Pokémon and the story are Emerald's own. Save safety comes first: every build
passes an emulator test suite that covers save cycles, crashes during a save and damaged-save recovery, and
the same suite runs on unmodified Emerald as the control.

Why pret's decompilation and not pokeemerald-expansion: [`docs/ENGINE.md`](docs/ENGINE.md).

## Play it

Releases ship a **BPS patch**, not a ROM. You need your own dump of **Pokémon Emerald (USA)**:

```
sha1: f3ae088181bf583e55daf962a92bb46f4f1d07b7
```

1. Download the `.bps` file from [Releases](../../releases).
2. Apply it to your ROM with [Rom Patcher JS](https://www.marcrobledo.com/RomPatcher.js/) (in the browser),
   Floating IPS, or `py tools/patch/bps.py apply <emerald.gba> <patch.bps> <out.gba>`.
3. Play the result in any GBA emulator or flashcart. The patcher checks your ROM first and refuses the wrong one.

Saves carry over between releases unless the release notes say otherwise. Back up your save before updating.

## Build it

Follow pret's [`INSTALL.md`](INSTALL.md), then run `make`. A clean build reproduces the release ROM, and
`tools/patch/bps.py make` makes the patch from it.

## Credits

- [pret](https://github.com/pret/pokeemerald) for the Emerald decompilation this hack is built on, and for
  [agbcc](https://github.com/pret/agbcc).
- **Following Pokémon:** [aarant's pokeemerald followers](https://github.com/aarant/pokeemerald)
  (`followers-legacy` branch), with its sprites, emotes and messages.
- **DexNav:** [ghoulslash's pokeemerald DexNav](https://github.com/ghoulslash/pokeemerald) (`dexnav` branch)
  and its contributors; changed here to give only legal Gen 3 Pokémon.
- The DexNav idea comes from Pokémon Omega Ruby and Alpha Sapphire and fan hacks such as Pokémon Unbound.

Pokémon is © Nintendo, Creatures Inc. and GAME FREAK inc. This is a non-commercial fan project and contains
no ROM data; you need your own legally dumped copy of the game.

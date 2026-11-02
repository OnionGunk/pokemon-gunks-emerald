# Pokémon Gunk's Emerald Version

A quality-of-life hack of Pokémon Emerald, built on [pret's Emerald decompilation](https://github.com/pret/pokeemerald).
It keeps the game's difficulty and story as they are and takes the grind and friction out, for long playthroughs.

> **Status: in development.** No release yet. The first release comes once the hack is playable start to
> finish. Progress on each change is tracked in [`docs/GUNK_CHANGES.md`](docs/GUNK_CHANGES.md).

## Features (planned)

- **DexNav.** See which Pokémon live on the route, then search for one. Searches roll legal wild encounters,
  and search levels and chains raise the odds of good IVs and shinies.
- **Pokémon follow you.**
- **More PC boxes** than the original 14.
- **Faster play:** instant text, a faster EXP bar, auto-run, faster egg hatching.
- **Items:** reusable TMs, two registered items, bag sorting, a prompt to use another Repel when one runs out,
  and an Endless Repel toggle.
- **Pokémon:** IV and EV view in the summary, Exp. All, vitamins up to 252 EVs, and trade evolutions without
  trading. HM moves work without teaching the move, and every Pokémon Center has a Move Tutor.

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
- The DexNav idea comes from later Pokémon games and fan hacks such as Pokémon Unbound.
- Further credits are added here as features are ported.

Pokémon is © Nintendo, Creatures Inc. and GAME FREAK inc. This is a non-commercial fan project and contains
no ROM data; you need your own legally dumped copy of the game.

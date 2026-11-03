# Engine decision: pret `pokeemerald` + agbcc

Gunk's Emerald is built on [pret/pokeemerald](https://github.com/pret/pokeemerald), compiled with pret's
[agbcc](https://github.com/pret/agbcc), the same old GCC the retail game was built with.

The working branch is `gunk-followers`: Gunk's commits on top of aarant's `followers-legacy` branch, which is
pret from February 2026 plus following Pokémon, then ghoulslash's DexNav merged in. (The first Gunk's
commits were made on pret master `731ad5bfd`, branch `gunk`; pret has since moved graphics to a new build
pipeline that the follower branch predates, so the commits were moved rather than converting ~800 sprites.)

## Why not `pokeemerald-expansion`

rh-hideout's expansion already has followers, a DexNav, more PC boxes and many QoL switches, which would save
work. It was rejected because Gunk's hacks keep the original game's difficulty and feel:

- **Battle mechanics change by design.** The expansion replaces the battle engine with a Gen 9 one: new moves,
  abilities, items, the physical/special split, and updated species stats. Its `GEN_3` config switches roll back
  many formulas but not all of them, and the species, move and AI data are still the new data.
- **Vanilla can't be checked.** The expansion can't reproduce the retail ROM, so there's no control build to
  prove a test failure is ours and not the engine's. pret's tree builds the retail ROM byte for byte (sha1
  `f3ae088181bf583e55daf962a92bb46f4f1d07b7`), so every save test also runs on unmodified Emerald.
- **Save layout.** The expansion moves things around in the save blocks. Staying on pret's layout keeps Gunk's
  save changes small and reviewable, and save safety comes first.

The features the expansion would have given us are ported from smaller, self-contained pokeemerald sources
instead (see `docs/GUNK_CHANGES.md` and the README credits).

## Why agbcc and not `make modern`

- agbcc reproduces retail exactly, so the vanilla build is the test suite's control and `git diff` is the whole
  story of what Gunk's changes.
- Retail quirks that some code depends on (UB that `modern` builds expose) stay as they were.
- The patch is smaller: unchanged code stays at the same addresses.

`modern` (arm-none-eabi-gcc 16 from MSYS2 UCRT64) remains a fallback if a feature ever needs it.

## Toolchain (Windows)

- MSYS2 at `C:\msys64`: host `gcc`/`make` in `usr`, `arm-none-eabi` binutils in `ucrt64`.
- agbcc built from `C:\Projects\emerald-tools\agbcc` and installed into `tools/agbcc` with its `install.sh`.
- Shell setup: `C:\Projects\emerald-tools\env.sh` (PATH, and a TMP without the apostrophe in the user's profile
  path, which gcc can't handle).
- Build: `MSYSTEM=MSYS C:\msys64\usr\bin\bash.exe -l`, `source /c/Projects/emerald-tools/env.sh`, `make -j`.
  `make compare` checks the vanilla build against `rom.sha1`.

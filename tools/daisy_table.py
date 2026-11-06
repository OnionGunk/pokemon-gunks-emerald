"""Print a Markdown table of RAM symbols a companion app (PokeDaisy) reads: the vanilla address, the hack's, and
whether it moved. Addresses come from the two GNU ld maps.

    py -3 tools/daisy_table.py <vanilla.map> <hack.map>
"""
import re
import sys

SYMBOLS = [
    "gSaveBlock1Ptr", "gSaveBlock2Ptr", "gPokemonStoragePtr", "gSaveBlock1", "gSaveBlock2", "gPokemonStorage",
    "gPlayerParty", "gPlayerPartyCount", "gEnemyParty", "gEnemyPartyCount", "gMain", "gBattleTypeFlags",
    "gBattleMons", "gBattleOutcome", "gBattlersCount", "gActiveBattler", "gTasks", "gSprites", "gObjectEvents",
    "gPlayerAvatar", "gMapHeader", "gSpecialVar_Result", "gRngValue", "gStringVar1", "gStringVar4",
    "gBattleResults", "gWeatherPtr", "gPaletteFade", "gTrainerBattleOpponent_A", "gBattleTerrain",
]


def symbols(path):
    out = {}
    pat = re.compile(r"^\s+0x([0-9a-f]{8,16})\s+(\w+)(?: = \.)?\s*$")  # IWRAM symbols read "name = ."
    for line in open(path, encoding="utf-8", errors="replace"):
        m = pat.match(line)
        if m:
            out.setdefault(m.group(2), int(m.group(1), 16))
    return out


def main():
    vanilla, hack = symbols(sys.argv[1]), symbols(sys.argv[2])
    print("| Symbol | Vanilla | Gunk's | |")
    print("|---|---|---|---|")
    for name in SYMBOLS:
        v, h = vanilla.get(name), hack.get(name)
        if v is None and h is None:
            continue
        fmt = lambda a: f"0x{a:08X}" if a is not None else "—"
        note = "same" if v == h else "**moved**"
        print(f"| {name} | {fmt(v)} | {fmt(h)} | {note} |")


if __name__ == "__main__":
    main()

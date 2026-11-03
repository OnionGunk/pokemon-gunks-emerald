"""Title screen art as one editable picture, and back.

    py -3 tools/title_art.py export <title_logo.png>    the logo and the banner as they sit on screen (240x160)
    py -3 tools/title_art.py import <title_logo.png>    split an edited picture back into the game's graphics

On screen the title art is two pieces:
- the logo: background 2, 256 color, from graphics/title_screen/pokemon_logo.png, drawn with its top-left
  corner at screen (29, 0). Its tilemap is the identity (tile n at position n), so the picture is the map.
- the banner ("EMERALD VERSION"): two 64x32 sprites from graphics/title_screen/emerald_version.png, with its
  top-left corner at screen (66, 50), drawn over the logo, using one 16-color palette.

import splits by area; where the two overlap (rows 50-63), a pixel goes to the banner if its color is one the
banner uses below the logo, else to the logo, so the screen looks the same either way (see SPEC_TEXT).
Needs Pillow.
"""
import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GFX = os.path.join(ROOT, "graphics", "title_screen")
LOGO_PNG, LOGO_PAL = os.path.join(GFX, "pokemon_logo.png"), os.path.join(GFX, "pokemon_logo.pal")
BANNER_PNG = os.path.join(GFX, "emerald_version.png")

SCREEN = (240, 160)
LOGO_POS, LOGO_SIZE = (29, 0), (256, 64)
BANNER_POS, BANNER_SIZE = (66, 50), (128, 32)
LOGO_COLORS = 224        # palette banks 0-13; bank 14 is Rayquaza's and the clouds'
BANNER_COLORS = 16       # one sprite palette

SPEC_TEXT = """Gunk's Emerald title art: title_logo.png
========================================

What it is: the title screen's logo exactly as the game shows it, on a 240x160 canvas (the GBA screen,
1:1 pixels). Transparent pixels show the title screen's background (Rayquaza and the green clouds).
Edit it and send the same file back; tools/title_art.py splits it into the game's two graphics.

Size: keep 240x160. Draw with 1-pixel precision; no scaling, no anti-aliasing to transparency
(a pixel is either fully opaque or fully transparent; half-transparent pixels count as transparent).

The two areas (x, y of the top-left corner, then width x height, in canvas pixels):
1. BANNER  x=66..193, y=50..81  (128 x 32). This was "EMERALD VERSION".
   - At most 15 colors plus transparency (one 16-color sprite palette).
   - It is drawn on top of the logo. Rows y=50..63 are shared with the logo: there, a pixel belongs to the
     banner if its color is one the banner also uses in rows 64..81, and to the logo otherwise. The screen
     looks the same either way; it only matters for the colour counts below.
   - In game it slides down into place from the top of the screen and fades in.
2. LOGO    the rest of x=29..239, y=0..63 (the logo is 256 x 64 starting at x=29; the screen cuts it
   at x=239). This was "POKeMON".
   - At most 223 colors plus transparency, shared by the whole logo.
   - The 8x8 square at x=29..36, y=0..7 must stay fully transparent (the game uses it as its empty tile).
   - In game a white shine sweeps across it, which lights up opaque logo pixels.
Nothing outside those two areas can be shown: pixels at y >= 82, or at x < 29 outside the banner, are ignored.

Colors: the GBA has 15-bit color (5 bits per channel). Colors are rounded to that, so very close shades merge.
The background behind the logo is dark green with Rayquaza in black; the original uses white and light
outlines so it reads on that.

The rest of the title screen (PRESS START, the copyright line, Rayquaza, the clouds) isn't part of this picture.
"""


def to_gba(rgb):
    return tuple((c >> 3) << 3 for c in rgb)


def write_jasc(path, colors, count):
    colors = list(colors) + [(0, 0, 0)] * (count - len(colors))
    with open(path, "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n%d\n" % count)
        for r, g, b in colors:
            f.write("%d %d %d\n" % (r, g, b))


def export(out):
    canvas = Image.new("RGBA", SCREEN, (0, 0, 0, 0))
    for path, pos in ((LOGO_PNG, LOGO_POS), (BANNER_PNG, BANNER_POS)):
        im = Image.open(path)
        idx = im.load()
        rgba = im.convert("RGBA")
        px = rgba.load()
        for y in range(im.size[1]):
            for x in range(im.size[0]):
                sx, sy = x + pos[0], y + pos[1]
                if idx[x, y] != 0 and 0 <= sx < SCREEN[0] and 0 <= sy < SCREEN[1]:
                    canvas.putpixel((sx, sy), to_gba(px[x, y][:3]) + (255,))  # the colors the GBA shows
    canvas.save(out)


def indexed(pixels, size, max_colors, what):
    """pixels: {(x, y): rgb}. Returns a P-mode image with index 0 transparent."""
    colors = sorted(set(pixels.values()))
    if len(colors) > max_colors - 1:
        sys.exit(f"{what}: {len(colors)} colors after rounding to GBA color; the limit is {max_colors - 1}")
    index = {c: i + 1 for i, c in enumerate(colors)}
    im = Image.new("P", size, 0)
    im.putpalette([0, 0, 0] + [v for c in colors for v in c] + [0, 0, 0] * (255 - len(colors)))
    for (x, y), c in pixels.items():
        im.putpixel((x, y), index[c])
    return im, [(0, 0, 0)] + colors


def import_(path):
    art = Image.open(path).convert("RGBA")
    if art.size != SCREEN:
        sys.exit(f"{path}: {art.size[0]}x{art.size[1]}, expected 240x160")
    px = art.load()
    logo, banner = {}, {}
    bx0, by0 = BANNER_POS
    lx0, ly0 = LOGO_POS
    overlap_end = ly0 + LOGO_SIZE[1]  # rows by0..overlap_end-1 of the banner rectangle are over the logo

    def opaque(x, y):
        r, g, b, a = px[x, y]
        return to_gba((r, g, b)) if a >= 128 else None

    def in_banner(x, y):
        return bx0 <= x < bx0 + BANNER_SIZE[0] and by0 <= y < by0 + BANNER_SIZE[1]

    # the banner's colors: those it uses below the logo
    banner_colors = {opaque(x, y) for y in range(overlap_end, by0 + BANNER_SIZE[1])
                     for x in range(bx0, bx0 + BANNER_SIZE[0])} - {None}
    for y in range(SCREEN[1]):
        for x in range(SCREEN[0]):
            c = opaque(x, y)
            if c is None:
                continue
            if in_banner(x, y) and (y >= overlap_end or c in banner_colors):
                banner[(x - bx0, y - by0)] = c
            elif lx0 <= x < lx0 + LOGO_SIZE[0] and ly0 <= y < ly0 + LOGO_SIZE[1]:
                if x - lx0 < 8 and y - ly0 < 8:
                    sys.exit(f"pixel ({x}, {y}) is in the logo's first tile, which must stay transparent")
                logo[(x - lx0, y - ly0)] = c
    logo_im, logo_colors = indexed(logo, LOGO_SIZE, LOGO_COLORS, "logo")
    banner_im, banner_colors = indexed(banner, BANNER_SIZE, BANNER_COLORS, "banner")
    logo_im.save(LOGO_PNG)
    write_jasc(LOGO_PAL, logo_colors, 256)
    banner_im.save(BANNER_PNG)
    print(f"logo: {len(logo_colors) - 1} colors, banner: {len(banner_colors) - 1} colors; rebuild with make")


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("export", "import", "spec"):
        sys.exit(__doc__)
    if sys.argv[1] == "export":
        export(sys.argv[2])
    elif sys.argv[1] == "spec":
        open(sys.argv[2], "w", encoding="utf-8").write(SPEC_TEXT)
    else:
        import_(sys.argv[2])


if __name__ == "__main__":
    main()

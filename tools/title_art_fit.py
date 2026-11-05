"""Fit a large title logo picture (on a plain black background) into title_art.py's 240x160 canvas.

    py -3 tools/title_art_fit.py <logo.png> <out_title_logo.png>

The black around the art (flood-filled from the corners) becomes transparent. The art is scaled uniformly so it
fills the 82 rows the title screen can show (y 0..81) and centered. Parts that start in the lower part of the art
(the version text) are quantized to the banner's 15 colors, the rest (the POKeMON logo) to the logo's colors.
Then run: py -3 tools/title_art.py import <out_title_logo.png>
Needs Pillow, numpy and scipy.
"""
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

ROWS = 82                # y 0..81: the logo's 64 rows and the banner's last rows
BANNER_X = (66, 194)     # the banner's columns; the version text has to fit inside
LOGO_COLORS, BANNER_COLORS = 200, 15


def background(rgb):
    """True where a pixel is part of the dark area connected to the corners."""
    dark = rgb.max(axis=2) < 12  # the background is 0..4; the text outline is about 30
    lab, _ = ndimage.label(dark)
    corners = {lab[0, 0], lab[0, -1], lab[-1, 0], lab[-1, -1]} - {0}
    return np.isin(lab, list(corners))


def quantize(rgb, mask, colors):
    """rgb rounded to GBA colors, the masked pixels quantized to at most `colors` colors."""
    out = rgb.copy()
    pts = rgb[mask]
    if len(pts) == 0:
        return out
    strip = Image.fromarray(pts.reshape(1, -1, 3).astype(np.uint8))
    q = np.array(strip.quantize(colors, method=Image.Quantize.MEDIANCUT).convert("RGB")).reshape(-1, 3)
    out[mask] = (q >> 3) << 3
    return out


def main():
    src, out = sys.argv[1:3]
    rgb = np.array(Image.open(src).convert("RGB")).astype(np.int32)
    fg = ~background(rgb)
    lab, _ = ndimage.label(fg)
    # the biggest part is the POKeMON logo; parts starting below its middle and left of its right edge are text
    sizes = np.bincount(lab.ravel())
    sizes[0] = 0
    logo_id = sizes.argmax()
    ly = np.where((lab == logo_id).any(axis=1))[0]
    lx = np.where((lab == logo_id).any(axis=0))[0]
    text = np.zeros_like(fg)
    for i, sl in enumerate(ndimage.find_objects(lab), 1):
        if i != logo_id and sl[0].start > (ly[0] + ly[-1]) // 2 and sl[1].stop < lx[-1] - (lx[-1] - lx[0]) // 10:
            text |= lab == i
    ys, xs = np.where(fg)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    scale = ROWS / (y1 - y0)
    w = round((x1 - x0) * scale)
    left = (240 - w) // 2

    def shrink(a):
        im = Image.fromarray(a[y0:y1, x0:x1].astype(np.float32))
        return np.array(im.resize((w, ROWS), Image.Resampling.BOX))

    alpha = shrink(fg.astype(np.float32))
    text_cov = shrink(text.astype(np.float32))
    small = np.stack([shrink(rgb[..., c] * fg) for c in range(3)], axis=2)
    opaque = alpha >= 0.5
    small = np.clip(np.round(small / np.maximum(alpha, 1e-6)[..., None]), 0, 255).astype(np.int32)
    is_text = opaque & (text_cov >= alpha / 2)
    tx = np.where(is_text.any(axis=0))[0] + left
    if len(tx) and (tx.min() < BANNER_X[0] or tx.max() >= BANNER_X[1]):
        sys.exit(f"the version text spans x {tx.min()}..{tx.max()}, outside the banner's {BANNER_X}")
    small = quantize(small, is_text, BANNER_COLORS)
    small = quantize(small, opaque & ~is_text, LOGO_COLORS)
    canvas = np.zeros((160, 240, 4), np.uint8)
    canvas[:ROWS, left:left + w, :3] = small
    canvas[:ROWS, left:left + w, 3] = opaque * 255
    Image.fromarray(canvas, "RGBA").save(out)
    print(f"{w}x{ROWS} at x={left}; text x {tx.min()}..{tx.max()}")


if __name__ == "__main__":
    main()

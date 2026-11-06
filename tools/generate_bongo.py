#!/usr/bin/env python3
"""Build crisp Bongo Cat frames for the 118x118 visible LCD area, and the pixels of each key.

The art comes from the bongo.cat dark layers plus a drawn 65% keyboard. Each layer
is reduced to palette colors with a few coverage levels, which keeps a light
anti-aliasing on the edges without the softness of a plain downscale.

usage: generate_bongo.py <bongo.cat checkout> <output dir>

"""
import json
import re
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

SRC, OUT = sys.argv[1:3]
FRAME = (800, 450)
CROP = (312, 4, 690, 300)
WIDTH = 118
HEIGHT = round(WIDTH * (CROP[3] - CROP[1]) / (CROP[2] - CROP[0]))
SCALE = WIDTH / (CROP[2] - CROP[0])
ALPHA_LOW = 50
ALPHA_HIGH = 190
ALPHA_LEVELS = 3
SS = 4
CHROMA_CUTOFF = 40
WHITE_CUTOFF = 128
TINT_CUTOFF = 110

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
PINK = (255, 150, 160)
RED = (220, 60, 70)

# Zoom65 v3 ANSI layout in key units, None marks the gap before the arrow cluster.
# The top right position is the screen module, not a key
ROWS = (
    (1,) * 13 + (2, 1),
    (1.5,) + (1,) * 12 + (1.5, 1),
    (1.75,) + (1,) * 11 + (2.25, 1),
    (2.25,) + (1,) * 10 + (1.75, 1, 1),
    (1.25, 1.25, 1.25, 6.25, 1.25, 1.25, None, 1, 1, 1),
)
MODULE = (0, 15)
BOARD_UNITS = (16, 5)
BOARD_BEZEL = 0.3
BOARD_THICKNESS = 4
BOARD_ORIGIN = (352, 146)
BOARD_U = (322 / 16, 76 / 16)
BOARD_V = (-34 / 5, 58 / 5)
# Down paws reach the board, which sits clear of the cat, so each arm stretches from where it joins the body
PAW_LEFT_REACH = 25
PAW_RIGHT_STRETCH = 1.28
# Eye boxes on the panel, kept as open images for blinking
EYE_RIGHT_BOX = (40, 30, 46, 36)
EYE_LEFT_BOX = (71, 40, 77, 45)


def layer(name, column):
    image = Image.open(f"{SRC}/images/{name}.png").convert("RGBA")
    x, y = column * FRAME[0], FRAME[1]
    return image.crop((x, y, x + FRAME[0], y + FRAME[1])).crop(CROP)


def stretched(part, factor):
    box = part.getbbox()
    piece = part.crop(box)
    piece = piece.resize((piece.width, round(piece.height * factor)), Image.LANCZOS)
    out = Image.new("RGBA", part.size, (0, 0, 0, 0))
    out.paste(piece, box[:2])
    return out


def reaching(part, pixels):
    box = part.getbbox()
    return stretched(part, (box[3] - box[1] + pixels) / (box[3] - box[1]))


def nearest(color):
    """Only strongly tinted pixels become pink or red, so outline edges stay white."""
    r, g, b = color
    if r - g < CHROMA_CUTOFF:
        return WHITE if max(color) >= WHITE_CUTOFF else BLACK
    return PINK if g >= TINT_CUTOFF else RED


def coverage(alpha):
    """Steepen the edge ramp and limit it to a few levels."""
    ramp = min(max((alpha - ALPHA_LOW) / (ALPHA_HIGH - ALPHA_LOW), 0), 1)
    return round(round(ramp * ALPHA_LEVELS) * 255 / ALPHA_LEVELS)


def snap(part):
    """Area-average to panel size, then force each pixel to a palette color and coverage level."""
    r, g, b, a = part.split()
    premultiplied = Image.merge("RGBA", (ImageChops.multiply(r, a), ImageChops.multiply(g, a), ImageChops.multiply(b, a), a))
    small = premultiplied.resize((WIDTH, HEIGHT), Image.BOX)
    out = Image.new("RGBA", small.size, (0, 0, 0, 0))
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b, a = small.getpixel((x, y))
            level = coverage(a)
            if level:
                out.putpixel((x, y), nearest((r * 255 // a, g * 255 // a, b * 255 // a)) + (level,))
    return out


def board_point(u, v):
    # The board faces the cat, so the space row is at the back and the layout turns 180 degrees
    u, v = BOARD_UNITS[0] - u, BOARD_UNITS[1] - v
    x = BOARD_ORIGIN[0] + u * BOARD_U[0] + v * BOARD_V[0] - CROP[0]
    y = BOARD_ORIGIN[1] + u * BOARD_U[1] + v * BOARD_V[1] - CROP[1]
    return x * SCALE * SS, y * SCALE * SS


def keys():
    """Yield each drawn key's corners, and whether it is the screen module."""
    for v, row in enumerate(ROWS):
        u = 0
        for width in row:
            if width is None:
                u += 0.5
                continue
            corners = [board_point(u, v), board_point(u + width, v), board_point(u + width, v + 1), board_point(u, v + 1)]
            yield corners, (v, u) == MODULE
            u += width


def keyboard():
    image = Image.new("RGBA", (WIDTH * SS, HEIGHT * SS), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    line = dict(fill=WHITE + (255,), width=SS, joint="curve")
    b = BOARD_BEZEL
    w, h = BOARD_UNITS
    outline = [board_point(-b, -b), board_point(w + b, -b), board_point(w + b, h + b), board_point(-b, h + b)]
    front_right, front_left, _, back_right = outline
    for a, b in ((front_left, front_right), (front_right, back_right)):
        face = [a, b, (b[0], b[1] + BOARD_THICKNESS * SS), (a[0], a[1] + BOARD_THICKNESS * SS)]
        draw.polygon(face, fill=BLACK + (255,))
        draw.line(face + face[:1], **line)
    draw.polygon(outline, fill=BLACK + (255,))
    for corners, module in keys():
        draw.polygon(corners, fill=(WHITE if module else BLACK) + (255,))
        draw.line(corners + corners[:1], **line)
    draw.line(outline + outline[:1], **line)
    return snap(image)


def without_tint(part):
    """Clears the impact lines the source art draws on down paws."""
    out = part.copy()
    for y in range(out.height):
        for x in range(out.width):
            if out.getpixel((x, y))[:3] not in (BLACK, WHITE):
                out.putpixel((x, y), (0, 0, 0, 0))
    return out


BODY = [snap(layer("cat", 0)), snap(layer("mouth", 0))]
PAWS = {
    "left": (snap(layer("paw-left", 0)), without_tint(snap(reaching(layer("paw-left", 1), PAW_LEFT_REACH)))),
    "right": (snap(layer("paw-right", 0)), without_tint(snap(stretched(layer("paw-right", 1), PAW_RIGHT_STRETCH)))),
}


def frame(left_down, right_down):
    image = Image.new("RGBA", (WIDTH, HEIGHT), BLACK + (255,))
    for part in BODY:
        image.alpha_composite(part)
    image.alpha_composite(keyboard())
    image.alpha_composite(PAWS["left"][left_down])
    image.alpha_composite(PAWS["right"][right_down])
    return image.convert("RGB")


def changed_box(a, b):
    return ImageChops.difference(a, b).getbbox()


def overlap(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


idle = frame(0, 0)
left = frame(1, 0)
right = frame(0, 1)
left_box = changed_box(idle, left)
right_box = changed_box(idle, right)
if overlap(left_box, right_box):
    sys.exit(f"paw patches overlap: {left_box} {right_box}")

base = idle.copy()
base.paste(BLACK, left_box)
base.paste(BLACK, right_box)
base.save(f"{OUT}/bongo_base.png")
idle.save(f"{OUT}/bongo_idle.png")
idle.crop(left_box).save(f"{OUT}/bongo_left_up.png")
left.crop(left_box).save(f"{OUT}/bongo_left_down.png")
idle.crop(right_box).save(f"{OUT}/bongo_right_up.png")
right.crop(right_box).save(f"{OUT}/bongo_right_down.png")
idle.crop(EYE_RIGHT_BOX).save(f"{OUT}/bongo_eye_right.png")
idle.crop(EYE_LEFT_BOX).save(f"{OUT}/bongo_eye_left.png")

print(f"#define BONGO_HEIGHT {idle.height}")
print(f"#define BONGO_LEFT_X {left_box[0]}\n#define BONGO_LEFT_Y {left_box[1]}")
print(f"#define BONGO_RIGHT_X {right_box[0]}\n#define BONGO_RIGHT_Y {right_box[1]}")


def matrix_keys():
    """Reads the physical position of each matrix key from the firmware's table."""
    source = (Path(__file__).parent.parent / "src" / "keyvis.h").read_text()
    table = source[source.index("key_rects[KEY_ROWS][KEY_COLS]") :]
    for row, block in enumerate(re.findall(r"\[\d\] = \{(.*?)\n    \},", table, re.S)):
        for col, (x, width) in enumerate(re.findall(r"\{(\d+), (\d+)\}", block)):
            yield row, col, int(x) / 4, int(width) / 4


def key_runs(row, x, width):
    """Vertical runs of the pixels inside a key, in LCD stream order."""
    corners = [board_point(x, row), board_point(x + width, row), board_point(x + width, row + 1), board_point(x, row + 1)]
    mask = Image.new("1", board.size)
    ImageDraw.Draw(mask).polygon([(px / SS, py / SS) for px, py in corners], fill=1)
    runs = []
    for px in reversed(range(board.width)):
        ys = [py for py in range(board.height) if mask.getpixel((px, py)) and board.getpixel((px, py)) == BLACK + (255,)]
        start = None
        for i, py in enumerate(ys):
            if start is None:
                start = py
            if i + 1 == len(ys) or ys[i + 1] != py + 1:
                runs.append((px, start, py - start + 1))
                start = None
    return runs


def visible_bits(paw, box):
    """One bit per patch pixel, in rows: set where this paw does not cover the keyboard."""
    return [[paw.getpixel((x, y))[3] == 0 for x in range(box[0], box[2])] for y in range(box[1], box[3])]


board = keyboard()


key_table = {}
for row, col, x, width in matrix_keys():
    if not width or (row, x) == MODULE:
        continue
    key_table[f"{row},{col}"] = key_runs(row, x, width)

Path(f"{OUT}/cat_keys.json").write_text(json.dumps({
    "keys": key_table,
    "left_up": {"box": left_box, "bits": visible_bits(PAWS["left"][0], left_box)},
    "left_down": {"box": left_box, "bits": visible_bits(PAWS["left"][1], left_box)},
    "right_up": {"box": right_box, "bits": visible_bits(PAWS["right"][0], right_box)},
    "right_down": {"box": right_box, "bits": visible_bits(PAWS["right"][1], right_box)},
}))

#!/usr/bin/env python3
"""Runs tools/sim/screenshots.txt through the simulator and saves PNGs and a contact sheet.

usage: screenshots.py <sim> <internal flash dump> <external flash dump> <output dir>
"""
import subprocess
import sys
from pathlib import Path

from PIL import Image

SCALE = 4
GAP = 8
COLUMNS = 5

sim, internal, external, out = sys.argv[1:5]
out = Path(out)
out.mkdir(parents=True, exist_ok=True)

script = Path(__file__).parent / "sim" / "screenshots.txt"
with script.open() as commands:
    subprocess.run([sim, internal, external, str(out)], stdin=commands, check=True)

names = [line.split()[1] for line in script.read_text().splitlines() if line.startswith("shot ")]
shots = []
for raw in (out / f"{name}.ppm" for name in names):
    image = Image.open(raw).convert("RGB")
    image = image.resize((image.width * SCALE, image.height * SCALE), Image.NEAREST)
    image.save(raw.with_suffix(".png"))
    raw.unlink()
    shots.append(image)

side = shots[0].width
rows = (len(shots) + COLUMNS - 1) // COLUMNS
sheet = Image.new("RGB", (COLUMNS * (side + GAP) + GAP, rows * (side + GAP) + GAP), (40, 40, 40))
for i, shot in enumerate(shots):
    sheet.paste(shot, (GAP + (i % COLUMNS) * (side + GAP), GAP + (i // COLUMNS) * (side + GAP)))
sheet.save(out / "sheet.png")
print(f"{len(shots)} screenshots in {out}")

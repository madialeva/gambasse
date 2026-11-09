#!/usr/bin/env python3
"""Generates resources/img/logo.ico, the icon embedded in Gambasse.exe, from logo.svg.

Run it by hand when the logo changes, and commit the result: the build only reads the
.ico, so it needs neither of the tools of this script, rsvg-convert and Pillow.

The sizes are the ones Windows asks for at 100-200 % scale. Each one is rendered from
the SVG instead of scaling a single image, and all of them are 32-bit bitmaps except
256 px, a PNG: the layout of Microsoft's icon guidelines.
"""
import io
import struct
import subprocess
from pathlib import Path

from PIL import Image

SIZES = (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
IMAGES = Path(__file__).resolve().parent.parent / "resources" / "img"


def render(size):
    """The logo at <size> px, as a PNG."""
    command = ["rsvg-convert", "-w", str(size), "-h", str(size), str(IMAGES / "logo.svg")]
    return subprocess.run(command, check=True, stdout=subprocess.PIPE).stdout


def bitmap(png, size):
    """The icon bitmap of a PNG: a header, the BGRA pixels and the 1-bit mask of the
    transparent ones, both from the bottom row up, with the mask rows padded to 32 bits.
    The header's height counts both, as in every icon."""
    image = Image.open(io.BytesIO(png)).convert("RGBA").transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    pixels = image.tobytes("raw", "BGRA")
    transparent = image.getchannel("A").point(lambda alpha: 255 if alpha == 0 else 0).convert("1")
    mask_row = (size + 7) // 8
    padding = bytes(-mask_row % 4)
    bits = transparent.tobytes("raw", "1")
    mask = b"".join(bits[row : row + mask_row] + padding for row in range(0, len(bits), mask_row))
    header = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, len(pixels) + len(mask), 0, 0, 0, 0)
    return header + pixels + mask


def main():
    images = [(size, render(size) if size == 256 else bitmap(render(size), size)) for size in SIZES]

    icon = struct.pack("<HHH", 0, 1, len(images))
    offset = len(icon) + 16 * len(images)
    for size, data in images:
        # A width and a height of 256 are written as 0
        icon += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    (IMAGES / "logo.ico").write_bytes(icon + b"".join(data for _, data in images))


if __name__ == "__main__":
    main()

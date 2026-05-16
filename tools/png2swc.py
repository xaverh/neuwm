#!/usr/bin/env python3

import getopt
import subprocess
import sys
from pathlib import Path
# yes its python, sorry 

PARTS = [
    ("top_left", "top_left.png"),
    ("top", "top.png"),
    ("top_right", "top_right.png"),
    ("left", "left.png"),
    ("right", "right.png"),
    ("bottom_left", "bottom_left.png"),
    ("bottom", "bottom.png"),
    ("bottom_right", "bottom_right.png"),
]


def usage(file):
    file.write(
        "usage: png2swc.py [-a asset-dir] [-o output] [-s symbol]\n"
    )


def run(cmd):
    return subprocess.run(cmd, check=True, capture_output=True)


def image_size(path: Path):
    result = run(["magick", str(path), "-format", "%w %h", "info:"])
    width, height = result.stdout.decode("utf-8").strip().split()
    return int(width), int(height)


def image_argb8888(path: Path):
    width, height = image_size(path)
    result = run(["magick", str(path), "rgba:-"])
    raw = result.stdout
    expected = width * height * 4
    if len(raw) != expected:
        raise ValueError(f"unexpected byte count for {path}: {len(raw)} != {expected}")

    pixels = []
    for i in range(0, len(raw), 4):
        r, g, b, a = raw[i:i + 4]
        pixels.append((a << 24) | (r << 16) | (g << 8) | b)
    return width, height, pixels


def format_pixels(name, pixels):
    lines = []
    row = []
    for i, pixel in enumerate(pixels, start=1):
        row.append(f"0x{pixel:08x}u")
        if i % 8 == 0:
            lines.append(", ".join(row))
            row = []
    if row:
        lines.append(", ".join(row))

    joined = ",\n    ".join(lines)
    return f"static const uint32_t {name}_pixels[] = {{\n    {joined}\n}};\n"


def generate_header(asset_dir: Path, output: Path, symbol: str):
    arrays = []
    parts = []

    for part_name, filename in PARTS:
        path = asset_dir / filename
        width, height, pixels = image_argb8888(path)
        arrays.append(format_pixels(f"{symbol}_{part_name}", pixels))
        parts.append(
            "static const struct swc_decor_part "
            f"{symbol}_{part_name} = {{ .width = {width}u, .height = {height}u, "
            f".stride = {width * 4}u, .data = {symbol}_{part_name}_pixels }};\n"
        )

    body = "\n".join(arrays + parts)
    mapping = (
        f"static const struct swc_decor_parts {symbol}_parts = {{\n"
        f"    .top_left = {symbol}_top_left,\n"
        f"    .top = {symbol}_top,\n"
        f"    .top_right = {symbol}_top_right,\n"
        f"    .left = {symbol}_left,\n"
        f"    .right = {symbol}_right,\n"
        f"    .bottom_left = {symbol}_bottom_left,\n"
        f"    .bottom = {symbol}_bottom,\n"
        f"    .bottom_right = {symbol}_bottom_right,\n"
        f"}};\n"
    )

    guard = f"{symbol.upper()}_H"
    text = (
        f"#ifndef {guard}\n"
        f"#define {guard}\n\n"
        f"#include <stdint.h>\n"
        f"#include <swc.h>\n\n"
        f"/* generated from {asset_dir.name} png decor parts, probably dont edit this */\n\n"
        f"{body}\n"
        f"{mapping}\n"
        f"#endif\n"
    )
    output.write_text(text, encoding="ascii")


def main():
    asset_dir = "."
    output = "parts.h"
    symbol = "afterstep_qnx"

    try:
        opts, args = getopt.getopt(
            sys.argv[1:], "a:o:s:h", ["asset-dir=", "output=", "symbol=", "help"]
        )
    except getopt.GetoptError:
        usage(sys.stderr)
        raise SystemExit(1)

    for opt, val in opts:
        if opt in ("-a", "--asset-dir"):
            asset_dir = val
        elif opt in ("-o", "--output"):
            output = val
        elif opt in ("-s", "--symbol"):
            symbol = val
        elif opt in ("-h", "--help"):
            usage(sys.stdout)
            raise SystemExit(0)

    if args:
        usage(sys.stderr)
        raise SystemExit(1)

    generate_header(Path(asset_dir).resolve(), Path(output).resolve(), symbol)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build icon sizes and .ico from Etemenanki brand masters."""
from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Install Pillow: pip install Pillow", file=sys.stderr)
    raise SystemExit(1)

ROOT = Path(__file__).resolve().parents[1]
BRAND = ROOT / "assets" / "branding"
CURSOR_ASSETS = Path(
    r"C:\Users\Admin\.cursor\projects\c-Users-Admin-Projects-Etemenanki\assets"
)

HORIZONTAL_NAMES = (
    "logo-horizontal-source.png",
    "logo-header.png",
    "etemenanki-brand-horizontal-master.png",
    "etemenanki-logo-v5-slogan-horizontal.png",
)
ICON_NAMES = (
    "app-icon-source.png",
    "etemenanki-brand-icon-master.png",
    "etemenanki-logo-v1-minimal.png",
)


def find_master(candidates: tuple[str, ...]) -> Path:
    for base in (BRAND, CURSOR_ASSETS, ROOT / "assets"):
        for name in candidates:
            path = base / name
            if path.is_file():
                return path
    raise FileNotFoundError(f"No master found among {candidates}")


def save_png(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if img.mode not in ("RGB", "RGBA"):
        img = img.convert("RGBA")
    img.save(path, format="PNG", optimize=True)


def write_ico(images: list[Image.Image], path: Path) -> None:
    """Write a standard multi-size Windows .ico."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not images:
        raise ValueError("No icon images to write")
    largest = max(images, key=lambda img: img.size[0] * img.size[1])
    sizes = sorted({img.size for img in images}, key=lambda s: s[0], reverse=True)
    largest.convert("RGBA").save(path, format="ICO", sizes=sizes)


def _png_bytes(img: Image.Image) -> bytes:
    from io import BytesIO

    buf = BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def square_icon(img: Image.Image, size: int = 512) -> Image.Image:
    rgba = img.convert("RGBA")
    w, h = rgba.size
    side = max(w, h)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(rgba, ((side - w) // 2, (side - h) // 2), rgba)
    return canvas.resize((size, size), Image.Resampling.LANCZOS)


def main() -> int:
    BRAND.mkdir(parents=True, exist_ok=True)
    horiz_src = find_master(HORIZONTAL_NAMES)
    try:
        icon_src = find_master(ICON_NAMES)
    except FileNotFoundError:
        icon_src = horiz_src

    horizontal = Image.open(horiz_src).convert("RGBA")
    save_png(horizontal, BRAND / "logo-horizontal.png")
    if icon_src == horiz_src:
        w, h = horizontal.size
        crop_w = max(1, int(w * 0.22))
        icon = horizontal.crop((0, 0, crop_w, h))
    else:
        icon = Image.open(icon_src).convert("RGBA")

    icon = square_icon(icon, 512)

    header_h = 72
    hw = max(1, int(horizontal.width * header_h / horizontal.height))
    save_png(horizontal.resize((hw, header_h), Image.Resampling.LANCZOS), BRAND / "logo-header.png")

    splash_w = 900
    sh = max(1, int(horizontal.height * splash_w / horizontal.width))
    save_png(horizontal.resize((splash_w, sh), Image.Resampling.LANCZOS), BRAND / "logo-splash.png")

    icon_sizes = (16, 24, 32, 48, 64, 128, 256)
    ico_images: list[Image.Image] = []
    for size in icon_sizes:
        resized = icon.resize((size, size), Image.Resampling.LANCZOS)
        save_png(resized, BRAND / f"icon-{size}.png")
        ico_images.append(resized)

    write_ico(ico_images, BRAND / "app.ico")
    save_png(icon.resize((512, 512), Image.Resampling.LANCZOS), BRAND / "icon-512.png")
    save_png(icon.resize((512, 512), Image.Resampling.LANCZOS), BRAND / "app-icon.png")

    print(f"Branding assets written to {BRAND}")
    print(f"  horizontal source: {horiz_src.name}")
    print(f"  icon source: {icon_src.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

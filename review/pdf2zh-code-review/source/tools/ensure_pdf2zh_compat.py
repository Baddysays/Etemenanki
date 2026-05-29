#!/usr/bin/env python3
"""Patch pdf2zh 1.7.x for NumPy 2.x (np.fromstring removed). Idempotent."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

OLD = "np.fromstring(pix.samples, np.uint8)"
NEW = "np.frombuffer(pix.samples, dtype=np.uint8)"


def patch_high_level(path: Path) -> bool:
    if not path.is_file():
        return False
    text = path.read_text(encoding="utf-8")
    if NEW in text:
        return False
    if OLD not in text:
        return False
    path.write_text(text.replace(OLD, NEW), encoding="utf-8")
    return True


def high_level_path() -> Path | None:
    spec = importlib.util.find_spec("pdf2zh")
    if spec is None or not spec.submodule_search_locations:
        return None
    return Path(spec.submodule_search_locations[0]) / "high_level.py"


def apply_numpy_runtime_patch() -> None:
    import numpy as np

    np.fromstring = np.frombuffer  # type: ignore[attr-defined, assignment]


def apply_pdf2zh_patches(*, quiet: bool = False) -> bool:
    """Patch site-packages and NumPy. Returns True if high_level.py was modified."""
    apply_numpy_runtime_patch()
    target = high_level_path()
    if target is None:
        return False
    changed = patch_high_level(target)
    if changed and not quiet:
        print(f"patched {target}", file=sys.stderr)
    return changed


def main() -> int:
    if importlib.util.find_spec("pdf2zh") is None:
        print("pdf2zh not installed", file=sys.stderr)
        return 0
    apply_pdf2zh_patches()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

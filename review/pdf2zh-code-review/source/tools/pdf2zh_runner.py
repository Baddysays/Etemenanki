#!/usr/bin/env python3
"""Launch pdf2zh with NumPy 2.x compatibility (pdf2zh 1.7.9)."""

from __future__ import annotations

import sys
import traceback
from pathlib import Path

# Patch before any pdf2zh import (disk + runtime NumPy).
_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from ensure_pdf2zh_compat import apply_pdf2zh_patches  # noqa: E402

apply_pdf2zh_patches(quiet=True)

from pdf2zh.pdf2zh import main  # noqa: E402


def _run(argv: list[str]) -> int:
    try:
        return int(main(argv))
    except Exception as exc:
        print(f"pdf2zh error: {exc}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(_run(sys.argv[1:]))

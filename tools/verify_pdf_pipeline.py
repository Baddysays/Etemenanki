#!/usr/bin/env python3
"""One-shot PDFMathTranslate pipeline check (no GUI)."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = Path(__file__).resolve().parent
APP_DIR = Path(os.environ.get("ETE_APP_DIR", ROOT / "build" / "Release")).resolve()
PY = Path(sys.executable)


def run(cmd: list[str], *, env: dict | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        env=env or os.environ,
        cwd=str(ROOT),
        timeout=600,
        check=False,
    )


def main() -> int:
    os.environ.setdefault("ETE_APP_DIR", str(APP_DIR))
    os.environ["HF_HUB_DISABLE_SYMLINKS_WARNING"] = "1"

    steps: list[tuple[str, int]] = []

    r = run([str(PY), str(TOOLS / "ensure_pdf2zh_compat.py")])
    steps.append(("compat patch", r.returncode))
    if r.returncode != 0:
        print(r.stderr or r.stdout)
        return 1

    runner = APP_DIR / "tools" / "pdf2zh_runner.py"
    r = run([str(PY), str(runner), "--version"])
    steps.append(("runner --version", r.returncode))
    if r.returncode != 0:
        print(r.stderr or r.stdout)
        return 1
    print((r.stdout or r.stderr).strip())

    engines = APP_DIR / "tools" / "pdf_engines.py"
    r = run([str(PY), str(engines), "probe", "--engine", "pdfmathtranslate"])
    steps.append(("probe", r.returncode))
    if r.returncode != 0:
        print(r.stderr or r.stdout)
        return 1
    print(r.stdout.strip())

    if "--translate" in sys.argv:
        sample = ROOT / "test_sample.pdf"
        if not sample.is_file():
            import fitz

            doc = fitz.open()
            page = doc.new_page()
            page.insert_text((72, 72), "Hello PDF test.", fontsize=14)
            doc.save(sample)
            doc.close()
        out = Path(os.environ.get("TEMP", "/tmp")) / "etemenanki_verify_out.pdf"
        r = run(
            [
                str(PY),
                str(engines),
                "translate",
                "--engine",
                "pdfmathtranslate",
                "--input",
                str(sample),
                "--output",
                str(out),
                "--src-lang",
                "en",
                "--dst-lang",
                "ru",
                "--model",
                "translategemma:4b",
            ],
            env={**os.environ, "ETE_APP_DIR": str(APP_DIR)},
        )
        steps.append(("translate", r.returncode))
        if r.returncode != 0:
            print(r.stderr or r.stdout)
            return 1
        data = json.loads(r.stdout.strip().splitlines()[-1])
        if not data.get("ok"):
            print(data)
            return 1
        print("translate OK:", data.get("output_path"))

    failed = [name for name, code in steps if code != 0]
    if failed:
        print("FAILED:", ", ".join(failed))
        return 1
    print("All checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

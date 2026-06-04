#!/usr/bin/env python3
"""Unified adapter for PDF layout-preserving translation engines."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

try:
    from ensure_pdf2zh_compat import apply_pdf2zh_patches

    apply_pdf2zh_patches(quiet=True)
except Exception:
    pass

ENGINES = ("etemenanki", "pdfmathtranslate")
NATIVE_ENGINES = ENGINES

LANG_TO_PDF2ZH = {
    "auto": None,
    "en": "en",
    "ru": "ru",
    "de": "de",
    "fr": "fr",
    "es": "es",
    "it": "it",
    "pt": "pt",
    "zh": "zh-CN",
    "zh-cn": "zh-CN",
    "zh-tw": "zh-TW",
    "ja": "ja",
    "ko": "ko",
    "ar": "ar",
    "uk": "uk",
    "pl": "pl",
    "tr": "tr",
    "nl": "nl",
    "sv": "sv",
    "cs": "cs",
    "hi": "hi",
}


def app_roots() -> list[Path]:
    roots: list[Path] = []
    script_dir = Path(__file__).resolve().parent
    roots.append(script_dir.parent)
    app_dir = os.environ.get("ETE_APP_DIR", "").strip()
    if app_dir:
        roots.append(Path(app_dir))
    seen: set[Path] = set()
    unique: list[Path] = []
    for root in roots:
        resolved = root.resolve()
        if resolved not in seen:
            seen.add(resolved)
            unique.append(resolved)
    return unique


def bundled_pdf2zh_exe() -> Path | None:
    for root in app_roots():
        direct = root / "engines" / "pdf2zh" / "pdf2zh.exe"
        if direct.is_file():
            return direct
        pdf2zh_dir = root / "engines" / "pdf2zh"
        if pdf2zh_dir.is_dir():
            for exe in pdf2zh_dir.rglob("pdf2zh.exe"):
                if exe.is_file():
                    return exe
    return None


def bundled_pdf2zh_runner() -> Path | None:
    local = Path(__file__).with_name("pdf2zh_runner.py")
    if local.is_file():
        return local
    for root in app_roots():
        candidate = root / "tools" / "pdf2zh_runner.py"
        if candidate.is_file():
            return candidate
    return None


def pdf2zh_runner_cmd() -> tuple[list[str], Path, str] | None:
    runner = bundled_pdf2zh_runner()
    if not runner:
        return None
    resolved = runner.resolve()
    return [sys.executable, str(resolved)], resolved.parent, "runner"


def _pdf2zh_importable() -> bool:
    try:
        import importlib.util

        return importlib.util.find_spec("pdf2zh") is not None
    except Exception:
        return False


def resolve_pdf2zh() -> tuple[list[str], Path | None, str]:
    """Return (argv prefix, working directory, source label).

    Prefer the Python runner (NumPy 2 patch) when pdf2zh is installed in this interpreter.
    Portable pdf2zh.exe is used only if the runner is unavailable.
    """
    runner = pdf2zh_runner_cmd()
    if runner and _pdf2zh_importable():
        return runner

    bundled = bundled_pdf2zh_exe()
    if bundled:
        return [str(bundled)], bundled.parent, "portable bundle"

    if runner:
        return runner

    exe = shutil.which("pdf2zh")
    if exe:
        runner = pdf2zh_runner_cmd()
        if runner:
            return runner
        return [exe], Path(exe).parent, "PATH"

    return [], None, ""


def normalize_ollama_url(url: str) -> str:
    """Fix http:\\host from Qt toNativeSeparators and normalize host-only values."""
    u = (url or "").strip().replace("\\", "/")
    if not u:
        return "http://127.0.0.1:11434"
    if not u.lower().startswith(("http://", "https://")):
        u = "http://" + u.lstrip("/")
    return u.rstrip("/")


def emit(obj: dict) -> None:
    sys.stdout.buffer.write(json.dumps(obj, ensure_ascii=True).encode("utf-8"))
    sys.stdout.buffer.write(b"\n")
    sys.stdout.buffer.flush()


def find_pdf2zh() -> list[str] | None:
    cmd, _, _ = resolve_pdf2zh()
    return cmd or None


def probe_pdfmathtranslate() -> dict:
    cmd, workdir, source = resolve_pdf2zh()
    if not cmd:
        return {
            "available": False,
            "message": "pdf2zh не найден. Запустите: powershell -File tools/setup_pdf2zh.ps1",
        }
    try:
        proc = subprocess.run(
            cmd + ["--version"],
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
            cwd=str(workdir) if workdir else None,
        )
        version = (proc.stdout or proc.stderr or "").strip().splitlines()
        version_text = version[0] if version else "pdf2zh"
        if source == "portable bundle":
            version_text += " (portable)"
        return {"available": True, "message": version_text, "source": source}
    except Exception as exc:
        return {"available": False, "message": str(exc)}


def probe_engine(name: str) -> dict:
    if name == "etemenanki":
        script = Path(__file__).with_name("pdf_layout.py")
        ok = script.exists()
        return {
            "available": ok,
            "message": "Встроенный PyMuPDF (Etemenanki)" if ok else "pdf_layout.py не найден",
            "source": "embedded python",
        }
    if name == "pdfmathtranslate":
        return probe_pdfmathtranslate()
    return {"available": False, "message": f"Неизвестный движок: {name}"}


def cmd_probe(args: argparse.Namespace) -> int:
    engines = {}
    targets = NATIVE_ENGINES if args.engine == "all" else (args.engine,)
    for name in targets:
        engines[name] = probe_engine(name)
    emit({"ok": True, "engines": engines})
    return 0


def newest_pdf(directory: Path, stem_hint: str) -> Path | None:
    """Pick translated PDF from pdf2zh output (v1.7+ uses -zh, older builds use -mono)."""
    patterns = [
        f"{stem_hint}-zh.pdf",
        f"{stem_hint}-mono.pdf",
        f"{stem_hint}-dual.pdf",
    ]
    for name in patterns:
        candidate = directory / name
        if candidate.is_file() and candidate.stat().st_size > 0:
            return candidate

    candidates = list(directory.glob("*.pdf"))
    if not candidates:
        return None
    related = [p for p in candidates if stem_hint in p.stem]
    if related:
        for suffix in ("-zh", "-mono", "-dual"):
            tagged = [p for p in related if p.stem.endswith(suffix)]
            if tagged:
                return max(tagged, key=lambda p: p.stat().st_mtime)
        return max(related, key=lambda p: p.stat().st_mtime)
    return max(candidates, key=lambda p: p.stat().st_mtime)


def translate_pdfmathtranslate(
    input_path: Path,
    output_path: Path,
    src_lang: str,
    dst_lang: str,
    runtime: str,
    model: str,
    ollama_url: str,
    cloud_base: str,
    cloud_key: str,
) -> dict:
    cmd, workdir, source = resolve_pdf2zh()
    if not cmd:
        return {
            "ok": False,
            "error": "pdf2zh не найден. Запустите: powershell -File tools/setup_pdf2zh.ps1",
        }

    out_dir = output_path.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"
    env["HF_HUB_DISABLE_SYMLINKS_WARNING"] = "1"

    # pdf2zh v1.7.x has no -o flag; writes {basename}-zh.pdf into cwd
    run_cwd = out_dir
    input_arg = str(input_path.resolve())

    args = cmd + [input_arg]
    src = LANG_TO_PDF2ZH.get(src_lang.lower(), src_lang)
    dst = LANG_TO_PDF2ZH.get(dst_lang.lower(), dst_lang)
    if src:
        args += ["-li", src]
    if dst:
        args += ["-lo", dst]

    api_key = (cloud_key or os.environ.get("OPENAI_API_KEY") or "").strip()
    if runtime == "cloud" and cloud_base and api_key:
        env["OPENAI_BASE_URL"] = cloud_base.rstrip("/")
        if not env["OPENAI_BASE_URL"].endswith("/v1"):
            env["OPENAI_BASE_URL"] += "/v1"
        env["OPENAI_API_KEY"] = api_key
        env["OPENAI_MODEL"] = model
        args += ["-s", f"openai:{model}"]
    else:
        host = normalize_ollama_url(ollama_url)
        env["OLLAMA_HOST"] = host
        env["OLLAMA_MODEL"] = model or "gemma2"
        args += ["-s", f"ollama:{model or env['OLLAMA_MODEL']}"]

    proc = subprocess.run(
        args,
        capture_output=True,
        text=True,
        env=env,
        cwd=str(run_cwd),
        timeout=max(600, int(os.environ.get("ETE_PDF2ZH_TIMEOUT", "7200"))),
        check=False,
    )
    if proc.returncode != 0:
        err = (proc.stderr or proc.stdout or "").strip()
        return {
            "ok": False,
            "error": err[:2000] or f"pdf2zh завершился с кодом {proc.returncode}",
        }

    produced = newest_pdf(run_cwd, input_path.stem)
    if not produced or not produced.exists():
        return {"ok": False, "error": "pdf2zh не создал выходной PDF"}

    shutil.copy2(produced, output_path)
    return {"ok": True, "output_path": str(output_path), "engine_output": str(produced)}


def cmd_translate(args: argparse.Namespace) -> int:
    engine = args.engine
    input_path = Path(args.input).resolve()
    output_path = Path(args.output).resolve()
    if not input_path.exists():
        emit({"ok": False, "error": f"Файл не найден: {input_path}"})
        return 1

    if engine == "etemenanki":
        emit({"ok": False, "error": "Движок etemenanki использует встроенный C++ пайплайн"})
        return 1

    if engine == "pdfmathtranslate":
        result = translate_pdfmathtranslate(
            input_path,
            output_path,
            args.src_lang,
            args.dst_lang,
            args.runtime,
            args.model,
            args.ollama_url,
            args.cloud_base,
            args.cloud_key,
        )
        emit(result)
        return 0 if result.get("ok") else 2

    emit({"ok": False, "error": f"Неизвестный движок: {engine}"})
    return 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Etemenanki PDF engine adapter")
    sub = parser.add_subparsers(dest="command", required=True)

    probe = sub.add_parser("probe")
    probe.add_argument("--engine", default="all", choices=["all", *NATIVE_ENGINES])
    probe.set_defaults(func=cmd_probe)

    translate = sub.add_parser("translate")
    translate.add_argument("--engine", required=True, choices=NATIVE_ENGINES)
    translate.add_argument("--input", required=True)
    translate.add_argument("--output", required=True)
    translate.add_argument("--src-lang", default="en")
    translate.add_argument("--dst-lang", default="ru")
    translate.add_argument("--runtime", default="local")
    translate.add_argument("--model", default="")
    translate.add_argument("--ollama-url", default="http://127.0.0.1:11434")
    translate.add_argument("--cloud-base", default="")
    translate.add_argument(
        "--cloud-key",
        default="",
        help="Optional; prefer OPENAI_API_KEY env (set by Etemenanki, not logged)",
    )
    translate.set_defaults(func=cmd_translate)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())

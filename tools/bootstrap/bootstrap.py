#!/usr/bin/env python3
"""First-run bootstrap: probe hardware, recommend models, install PDF/Python/Ollama deps."""
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "assets" / "models_catalog.json"
MANIFEST = Path(__file__).resolve().parent / "install_manifest.json"


def log(msg: str) -> None:
    print(msg, flush=True)


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def probe_ram_gb() -> float:
    if platform.system() == "Windows":
        try:
            import ctypes

            class MEMORYSTATUSEX(ctypes.Structure):
                _fields_ = [
                    ("dwLength", ctypes.c_ulong),
                    ("dwMemoryLoad", ctypes.c_ulong),
                    ("ullTotalPhys", ctypes.c_ulonglong),
                    ("ullAvailPhys", ctypes.c_ulonglong),
                    ("ullTotalPageFile", ctypes.c_ulonglong),
                    ("ullAvailPageFile", ctypes.c_ulonglong),
                    ("ullTotalVirtual", ctypes.c_ulonglong),
                    ("ullAvailVirtual", ctypes.c_ulonglong),
                    ("ullAvailExtendedVirtual", ctypes.c_ulonglong),
                ]

            stat = MEMORYSTATUSEX()
            stat.dwLength = ctypes.sizeof(MEMORYSTATUSEX)
            ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(stat))
            return round(stat.ullTotalPhys / (1024**3), 1)
        except Exception:
            pass
    try:
        import psutil

        return round(psutil.virtual_memory().total / (1024**3), 1)
    except Exception:
        return 8.0


def probe_vram_gb() -> float | None:
    if platform.system() != "Windows":
        return None
    try:
        import ctypes
        from ctypes import wintypes

        class DISPLAY_DEVICEW(ctypes.Structure):
            _fields_ = [
                ("cb", wintypes.DWORD),
                ("DeviceName", wintypes.WCHAR * 32),
                ("DeviceString", wintypes.WCHAR * 128),
                ("StateFlags", wintypes.DWORD),
                ("DeviceID", wintypes.WCHAR * 128),
                ("DeviceKey", wintypes.WCHAR * 128),
            ]

        # Fallback: try nvidia-smi
        try:
            out = subprocess.check_output(
                ["nvidia-smi", "--query-gpu=memory.total", "--format=csv,noheader,nounits"],
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=5,
            )
            lines = [ln.strip() for ln in out.splitlines() if ln.strip()]
            if lines:
                return round(max(float(x) for x in lines) / 1024.0, 1)
        except Exception:
            pass
    except Exception:
        pass
    return None


def hardware_tier(ram_gb: float, vram_gb: float | None) -> str:
    v = vram_gb if vram_gb is not None else 0.0
    if ram_gb <= 11 or v < 4:
        return "light"
    if ram_gb <= 23 or v < 10:
        return "balanced"
    return "quality"


def recommend_models(catalog: dict, tier: str) -> list[dict]:
    out: list[dict] = []
    for m in catalog.get("models", []):
        if m.get("provider") != "ollama":
            continue
        mt = m.get("tier", "balanced")
        ram = float(m.get("required_ram_gb", 8))
        vram = m.get("required_vram_gb")
        vram_f = float(vram) if vram is not None else 0.0
        fit = mt == tier or (tier == "quality" and mt in ("balanced", "quality")) or (
            tier == "balanced" and mt in ("light", "balanced")
        )
        if tier == "light" and ram > 12:
            fit = mt == "light"
        recommended = mt == tier or (tier == "quality" and m["id"].startswith("translategemma:12b"))
        if tier == "light" and m["id"] == "translategemma:4b":
            recommended = True
        if tier == "balanced" and m["id"] in ("translategemma:4b", "qwen2.5:7b"):
            recommended = True
        if tier == "quality" and m["id"] in ("translategemma:12b", "translategemma:4b"):
            recommended = True
        out.append(
            {
                "id": m["id"],
                "tier": mt,
                "translation_quality": m.get("translation_quality", 3),
                "speed_score": m.get("speed_score", 3),
                "required_ram_gb": ram,
                "required_vram_gb": vram,
                "quality_label": m.get("quality_label", ""),
                "speed_label": m.get("speed_label", ""),
                "description": m.get("description", ""),
                "description_ru": m.get("description_ru", m.get("description", "")),
                "fits_hardware": fit,
                "recommended": recommended,
            }
        )
    out.sort(key=lambda x: (not x["recommended"], -x["translation_quality"]))
    return out


def find_app_root() -> Path:
    env = os.environ.get("ETEMENANKI_ROOT")
    if env:
        return Path(env)
    # tools/bootstrap -> repo or install dir
    cand = ROOT
    for _ in range(4):
        if (cand / "Etemenanki.exe").exists() or (cand / "assets" / "models_catalog.json").exists():
            return cand
        if cand.parent == cand:
            break
        cand = cand.parent
    return ROOT


def resolve_python(app_root: Path) -> Path | None:
    for rel in (
        "engines/python/python.exe",
        "tools/python_path.txt",
        ".venv/Scripts/python.exe",
    ):
        p = app_root / rel
        if rel.endswith(".txt") and p.exists():
            line = p.read_text(encoding="utf-8").strip().splitlines()[0].strip()
            if line and Path(line).exists():
                return Path(line)
        elif p.exists():
            return p
    for name in ("py", "python"):
        found = shutil.which(name)
        if found:
            return Path(found)
    return None


def pdf2zh_ready(app_root: Path) -> bool:
    engines = app_root / "engines" / "pdf2zh"
    if engines.exists():
        for exe in engines.rglob("pdf2zh.exe"):
            if exe.is_file():
                return True
    py = resolve_python(app_root)
    if not py:
        return False
    try:
        subprocess.check_call(
            [str(py), "-c", "import pdf2zh"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=30,
        )
        return True
    except Exception:
        return False


def install_pdf2zh_portable(app_root: Path) -> bool:
    script = app_root / "tools" / "setup_pdf2zh.ps1"
    if not script.exists():
        script = ROOT / "tools" / "setup_pdf2zh.ps1"
    if not script.exists():
        log("setup_pdf2zh.ps1 not found")
        return False
    cmd = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(script),
    ]
    log("Installing portable pdf2zh...")
    subprocess.check_call(cmd, cwd=str(app_root))
    return pdf2zh_ready(app_root)


def install_python_deps(app_root: Path) -> bool:
    py = resolve_python(app_root)
    req = app_root / "tools" / "requirements-pdf.txt"
    if not py or not req.exists():
        log("Python or requirements-pdf.txt missing — skip pip deps")
        return False
    log(f"Installing Python deps via {py}...")
    subprocess.check_call([str(py), "-m", "pip", "install", "-q", "-r", str(req)])
    compat = app_root / "tools" / "ensure_pdf2zh_compat.py"
    if compat.exists():
        subprocess.check_call([str(py), str(compat)])
    return True


def ollama_available() -> bool:
    if shutil.which("ollama"):
        return True
    try:
        urllib.request.urlopen("http://127.0.0.1:11434/api/tags", timeout=2)
        return True
    except Exception:
        return False


def ollama_pull(model: str) -> bool:
    if not shutil.which("ollama"):
        log("ollama CLI not in PATH — install from https://ollama.com/download")
        return False
    log(f"ollama pull {model} ...")
    subprocess.check_call(["ollama", "pull", model])
    return True


def cmd_probe(_: argparse.Namespace) -> int:
    catalog = load_json(CATALOG if CATALOG.exists() else find_app_root() / "assets" / "models_catalog.json")
    ram = probe_ram_gb()
    vram = probe_vram_gb()
    tier = hardware_tier(ram, vram)
    app_root = find_app_root()
    result = {
        "ok": True,
        "ram_gb": ram,
        "vram_gb": vram,
        "hardware_tier": tier,
        "tier_labels": catalog.get("hardware_tiers", {}),
        "recommendations": recommend_models(catalog, tier),
        "deps": {
            "python": resolve_python(app_root) is not None,
            "pdf2zh": pdf2zh_ready(app_root),
            "ollama": ollama_available(),
        },
        "app_root": str(app_root),
    }
    print(json.dumps(result, ensure_ascii=False))
    return 0


def cmd_install(args: argparse.Namespace) -> int:
    app_root = find_app_root()
    os.environ.setdefault("ETEMENANKI_ROOT", str(app_root))
    ok = True
    if args.pdf2zh and not pdf2zh_ready(app_root):
        try:
            ok = install_pdf2zh_portable(app_root) and ok
        except subprocess.CalledProcessError:
            ok = False
    if args.python_deps:
        try:
            install_python_deps(app_root)
        except subprocess.CalledProcessError:
            ok = False
    if args.ollama_models:
        if not ollama_available():
            log("WARNING: Ollama not running. Install Ollama and run: ollama serve")
            ok = False
        else:
            for m in args.ollama_models:
                try:
                    ollama_pull(m)
                except subprocess.CalledProcessError:
                    ok = False
    print(json.dumps({"ok": ok}))
    return 0 if ok else 1


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)
    p_probe = sub.add_parser("probe")
    p_probe.set_defaults(func=cmd_probe)
    p_inst = sub.add_parser("install")
    p_inst.add_argument("--pdf2zh", action="store_true")
    p_inst.add_argument("--python-deps", action="store_true")
    p_inst.add_argument("--ollama-models", nargs="*", default=[])
    p_inst.set_defaults(func=cmd_install)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

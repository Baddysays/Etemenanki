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


def emit_json(obj: dict) -> None:
    """Write UTF-8 JSON to stdout (Windows pipes break with print + Cyrillic)."""
    sys.stdout.buffer.write(json.dumps(obj, ensure_ascii=False).encode("utf-8"))
    sys.stdout.buffer.write(b"\n")
    sys.stdout.buffer.flush()


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


def gpu_kind(name: str) -> str:
    n = name.lower()
    if any(
        x in n
        for x in (
            "geforce",
            "rtx ",
            "gtx ",
            "quadro",
            "tesla",
            "radeon rx",
            "arc a",
            "arc b",
        )
    ):
        return "discrete"
    if any(
        x in n
        for x in (
            "intel",
            "uhd",
            "iris",
            "vega",
            "radeon graphics",
            "basic display",
            "microsoft",
        )
    ):
        return "integrated"
    return "unknown"


def probe_gpus() -> list[dict]:
    gpus: list[dict] = []
    seen: set[str] = set()

    if platform.system() == "Windows":
        try:
            out = subprocess.check_output(
                [
                    "nvidia-smi",
                    "--query-gpu=index,name,memory.total",
                    "--format=csv,noheader,nounits",
                ],
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=8,
            )
            for line in out.splitlines():
                parts = [p.strip() for p in line.split(",")]
                if len(parts) < 3:
                    continue
                idx = int(parts[0])
                name = parts[1]
                vram_gb = round(float(parts[2]) / 1024.0, 1)
                key = f"nvidia:{idx}:{name}"
                if key in seen:
                    continue
                seen.add(key)
                gpus.append(
                    {
                        "index": idx,
                        "name": name,
                        "vram_gb": vram_gb,
                        "kind": gpu_kind(name),
                        "vendor": "nvidia",
                    }
                )
        except Exception:
            pass

        try:
            ps = (
                "Get-CimInstance Win32_VideoController | "
                "Select-Object Name,AdapterRAM | ConvertTo-Json -Compress"
            )
            raw = subprocess.check_output(
                ["powershell", "-NoProfile", "-Command", ps],
                stderr=subprocess.DEVNULL,
                timeout=12,
            )
            data = json.loads(raw.decode("utf-8", errors="replace"))
            items = data if isinstance(data, list) else [data]
            for i, item in enumerate(items):
                name = str(item.get("Name", "")).strip()
                if not name:
                    continue
                ram = item.get("AdapterRAM") or 0
                try:
                    vram_gb = round(int(ram) / (1024**3), 1) if int(ram) > 0 else 0.0
                except (TypeError, ValueError):
                    vram_gb = 0.0
                key = f"wmi:{name}"
                if key in seen:
                    continue
                seen.add(key)
                kind = gpu_kind(name)
                gpus.append(
                    {
                        "index": i,
                        "name": name,
                        "vram_gb": vram_gb,
                        "kind": kind,
                        "vendor": "wmi",
                    }
                )
        except Exception:
            pass

    return gpus


def recommended_gpu_index(gpus: list[dict]) -> int:
    if not gpus:
        return -1
    discrete = [g for g in gpus if g.get("kind") == "discrete"]
    pool = discrete if discrete else gpus
    best = max(pool, key=lambda g: float(g.get("vram_gb") or 0))
    return int(best.get("index", -1))


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


def model_fits_hardware(ram_gb: float, vram_gb: float | None, m: dict) -> bool:
    req_ram = float(m.get("required_ram_gb", 8))
    if ram_gb + 0.5 < req_ram:
        return False
    req_vram = m.get("required_vram_gb")
    if req_vram is not None and vram_gb is not None:
        if vram_gb + 0.5 < float(req_vram):
            return False
    return True


def is_recommended_for_tier(model_id: str, tier: str) -> bool:
    picks = {
        "light": ("translategemma:4b",),
        "balanced": ("translategemma:4b", "qwen2.5:7b"),
        "quality": ("translategemma:12b", "translategemma:4b"),
    }
    return model_id in picks.get(tier, ())


def embedded_model_ready(app_root: Path) -> bool:
    script = app_root / "tools" / "embedded_llm.py"
    py = resolve_python(app_root)
    if not py or not script.exists():
        return False
    try:
        out = subprocess.check_output(
            [str(py), str(script), "status"],
            stderr=subprocess.DEVNULL,
            timeout=30,
            env={**os.environ, "ETEMENANKI_ROOT": str(app_root)},
        )
        data = json.loads(out.decode("utf-8", errors="replace"))
        return bool(data.get("ready"))
    except Exception:
        return False


def recommend_models(
    catalog: dict, tier: str, ram_gb: float, vram_gb: float | None, installed: list[str] | None = None
) -> list[dict]:
    installed = installed or []
    out: list[dict] = []
    app_root = find_app_root()
    emb_ready = embedded_model_ready(app_root)
    for m in catalog.get("models", []):
        provider = m.get("provider")
        if provider == "embedded":
            fit = model_fits_hardware(ram_gb, vram_gb, m)
            out.append(
                {
                    "id": m["id"],
                    "provider": "embedded",
                    "tier": m.get("tier", "light"),
                    "translation_quality": m.get("translation_quality", 3),
                    "speed_score": m.get("speed_score", 3),
                    "required_ram_gb": float(m.get("required_ram_gb", 8)),
                    "required_vram_gb": m.get("required_vram_gb"),
                    "quality_label_en": m.get("quality_label_en", m.get("quality_label", "")),
                    "quality_label_ru": m.get("quality_label_ru", m.get("quality_label", "")),
                    "speed_label_en": m.get("speed_label_en", m.get("speed_label", "")),
                    "speed_label_ru": m.get("speed_label_ru", m.get("speed_label", "")),
                    "description": m.get("description", ""),
                    "description_ru": m.get("description_ru", m.get("description", "")),
                    "fits_hardware": fit,
                    "recommended": fit,
                    "installed": emb_ready,
                }
            )
            continue
        if provider != "ollama":
            continue
        mt = m.get("tier", "balanced")
        ram = float(m.get("required_ram_gb", 8))
        vram = m.get("required_vram_gb")
        fit = model_fits_hardware(ram_gb, vram_gb, m)
        recommended = fit and is_recommended_for_tier(m["id"], tier)
        out.append(
            {
                "id": m["id"],
                "tier": mt,
                "translation_quality": m.get("translation_quality", 3),
                "speed_score": m.get("speed_score", 3),
                "required_ram_gb": ram,
                "required_vram_gb": vram,
                "quality_label_en": m.get("quality_label_en", m.get("quality_label", "")),
                "quality_label_ru": m.get("quality_label_ru", m.get("quality_label", "")),
                "speed_label_en": m.get("speed_label_en", m.get("speed_label", "")),
                "speed_label_ru": m.get("speed_label_ru", m.get("speed_label", "")),
                "description": m.get("description", ""),
                "description_ru": m.get("description_ru", m.get("description", "")),
                "fits_hardware": fit,
                "recommended": recommended,
                "installed": model_is_installed(m["id"], installed),
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


def probe_python_info(app_root: Path) -> dict:
    py = resolve_python(app_root)
    if not py:
        return {"found": False, "path": "", "version": "", "libs_ok": False}
    info = {"found": True, "path": str(py), "version": "", "libs_ok": False}
    try:
        out = subprocess.check_output(
            [
                str(py),
                "-c",
                "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}')",
            ],
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
        info["version"] = out.strip()
    except Exception:
        pass
    try:
        subprocess.check_call(
            [str(py), "-c", "import pymupdf, docx, openpyxl"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=25,
        )
        info["libs_ok"] = True
    except Exception:
        pass
    return info


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


def ollama_installed_models() -> list[str]:
    names: list[str] = []
    if shutil.which("ollama"):
        try:
            out = subprocess.check_output(
                ["ollama", "list"],
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=10,
            )
            for line in out.splitlines()[1:]:
                parts = line.split()
                if parts:
                    names.append(parts[0])
        except Exception:
            pass
    try:
        with urllib.request.urlopen("http://127.0.0.1:11434/api/tags", timeout=3) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            for item in data.get("models", []):
                name = item.get("name", "").strip()
                if name and name not in names:
                    names.append(name)
    except Exception:
        pass
    return names


def model_is_installed(model_id: str, installed: list[str]) -> bool:
    for name in installed:
        if name == model_id or name.startswith(model_id + ":") or model_id.startswith(name + ":"):
            return True
    return False


def ollama_available() -> bool:
    if ollama_installed_models():
        return True
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
    installed = ollama_installed_models()
    recs = recommend_models(catalog, tier, ram, vram, installed)
    gpus = probe_gpus()
    rec_gpu = recommended_gpu_index(gpus)
    py_info = probe_python_info(app_root)
    emb = embedded_model_ready(app_root)
    result = {
        "ok": True,
        "ram_gb": ram,
        "vram_gb": vram,
        "gpus": gpus,
        "recommended_gpu_index": rec_gpu,
        "hardware_tier": tier,
        "tier_labels": catalog.get("hardware_tiers", {}),
        "recommendations": recs,
        "ollama_installed": installed,
        "deps": {
            "python": py_info["found"],
            "python_path": py_info["path"],
            "python_version": py_info["version"],
            "python_libs": py_info["libs_ok"],
            "pdf2zh": pdf2zh_ready(app_root),
            "ollama": ollama_available(),
            "embedded": emb,
        },
        "app_root": str(app_root),
    }
    emit_json(result)
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
    if getattr(args, "embedded_model", False):
        py = resolve_python(app_root)
        script = app_root / "tools" / "embedded_llm.py"
        req = app_root / "tools" / "requirements-embedded-download.txt"
        if not req.exists():
            req = app_root / "tools" / "requirements-embedded.txt"
        if not py:
            log("WARNING: Python not found for embedded model")
            ok = False
        else:
            try:
                if req.exists():
                    subprocess.check_call(
                        [str(py), "-m", "pip", "install", "--prefer-binary", "-q", "-r", str(req)],
                        cwd=str(app_root),
                    )
                else:
                    subprocess.check_call(
                        [str(py), "-m", "pip", "install", "--prefer-binary", "-q", "huggingface_hub"],
                        cwd=str(app_root),
                    )
                subprocess.check_call(
                    [str(py), str(script), "download"],
                    cwd=str(app_root),
                    env={**os.environ, "ETEMENANKI_ROOT": str(app_root)},
                )
            except subprocess.CalledProcessError:
                ok = False
    emit_json({"ok": ok})
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
    p_inst.add_argument("--embedded-model", action="store_true")
    p_inst.set_defaults(func=cmd_install)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

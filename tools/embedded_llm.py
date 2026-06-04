#!/usr/bin/env python3
"""Built-in local LLM (llama.cpp server) — no Ollama required."""

from __future__ import annotations

import argparse
import json
import os
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def find_app_root() -> Path:
    env = os.environ.get("ETEMENANKI_ROOT")
    if env:
        return Path(env)
    cand = ROOT
    for _ in range(6):
        if (cand / "assets" / "models_catalog.json").exists():
            return cand
        if (cand / "Etemenanki.exe").exists():
            return cand
        if cand.parent == cand:
            break
        cand = cand.parent
    return ROOT


def manifest_path(app_root: Path) -> Path:
    return app_root / "engines" / "llm" / "manifest.json"


def models_dir(app_root: Path) -> Path:
    return app_root / "engines" / "llm" / "models"


def load_manifest(app_root: Path) -> dict:
    path = manifest_path(app_root)
    if not path.exists():
        return {"models": [], "server_port": 11435, "server_host": "127.0.0.1"}
    return json.loads(path.read_text(encoding="utf-8"))


def default_model(manifest: dict) -> dict | None:
    models = manifest.get("models") or []
    if not models:
        return None
    default_id = manifest.get("default_model_id")
    for m in models:
        if m.get("id") == default_id:
            return m
    return models[0]


def model_gguf_path(app_root: Path, model: dict) -> Path:
    return models_dir(app_root) / str(model.get("filename", ""))


def server_url(manifest: dict) -> str:
    host = manifest.get("server_host", "127.0.0.1")
    port = int(manifest.get("server_port", 11435))
    return f"http://{host}:{port}"


def port_open(host: str, port: int, timeout: float = 1.5) -> bool:
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def emit_json(obj: dict) -> None:
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def emit_phase(phase: str) -> None:
    print(f"ETEMENANKI_PHASE {phase}", flush=True)


def emit_progress(percent: int) -> None:
    pct = max(0, min(100, int(percent)))
    print(f"ETEMENANKI_PROGRESS {pct}", flush=True)


def _watch_download_progress(dest: Path, models_root: Path, expected_bytes: int, stop: threading.Event) -> None:
    while not stop.wait(1.5):
        best = 0
        if dest.is_file():
            best = dest.stat().st_size
        if models_root.is_dir():
            for p in models_root.rglob("*.gguf"):
                try:
                    best = max(best, p.stat().st_size)
                except OSError:
                    pass
        if expected_bytes > 0 and best > 0:
            emit_progress(min(99, int(best * 100 / expected_bytes)))


def cmd_status(app_root: Path) -> int:
    manifest = load_manifest(app_root)
    model = default_model(manifest)
    if not model:
        emit_json({"ready": False, "server_up": False, "error": "no manifest model"})
        return 1
    path = model_gguf_path(app_root, model)
    host = manifest.get("server_host", "127.0.0.1")
    port = int(manifest.get("server_port", 11435))
    emit_json(
        {
            "ready": path.is_file(),
            "server_up": port_open(host, port),
            "model_id": model.get("id"),
            "model_path": str(path),
            "server_url": server_url(manifest),
            "size_mb": model.get("size_mb"),
        }
    )
    return 0


def cmd_download(app_root: Path) -> int:
    manifest = load_manifest(app_root)
    model = default_model(manifest)
    if not model:
        print("No embedded model in manifest", file=sys.stderr)
        return 1
    dest = model_gguf_path(app_root, model)
    if dest.is_file():
        print(f"Already downloaded: {dest}")
        return 0
    models_dir(app_root).mkdir(parents=True, exist_ok=True)
    try:
        from huggingface_hub import hf_hub_download
    except ImportError:
        print("Install: pip install huggingface_hub llama-cpp-python", file=sys.stderr)
        return 2
    repo_id = model.get("repo_id")
    repo_file = model.get("repo_file") or model.get("filename")
    if not repo_id or not repo_file:
        print("Manifest missing repo_id/repo_file", file=sys.stderr)
        return 1
    min_bytes = int(float(model.get("size_mb", 1500)) * 1024 * 1024 * 0.85)
    if dest.is_file() and dest.stat().st_size >= min_bytes:
        print(f"Already complete: {dest} ({dest.stat().st_size // (1024 * 1024)} MB)")
        return 0
    if dest.is_file() and dest.stat().st_size < min_bytes:
        print(f"Removing incomplete file ({dest.stat().st_size} bytes)...")
        dest.unlink()

    expected_bytes = int(float(model.get("size_mb", 1500)) * 1024 * 1024)
    mdir = models_dir(app_root)
    print(f"Downloading {repo_id} / {repo_file} (~{model.get('size_mb', '?')} MB)...", flush=True)
    emit_phase("download")
    emit_progress(0)
    stop = threading.Event()
    watcher = threading.Thread(
        target=_watch_download_progress,
        args=(dest, mdir, expected_bytes, stop),
        daemon=True,
    )
    watcher.start()
    try:
        os.environ["HF_HUB_DISABLE_PROGRESS_BARS"] = "1"
        cached = hf_hub_download(
            repo_id=repo_id,
            filename=repo_file,
            local_dir=str(mdir),
            resume_download=True,
        )
    finally:
        stop.set()
        watcher.join(timeout=3)
    cached_path = Path(cached)
    if cached_path.resolve() != dest.resolve():
        if dest.exists():
            dest.unlink()
        cached_path.replace(dest)
    if not dest.is_file() or dest.stat().st_size < min_bytes:
        print("Download incomplete — check internet and retry", file=sys.stderr)
        return 1
    emit_progress(100)
    print(f"Saved: {dest} ({dest.stat().st_size // (1024 * 1024)} MB)")
    flag = app_root / "engines" / "llm" / ".install_embedded_mode"
    flag.parent.mkdir(parents=True, exist_ok=True)
    flag.write_text("1", encoding="utf-8")
    return 0


def cmd_install_embedded(app_root: Path) -> int:
    """pip huggingface_hub + download GGUF with progress lines for the UI."""
    emit_phase("pip")
    emit_progress(0)
    py = sys.executable
    req_dl = app_root / "tools" / "requirements-embedded-download.txt"
    pip_args = [py, "-m", "pip", "install", "--prefer-binary"]
    if req_dl.is_file():
        pip_args += ["-r", str(req_dl)]
    else:
        pip_args += ["huggingface_hub"]
    print("Installing huggingface_hub...", flush=True)
    r = subprocess.run(pip_args, cwd=str(app_root))
    if r.returncode != 0:
        print("ERROR: pip install huggingface_hub failed", file=sys.stderr)
        return r.returncode
    emit_progress(5)
    return cmd_download(app_root)


def _start_server_process(app_root: Path, model_path: Path, manifest: dict) -> subprocess.Popen | None:
    model = default_model(manifest) or {}
    n_ctx = int(model.get("n_ctx", 4096))
    n_gpu = int(model.get("n_gpu_layers", -1))
    host = manifest.get("server_host", "127.0.0.1")
    port = int(manifest.get("server_port", 11435))
    args = [
        sys.executable,
        "-m",
        "llama_cpp.server",
        "--model",
        str(model_path),
        "--host",
        host,
        "--port",
        str(port),
        "--n_ctx",
        str(n_ctx),
        "--n_gpu_layers",
        str(n_gpu),
    ]
    creationflags = 0
    if sys.platform == "win32":
        creationflags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    return subprocess.Popen(
        args,
        cwd=str(app_root),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        creationflags=creationflags,
    )


def cmd_serve(app_root: Path) -> int:
    manifest = load_manifest(app_root)
    model = default_model(manifest)
    if not model:
        print("No embedded model configured", file=sys.stderr)
        return 1
    path = model_gguf_path(app_root, model)
    if not path.is_file():
        print(f"Model not found: {path}\nRun: python tools/embedded_llm.py download", file=sys.stderr)
        return 3
    try:
        import llama_cpp  # noqa: F401
    except ImportError:
        print("Installing llama-cpp-python (first run, may take a few minutes)...", flush=True)
        import subprocess as sp

        req = find_app_root() / "tools" / "requirements-embedded.txt"
        cmd = [sys.executable, "-m", "pip", "install", "--prefer-binary", "-q"]
        if req.is_file():
            cmd += ["-r", str(req)]
        else:
            cmd += ["llama-cpp-python"]
        if sp.call(cmd) != 0:
            print("pip install llama-cpp-python failed", file=sys.stderr)
            return 2
        import llama_cpp  # noqa: F401

    host = manifest.get("server_host", "127.0.0.1")
    port = int(manifest.get("server_port", 11435))
    if port_open(host, port):
        print(f"Built-in AI already running on {server_url(manifest)}")
        return 0

    _start_server_process(app_root, path, manifest)
    for _ in range(45):
        time.sleep(2)
        if port_open(host, port):
            print(f"Built-in AI ready at {server_url(manifest)}")
            return 0
    print(
        f"Server did not start on port {port}. Check VRAM/RAM or reinstall llama-cpp-python.",
        file=sys.stderr,
    )
    return 4


def main() -> int:
    parser = argparse.ArgumentParser(description="Etemenanki built-in LLM")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status")
    sub.add_parser("download")
    sub.add_parser("install-embedded")
    sub.add_parser("serve")
    args = parser.parse_args()
    app_root = find_app_root()
    if args.cmd == "status":
        return cmd_status(app_root)
    if args.cmd == "download":
        return cmd_download(app_root)
    if args.cmd == "install-embedded":
        return cmd_install_embedded(app_root)
    if args.cmd == "serve":
        return cmd_serve(app_root)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Built-in local LLM (llama.cpp server) — no Ollama required.

Model is bundled with the installer — no download needed.
Commands: status, serve
"""

from __future__ import annotations

import argparse
import json
import os
import socket
import subprocess
import sys
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
    sys.stdout.buffer.write(json.dumps(obj, ensure_ascii=True).encode("utf-8"))
    sys.stdout.buffer.write(b"\n")
    sys.stdout.buffer.flush()


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
        print(f"Model not found: {path}", file=sys.stderr)
        print("Reinstall Etemenanki with the full installer.", file=sys.stderr)
        return 3
    try:
        import llama_cpp  # noqa: F401
    except ImportError:
        print("Installing llama-cpp-python (first run, may take a few minutes)...", flush=True)
        req = find_app_root() / "tools" / "requirements-embedded.txt"
        cmd = [sys.executable, "-m", "pip", "install", "--prefer-binary", "-q"]
        if req.is_file():
            cmd += ["-r", str(req)]
        else:
            cmd += ["llama-cpp-python"]
        if subprocess.call(cmd) != 0:
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
    sub.add_parser("serve")
    args = parser.parse_args()
    app_root = find_app_root()
    if args.cmd == "status":
        return cmd_status(app_root)
    if args.cmd == "serve":
        return cmd_serve(app_root)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

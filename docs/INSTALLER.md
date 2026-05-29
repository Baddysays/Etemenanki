# Etemenanki installer & first-run bootstrap

## Goal

One installer that:

1. Copies the Qt app + `tools/` + `engines/` + `assets/`
2. Optionally downloads **pdf2zh** during install (Inno `[Run]` step)
3. On first launch opens **Setup wizard** (hardware probe, model choice, `ollama pull`)
4. **Check for updates** via GitHub Releases (Settings → Updates)

## Build installer (Windows)

1. Build Release:

   ```powershell
   cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
   cmake --build build --config Release
   ```

2. Repo is configured: `Baddysays/Etemenanki` in `tools/bootstrap/install_manifest.json`.

3. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php), open `installer/EtemenankiSetup.iss`, compile.

   Output: `installer/dist/EtemenankiSetup-1.0.0.exe` (rename to `etemenanki-setup.exe` for releases)

## One-line install (PowerShell)

```powershell
irm https://raw.githubusercontent.com/Baddysays/Etemenanki/main/scripts/install-etemenanki.ps1 | iex
```

Or download manually from [Releases](https://github.com/Baddysays/Etemenanki/releases/latest).

## First-run wizard (in app)

- **Probe** — `tools/bootstrap/bootstrap.py probe` → RAM/VRAM, tier (`light` / `balanced` / `quality`), recommended Ollama models from `assets/models_catalog.json`
- **Install** — pdf2zh portable, pip deps, `ollama pull` for selected models

User must install **Ollama** separately (link in wizard). Etemenanki does not bundle Ollama (license/size).

## Recommended local models

| Tier | PC | Suggested models |
|------|-----|------------------|
| light | ≤11 GB RAM or weak GPU | `translategemma:4b` |
| balanced | 12–23 GB RAM | `translategemma:4b`, `qwen2.5:7b` |
| quality | 24+ GB RAM, 10+ GB VRAM | `translategemma:12b`, `translategemma:4b` |

Add more entries in `assets/models_catalog.json` (`provider: ollama`, `tier`, `translation_quality`).

## GitHub updates

1. Create a release tag `v1.0.1` with asset `EtemenankiSetup-1.0.1.exe`
2. App: **Settings → Check for updates** uses GitHub API `releases/latest`
3. Bump `SetupManager::appVersion()` when shipping

## CI (optional)

See `.github/workflows/release.yml` — builds on `windows-latest` with Qt 6.8 (adjust `CMAKE_PREFIX_PATH`).

## Portable Python (optional)

For fully offline installs, place embeddable Python under `engines/python/` before packaging (see `engines/python/README.md`).

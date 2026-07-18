# Etemenanki Installer — Self-Contained Build

## Overview

Etemenanki uses a **self-contained installer** — everything (TranslateGemma 4B model, Python, libraries) is bundled inside the `.exe`. User downloads one file (~2.0–2.1 GB), installs in 2 clicks, and everything works immediately.

**No post-install downloads required.** Model and Python are pre-downloaded during release preparation.

## Build Steps (Release Preparation)

### 1. Build the application

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release
```

### 2. Prepare release (downloads model + Python)

```powershell
.\scripts\prepare_release.ps1
```

This script:
- Downloads Python 3.12 embeddable → `build/Release/engines/python/`
- Installs pip dependencies (PyMuPDF, python-docx, etc.)
- Downloads embedded TranslateGemma 4B Q3_K_S (~1.8 GB) → `build/Release/engines/llm/models/`
- Sets up `tools/python_path.txt`

**Time:** ~15-30 minutes depending on internet speed.

### 3. Deploy Qt dependencies

```powershell
windeployqt build\Release\Etemenanki.exe
```

### 4. Compile installer

Open `installer\EtemenankiSetup.iss` in **Inno Setup 6** and compile.

**Output:** `installer\dist\EtemenankiSetup-1.0.5.exe` (~2.0–2.1 GB)

Or use the build script:

```powershell
.\scripts\build_installer.ps1
```

## What's Inside the Installer

| Component | Size | Purpose |
|-----------|------|---------|
| Etemenanki.exe + Qt DLLs | ~50 MB | Main application |
| `engines/python/` | ~200 MB | Python 3.12 + libraries |
| `engines/llm/models/*.gguf` | ~1.8 GB | TranslateGemma 4B Q3_K_S |
| `tools/` | ~5 MB | Python scripts |
| `assets/`, `releases/` | ~1 MB | Config, OTA updates |
| **Total** | **~2.0–2.1 GB** | **Under GitHub 2 GiB limit** |

## User Experience

1. User downloads `EtemenankiSetup-1.0.5.exe` from GitHub Releases
2. Runs installer → clicks "Next" → "Install" → "Finish"
3. Launches Etemenanki → sees welcome / status → clicks "Start"
4. Loads document → translates → saves result

**No additional downloads, no configuration, no Ollama needed.**

## Optional: pdf2zh for PDF Layout

The installer offers an optional checkbox to download **pdf2zh** (~200 MB) for layout-preserving PDF translation. This is the only post-install download.

## GitHub Updates (OTA)

1. Create release tag `v1.0.5` with asset `EtemenankiSetup-1.0.5.exe`
2. Update `releases/update.json`:
   ```json
   {
     "version_code": 105,
     "version_name": "1.0.5",
     "setup_url": "https://github.com/Baddysays/Etemenanki/releases/download/v1.0.5/EtemenankiSetup-1.0.5.exe",
     "release_notes": "TranslateGemma 4B bundled — translation-focused built-in model"
   }
   ```
3. App: **Settings → Check for updates** notifies user

## CI/CD (GitHub Actions)

See `.github/workflows/release-windows.yml`:

```yaml
- name: Build Release
  run: cmake --build build --config Release

- name: Prepare release (download model + Python)
  run: .\scripts\prepare_release.ps1

- name: Deploy Qt
  run: windeployqt build\Release\Etemenanki.exe

- name: Build installer
  run: |
    & "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\EtemenankiSetup.iss

- name: Upload artifact
  uses: actions/upload-artifact@v4
  with:
    name: installer
    path: installer\dist\EtemenankiSetup-*.exe
```

## Troubleshooting

### Model download fails in prepare_release.ps1

The script retries 3 times. If all fail:
- Check internet connection
- Try manual download: `https://huggingface.co/aoiandroid/translategemma-4b-it-GGUF/resolve/main/translategemma-4b-it.Q3_K_S.gguf`
- Place file in `build/Release/engines/llm/models/`

### Installer size too large for GitHub Releases

GitHub allows files up to **2 GiB**. The bundled quant is **Q3_K_S** (~1.8 GiB) so the full setup stays under the limit. Do not switch to Q4_K_M (~2.5 GiB) for the GitHub-hosted installer without an external host.

### User wants Ollama instead of embedded model

Settings → AI Mode → "I already use Ollama" → configure Ollama URL and models.

## File Structure

```
Etemenanki/
├── scripts/
│   ├── prepare_release.ps1      # Downloads model + Python into build/Release
│   └── build_installer.ps1      # Compiles Inno Setup installer
├── installer/
│   └── EtemenankiSetup.iss      # Inno Setup script (self-contained)
├── tools/
│   ├── embedded_llm.py          # Commands: status, serve (no download)
│   ├── install_deps.ps1         # Optional: pdf2zh only
│   └── setup_pdf2zh.ps1         # Downloads pdf2zh portable
└── engines/
    ├── llm/
    │   ├── manifest.json        # Model config (repo_id, filename, size_mb)
    │   └── models/              # GGUF model file (bundled by prepare_release.ps1)
    └── python/                  # Python 3.12 embeddable (bundled by prepare_release.ps1)
```

## Version History

| Version | Change |
|---------|--------|
| 1.0.4 | Self-contained installer — model + Python bundled |
| 1.0.3 | Post-install model download (unreliable) |
| 1.0.0 | Initial release |

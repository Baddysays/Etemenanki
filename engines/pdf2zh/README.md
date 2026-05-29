# PDFMathTranslate (pdf2zh) — portable bundle

Place the official Windows bundle here so Etemenanki runs pdf2zh without Python or servers.

## Quick setup (recommended)

From the project root:

```powershell
powershell -ExecutionPolicy Bypass -File tools/setup_pdf2zh.ps1
```

This downloads `pdf2zh-v1.9.11-with-assets-win64.zip` from
[PDFMathTranslate releases](https://github.com/Byaidu/PDFMathTranslate/releases)
and extracts it into this folder.

## Manual setup

1. Download **pdf2zh-*-with-assets-win64.zip** from GitHub releases.
2. Extract so that `pdf2zh.exe` is reachable, e.g.:
   - `engines/pdf2zh/pdf2zh.exe`, or
   - `engines/pdf2zh/pdf2zh-v1.9.11-with-assets-win64/pdf2zh.exe`
3. Rebuild or copy the `engines` folder next to `EtemenankiQt.exe`.

## Requirements

- Windows x64
- [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (usually already installed)
- Ollama for local translation (configured in Etemenanki settings)

## License

PDFMathTranslate is licensed under **AGPL-3.0**. See the upstream repository for details.

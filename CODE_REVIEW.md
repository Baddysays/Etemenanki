# Etemenanki — guide for code review

Desktop document translator (Windows). Current stack: **Qt 6.8 + C++ + QML**. Older Python/PySide code is kept for reference only.

## Review focus (active code)

| Area | Files |
|------|--------|
| App entry | `cpp/main.cpp` |
| Translation / API / file load | `cpp/translator_backend.h`, `cpp/translator_backend.cpp` |
| TXT/MD + PDF/DOCX text extraction | `cpp/document_loader.h`, `cpp/document_loader.cpp` |
| UI (mockup layout, PDF preview, thumbnails) | `qml_cpp/Main.qml` |
| PDF/DOCX extraction script | `tools/extract_document.py` |
| Build | `CMakeLists.txt`, `cpp/runtime_config.h.in` |
| Model requirements | `assets/models_catalog.json` |

## Data flow

1. User opens file → `TranslatorBackend::loadFile` → `DocumentLoader::load`.
2. TXT/MD: read in C++. PDF/DOCX: subprocess `tools/extract_document.py` (pymupdf / pypdf / PyPDF2).
3. PDF preview: `QtQuick.Pdf` in `Main.qml` (`PdfDocument` + `PdfScrollablePageView`).
4. Translate: page-by-page chunks → Ollama (`local`) or OpenAI-compatible API (`cloud`).

## Out of scope for this review

- `build/`, `.venv/`, `dist/` — generated or local env
- `src/`, `qml/` — previous PySide6 / QML prototypes
- `*.spec` — PyInstaller leftovers

## Build (reviewer optional)

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build --config Release
pip install -r requirements.txt   # for PDF/DOCX extraction
```

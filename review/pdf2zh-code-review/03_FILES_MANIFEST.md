# 03 — Манифест файлов

## Python (`source/tools/`)

| Файл | Строк (порядок) | Роль |
|------|-----------------|------|
| `pdf_engines.py` | ~366 | Адаптер: `probe`, `translate`; resolve runner; env Ollama; JSON stdout |
| `pdf2zh_runner.py` | ~35 | Entry: патч → `pdf2zh.main` |
| `ensure_pdf2zh_compat.py` | ~55 | Патч `high_level.py`: `fromstring` → `frombuffer` |
| `requirements-pdf.txt` | ~12 | pip-зависимости post-build |
| `setup_pdf2zh.ps1` | — | Скачивание portable zip в `engines/pdf2zh/` |
| `verify_pdf_pipeline.py` | ~95 | Smoke: compat, runner, probe, optional translate |
| `verify_pdf_pipeline.ps1` | — | То же для PowerShell |
| `test_pdf_engines.ps1` | — | Dev smoke test |

Связанные (не pdf2zh, но в том же build):

| `pdf_layout.py` | Встроенный движок `etemenanki` |
| `extract_document.py` | Извлечение текста для не-PDF / fallback |

## C++ (`source/cpp/`)

| Файл | Ключевые символы |
|------|------------------|
| `app_settings.h` / `.cpp` | `pdfEngine`, `ollamaBaseUrl`, `pdfEngineCatalog()`, `normalizeOllamaUrl()` |
| `document_loader.h` / `.cpp` | `probePdfEngines()`, `translatePdfExternal()`, `runPythonScript()`, `ETE_APP_DIR` |
| `translator_backend.h` / `.cpp` | `runExternalPdfEngine()`, ветка `pdfEngine != etemenanki` |

## QML (`source/qml/`)

| Файл | Содержание |
|------|------------|
| `SettingsDialog.qml` | Выбор PDF-движка, probe status, ссылки AGPL |
| `Main.qml` | Перевод / превью (использует backend) |

## Сборка (`source/build/`)

| `CMakeLists.txt.fragment` | POST_BUILD: copy tools, engines, pip, `ensure_pdf2zh_compat.py` |

## Engines (`source/engines/`)

| `THIRD_PARTY.md` | AGPL PyMuPDF + PDFMathTranslate |
| `pdf2zh/README.md` | Portable bundle instructions |

## Runtime (не в git, после сборки)

```
build/Release/
  EtemenankiQt.exe
  tools/
    python_path.txt
    pdf_engines.py
    pdf2zh_runner.py
    ensure_pdf2zh_compat.py
  engines/
    pdf2zh/          # optional portable
```

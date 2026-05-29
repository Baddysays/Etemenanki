# 02 — Архитектура

## Диаграмма вызовов (pdfmathtranslate)

```mermaid
sequenceDiagram
    participant UI as Main.qml / SettingsDialog
    participant TB as TranslatorBackend
    participant DL as DocumentLoader
    participant PY as pdf_engines.py
    participant RN as pdf2zh_runner.py
    participant PZ as pdf2zh (site-packages)
    participant OL as Ollama

    UI->>TB: translate() pdfEngine=pdfmathtranslate
    TB->>DL: translatePdfExternal(request)
    DL->>PY: subprocess python pdf_engines.py translate ...
    Note over DL,PY: ETE_APP_DIR, URL без toNativeSeparators
    PY->>PY: resolve_pdf2zh() → runner
    PY->>RN: subprocess [venv/python, pdf2zh_runner.py, pdf, -li, -lo, -s]
    RN->>RN: apply_pdf2zh_patches()
    RN->>PZ: main(argv)
    PZ->>OL: OLLAMA_HOST + ollama:model
    PZ-->>RN: writes {stem}-zh.pdf in cwd
    RN-->>PY: exit code
    PY->>PY: copy newest *-zh.pdf → output_path
    PY-->>DL: JSON {"ok": true, "output_path": "..."}
    DL-->>TB: ExternalPdfTranslateResult
    TB-->>UI: translated PDF preview
```

## Переменные окружения

| Переменная | Кто выставляет | Назначение |
|------------|----------------|------------|
| `ETE_APP_DIR` | `document_loader.cpp` → `runProcess` | Корень exe; поиск `tools/`, `engines/` |
| `OLLAMA_HOST` | `pdf_engines.py` | Базовый URL Ollama (нормализованный) |
| `OLLAMA_MODEL` | `pdf_engines.py` | Имя модели (дублирует `-s`) |
| `PYTHONIOENCODING` / `PYTHONUTF8` | C++ + Python | UTF-8 для subprocess |
| `HF_HUB_DISABLE_SYMLINKS_WARNING` | `pdf_engines.py` | Подавление предупреждения HF |

## Разрешение pdf2zh (`resolve_pdf2zh`)

Приоритет:

1. **`[sys.executable, pdf2zh_runner.py]`** — если `importlib.find_spec("pdf2zh")` OK  
2. **`engines/pdf2zh/pdf2zh.exe`** — portable bundle  
3. **`pdf2zh` в PATH** — снова runner, если доступен  
4. Иначе — недоступен (probe `available: false`)

## Пути и cwd

- **Входной PDF**: абсолютный путь в аргументе.
- **cwd subprocess pdf2zh**: каталог **выходного** PDF (`output_path.parent`).
- **Итог**: `pdf_engines.py` ищет `{input_stem}-zh.pdf` (или `-mono`, `-dual`) и копирует в `output_path`.

## Probe (статус в настройках)

```
AppSettings::probePdfEnginesStatus()
  → DocumentLoader::probePdfEngines()
  → python tools/pdf_engines.py probe --engine all
  → JSON engines.{etemenanki|pdfmathtranslate}
```

## Python interpreter

- Файл `build/Release/tools/python_path.txt` → `.venv/Scripts/python.exe` (CMake post-build).
- `DocumentLoader::resolvePythonPath()` читает его при старте.

## Критичные места для ревью

1. **`runPythonScript`**: не применять `QDir::toNativeSeparators` к `http://` URL (ломает Ollama).
2. **`normalizeOllamaUrl`**: замена `\` → `/`, префикс `http://`.
3. **`pdf_engines.normalize_ollama_url`**: дублирующая защита на стороне Python.
4. **Таймаут перевода**: 7200 с в `translatePdfExternal`.

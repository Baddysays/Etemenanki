# 05 — Тестирование

## Предусловия

- `.venv` с зависимостями (или post-build CMake).
- Ollama: `ollama serve`, модель установлена (`ollama pull translategemma:4b`).
- Сборка: `build/Release/EtemenankiQt.exe`.

## Автоматическая проверка (без GUI)

```powershell
cd C:\Users\Admin\Projects\Etemenanki
$env:ETE_APP_DIR = "$PWD\build\Release"
.\.venv\Scripts\python.exe tools\verify_pdf_pipeline.py --translate
```

Ожидание: `All checks passed.` и PDF в `%TEMP%\etemenanki_verify_out.pdf`.

## Пошагово

| Шаг | Команда | Ожидание |
|-----|---------|----------|
| Patсh | `python tools\ensure_pdf2zh_compat.py` | тишина или `patched ...` |
| Runner | `python build\Release\tools\pdf2zh_runner.py --version` | `pdf2zh v1.7.9` |
| Probe | `python build\Release\tools\pdf_engines.py probe --engine pdfmathtranslate` | `"source": "runner"`, `"available": true` |
| Translate | `pdf_engines.py translate ...` | `"ok": true` |

## Ручная проверка в UI

1. Настройки → PDF-движок: **PDFMathTranslate**.
2. Ollama URL: `http://127.0.0.1:11434`.
3. Загрузить PDF (напр. 5+ страниц).
4. EN → RU, локально, `translategemma:4b`.
5. Перевести → превью `_ru.pdf`, сохранение.

## Регрессии для ревью

- [ ] URL Ollama с `/` не ломается после передачи из C++.
- [ ] Пути к PDF с пробелами и кириллицей.
- [ ] Переключение `etemenanki` ↔ `pdfmathtranslate` без перезапуска.
- [ ] Probe в настройках после установки portable `pdf2zh.exe`.

## Логи

- `%LOCALAPPDATA%` / каталог debug Etemenanki → `python_debug.log` (если включён в `document_loader.cpp`).

# 01 — Обзор

## Цель интеграции

Подключить **PDFMathTranslate (pdf2zh)** как внешний PDF-движок с сохранением вёрстки, без встраивания AGPL-кода в C++/QML: вызов через **отдельный subprocess** (Python-скрипты рядом с exe).

## Движки в приложении

| ID | Название | Реализация |
|----|----------|------------|
| `etemenanki` | Встроенный | `tools/pdf_layout.py` + C++ пайплайн постраничного перевода |
| `pdfmathtranslate` | PDFMathTranslate | `tools/pdf_engines.py` → `pdf2zh_runner.py` → пакет `pdf2zh` |

Устаревшие ID (`polyglotpdf`, `retainpdf`) при загрузке настроек сбрасываются на `etemenanki`.

## Версии и зависимости

| Компонент | Версия / источник |
|-----------|-------------------|
| pdf2zh (pip) | 1.7.9 (`tools/requirements-pdf.txt`) |
| Portable bundle (опционально) | v1.9.11 zip, `engines/pdf2zh/` (`setup_pdf2zh.ps1`) |
| NumPy | 2.x в venv; совместимость через `ensure_pdf2zh_compat.py` + runner |
| Layout model | DocLayout-YOLO (HuggingFace, первый запуск) |
| Перевод | Ollama API (`OLLAMA_HOST`, `-s ollama:model`) |

## CLI pdf2zh 1.7.9 (важно для ревью)

- **Нет** флага `-o` для выходного файла.
- Пишет в **cwd**: `{stem}-zh.pdf`, `{stem}-dual.pdf`, `{stem}-en.pdf` (временный).
- Аргументы: `pdf`, `-li`, `-lo`, `-s` (напр. `ollama:translategemma:4b`).

## Граница ответственности

| Слой | Ответственность |
|------|-----------------|
| Qt / C++ | UI, настройки, запуск Python, пути, таймауты, превью PDF |
| `pdf_engines.py` | Выбор runner/portable, env Ollama, поиск выходного PDF, JSON для C++ |
| `pdf2zh_runner.py` | Патч NumPy, вызов `pdf2zh.main` |
| pdf2zh (upstream) | Layout YOLO, извлечение, перевод, сборка PDF |

## Не входит в scope этого PR/пакета

- PolyglotPDF, RetainPDF (удалены из UI).
- Улучшение качества вёрстки pdf2zh (наложение текста в узких блоках) — ограничение upstream.
- Коммерческая лицензия AGPL (см. `06_AGPL_AND_LICENSING.md`).

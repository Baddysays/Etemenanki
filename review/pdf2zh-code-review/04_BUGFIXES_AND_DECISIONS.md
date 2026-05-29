# 04 — Исправления и решения (контекст для ревью)

> Историческая сводка инцидентов при интеграции. Код в `source/` — финальное состояние.

## 1. CLI: `unrecognized arguments: -o`

**Симптом:** pdf2zh 1.7.9 не поддерживает `-o`.  
**Решение:** выход ищется по шаблону `{stem}-zh.pdf` в `cwd`; `shutil.copy2` в `output_path`.

## 2. NumPy 2.x: `fromstring` removed

**Симптом:** `ValueError` в `high_level.py:167`.  
**Решение:**
- `ensure_pdf2zh_compat.py` — правка site-packages на диске;
- `pdf2zh_runner.py` — `np.fromstring = np.frombuffer` до импорта pdf2zh;
- post-build CMake вызывает compat-скрипт.

## 3. Лишний `import pdf2zh.high_level` в runner

**Симптом:** падение при импорте `converter` на старте runner.  
**Решение:** убран ранний импорт; патч только через compat + NumPy.

## 4. Ollama URL: `Port could not be cast... '\\\\127.0.0.1:11434'`

**Симптом:** `QDir::toNativeSeparators` на всех args subprocess превращал `http://` в `http:\\`.  
**Решение:**
- `document_loader.cpp`: не трогать args с `http://` / `https://`;
- `normalizeOllamaUrl()` + `pdf_engines.normalize_ollama_url()`.

## 5. Приоритет portable vs runner

**Решение:** runner первым, если `pdf2zh` импортируется в venv; portable — fallback.

## 6. UI / настройки

- Убраны PolyglotPDF / RetainPDF из каталога.
- Старые `pdfEngine` ID мигрируют на `etemenanki`.

## Известные ограничения (не баги интеграции)

- Плотный/наложенный текст в узких блоках после перевода — поведение pdf2zh + длина RU-текста.
- Первый запуск: загрузка DocLayout-YOLO с HuggingFace (сеть).
- `numpy<2` в requirements не собирается на части Windows — остаётся патч под NumPy 2.x.

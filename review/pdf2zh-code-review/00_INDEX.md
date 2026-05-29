# Etemenanki — интеграция pdf2zh (PDFMathTranslate)

**Пакет для code review** · движок: `pdfmathtranslate` (pdf2zh 1.7.9) · без изменений кода в этом пакете

## Содержание

| # | Документ | Назначение |
|---|----------|------------|
| 01 | [01_OVERVIEW.md](01_OVERVIEW.md) | Цель, scope, зависимости, версии |
| 02 | [02_ARCHITECTURE.md](02_ARCHITECTURE.md) | Поток данных, компоненты, env |
| 03 | [03_FILES_MANIFEST.md](03_FILES_MANIFEST.md) | Список файлов и роли |
| 04 | [04_BUGFIXES_AND_DECISIONS.md](04_BUGFIXES_AND_DECISIONS.md) | Исправленные инциденты (контекст ревью) |
| 05 | [05_TESTING.md](05_TESTING.md) | Как проверить, smoke-тесты |
| 06 | [06_AGPL_AND_LICENSING.md](06_AGPL_AND_LICENSING.md) | AGPL, subprocess, атрибуция |
| 07 | [07_REVIEW_CHECKLIST.md](07_REVIEW_CHECKLIST.md) | Чеклист для ревьюера |

## Исходники в архиве

```
source/
  tools/          # Python-адаптер и runner
  cpp/            # C++: настройки, загрузчик, backend
  qml/            # UI настроек и перевода PDF
  build/          # фрагмент CMakeLists.txt (post-build)
  engines/        # README и THIRD_PARTY
```

## Быстрый старт для ревьюера

1. Прочитать `02_ARCHITECTURE.md` (диаграмма вызовов).
2. Пройти `07_REVIEW_CHECKLIST.md`.
3. Сверить `source/tools/pdf_engines.py` и `source/cpp/document_loader.cpp` (граница Qt ↔ Python).
4. Запустить проверку из `05_TESTING.md` (опционально).

## Статус на момент сборки пакета

- Сборка: **Release** `build/Release/EtemenankiQt.exe`
- pdf2zh: **v1.7.9** (pip), runner + патч NumPy 2.x
- LLM: **Ollama** `http://127.0.0.1:11434`, модель из настроек (напр. `translategemma:4b`)
- Проверено: перевод многостраничного PDF (AUP, 5 стр.) — OK

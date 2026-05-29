# 07 — Чеклист code review

## Безопасность и процессы

- [ ] Subprocess: нет shell=True; аргументы списком.
- [ ] Таймауты на длительный перевод (2 ч).
- [ ] Секреты (cloud API) не логируются в `python_debug.log`.
- [ ] Пути пользователя не интерполируются в shell-команды.

## Корректность

- [ ] `http://` URL не проходят `toNativeSeparators` (`document_loader.cpp`).
- [ ] Выход pdf2zh находится после run (`newest_pdf`, `-zh.pdf`).
- [ ] `ETE_APP_DIR` задан при запуске из exe.
- [ ] Ошибки pdf2zh попадают в UI (JSON `error`, stderr до 2000 символов).

## Архитектура

- [ ] Разделение: C++ orchestration vs Python adapter vs upstream pdf2zh.
- [ ] Runner предпочтительнее portable при наличии venv.
- [ ] Fallback `etemenanki` не затронут регрессией.

## Зависимости

- [ ] `requirements-pdf.txt` согласован с post-build pip.
- [ ] `ensure_pdf2zh_compat.py` идемпотентен.
- [ ] Документирована зависимость от Ollama и HF при первом run.

## UI/UX

- [ ] Каталог движков: 2 пункта, понятные описания.
- [ ] Probe отображает `source: runner` / portable.
- [ ] Ссылка на upstream PDFMathTranslate.

## Тесты

- [ ] `verify_pdf_pipeline.py --translate` проходит в CI/dev.
- [ ] Ручной сценарий 5+ стр. PDF задокументирован.

## Вопросы к автору PR (шаблон)

1. Нужна ли поддержка cloud OpenAI для pdf2zh в UI (сейчас env в `pdf_engines.py` есть)?
2. План по AGPL source offer в инсталляторе?
3. Остаёмся на pdf2zh 1.7.9 или миграция на portable 1.9.x?

# Etemenanki — для пользователя

## Одна установка

1. **Скачайте** с [GitHub Releases](https://github.com/Baddysays/Etemenanki/releases/latest):
   - [`etemenanki-setup.exe`](https://github.com/Baddysays/Etemenanki/releases/download/v1.0.3/etemenanki-setup.exe) — установщик (рекомендуется)
   - или [`etemenanki-portable.zip`](https://github.com/Baddysays/Etemenanki/releases/download/v1.0.3/etemenanki-portable.zip) — распаковать и запустить `Etemenanki.exe`

2. **Ollama** (для локального ИИ): [ollama.com/download](https://ollama.com/download/windows)  
   После установки в терминале: `ollama serve` (или из трея).

3. **Первый запуск** — мастер настройки:
   - сканирует ПК и предлагает модели;
   - может скачать pdf2zh и Python-библиотеки;
   - выполнит `ollama pull` для выбранных моделей.

4. **Перевод**: «Загрузить файл» → языки → «Перевести» → «Сохранить».

## Обновления

Как в проекте Saylat: приложение читает  
`https://github.com/Baddysays/Etemenanki/blob/main/releases/update.json`

В Etemenanki: **Настройки → Общие → Обновления → Проверить обновления**.

## Две видеокарты (встроенная и дискретная)

Если перевод **очень долгий** или Ollama «висит»:

1. В мастере настройки или в **Настройки → Ollama** выберите **дискретную** видеокарту (NVIDIA GeForce / RTX), не Intel UHD / «Basic Display».
2. Откройте **Параметры Windows → Система → Дисплей → Графика** и для `ollama.exe` укажите **Высокая производительность** (дискретная GPU).
3. Кнопка **GPU для Ollama (Windows)** в тех же настройках откроет этот раздел Windows.

## Режимы ИИ

| Режим | Для кого |
|-------|----------|
| **Автоматически** | Ollama ставится/запускается сам — проще всего |
| **Свой Ollama** | Вы сами держите `ollama serve` |
| **Встроенная модель** | Без Ollama: один раз скачать ~1,7 ГБ в мастере, дальше «Перевести» |

В мастере настройки или **Настройки → Ollama → Режим ИИ** выберите нужный вариант.

## Если что-то не работает

| Симптом | Что сделать |
|---------|-------------|
| Нет локальных моделей | Установите Ollama, `ollama pull translategemma:4b`, в настройках «Обновить список» |
| PDF без текста | Установите pymupdf: `pip install pymupdf` или пройдите мастер настройки |
| PDF с вёрсткой | В мастере включите pdf2zh; или `tools/setup_pdf2zh.ps1` |
| Облачный перевод | Настройки → Облако API — ключ и URL |

Почта: [hello@baddysays.ru](mailto:hello@baddysays.ru) · Telegram: [@baddysays](https://t.me/baddysays)

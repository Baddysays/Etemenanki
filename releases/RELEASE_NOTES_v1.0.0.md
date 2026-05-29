## Etemenanki 1.0.0 (build 100)

**Первый публичный релиз: перевод документов локально (Ollama) и в облаке, мастер настройки «из коробки», обновления через GitHub.**

### Что умеет

- Перевод **PDF, DOCX, TXT, MD**, таблиц, субтитров и других форматов
- **Локально** — модели через [Ollama](https://ollama.com) (`translategemma:4b`, `qwen2.5:7b` и др.)
- **В облаке** — OpenAI-compatible API (DeepSeek и др., ключ в настройках)
- **PDF с вёрсткой** — движок PDFMathTranslate (pdf2zh), опционально при первой настройке
- Интерфейс на **10 языках** (RU, EN, DE, FR, ES, UK, ZH, PT, EL, LA)

### Мастер первого запуска

1. Сканирует ПК (ОЗУ / VRAM) и **рекомендует модели** под ваше железо  
2. Предлагает скачать **pdf2zh** и Python-библиотеки для извлечения текста  
3. Запускает `ollama pull` для выбранных моделей  

Установите [Ollama для Windows](https://ollama.com/download/windows) и выполните `ollama serve` — один раз.

### Скачать

| Файл | Для кого |
|------|----------|
| **etemenanki-setup.exe** | Обычная установка в `Program Files` |
| **etemenanki-portable.zip** | Без установщика — распаковать и запустить `Etemenanki.exe` |

Обновления в приложении: **Настройки → Обновления → Проверить обновления** (как в [Saylat](https://github.com/Baddysays/Saylat)).

### Быстрый старт

1. Скачайте и установите релиз (или portable ZIP)  
2. Установите Ollama, при необходимости: `ollama pull translategemma:4b`  
3. Запустите Etemenanki → пройдите мастер настройки  
4. **Загрузить файл** → выберите языки → **Перевести** → **Сохранить**  

Подробнее: [docs/DLYA-POLZOVATELYA.md](https://github.com/Baddysays/Etemenanki/blob/main/docs/DLYA-POLZOVATELYA.md)

### Системные требования (ориентир)

- Windows 10/11 x64  
- 8+ GB RAM для `translategemma:4b`, 16+ GB для `translategemma:12b`  
- Для PDF с вёрсткой — дополнительно ~2 GB на pdf2zh при первой загрузке  

### Известные ограничения

- Ollama **не входит** в установщик — ставится отдельно  
- Сканы PDF без текстового слоя требуют OCR (текст может не извлечься)  
- PyMuPDF (AGPL) — учитывайте лицензию при распространении сборки с встроенным Python  

### Обратная связь

- [Issues](https://github.com/Baddysays/Etemenanki/issues) — ошибки  
- [Discussions](https://github.com/Baddysays/Etemenanki/discussions) — вопросы и идеи  
- ✉️ [hello@baddysays.ru](mailto:hello@baddysays.ru) · 💬 [@baddysays](https://t.me/baddysays)

---

*Нет легкого пути от земли к звездам.*

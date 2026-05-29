<p align="center">
  <img src="assets/branding/logo-header.png" width="280" alt="Etemenanki — AI document translator" />
</p>

# Etemenanki — AI-переводчик документов

[![GitHub release](https://img.shields.io/github/v/release/Baddysays/Etemenanki)](https://github.com/Baddysays/Etemenanki/releases)
[![CI](https://github.com/Baddysays/Etemenanki/actions/workflows/ci.yml/badge.svg)](https://github.com/Baddysays/Etemenanki/actions/workflows/ci.yml)
[![Release Windows](https://github.com/Baddysays/Etemenanki/actions/workflows/release-windows.yml/badge.svg)](https://github.com/Baddysays/Etemenanki/actions/workflows/release-windows.yml)

**Нет легкого пути от земли к звездам** — перевод PDF, DOCX, TXT и других форматов с локальными моделями (Ollama) или облаком.

*by **baddysays*** · ✉️ [hello@baddysays.ru](mailto:hello@baddysays.ru) · 💬 [@baddysays](https://t.me/baddysays)

## Что это

Etemenanki — настольное приложение (Windows) на **Qt 6.8 + QML**:

- загрузка документа и извлечение текста (Python / PyMuPDF);
- перевод через **Ollama** локально или **OpenAI-compatible API** в облаке;
- сохранение результата (TXT, DOCX, PDF с вёрсткой — где поддерживается);
- мастер первого запуска: подбор моделей под ваш ПК, докачка pdf2zh и зависимостей.

## Скачать

| Способ | Ссылка |
|--------|--------|
| Установщик | [etemenanki-setup.exe](https://github.com/Baddysays/Etemenanki/releases/latest/download/etemenanki-setup.exe) |
| Портативная сборка | [etemenanki-portable.zip](https://github.com/Baddysays/Etemenanki/releases/latest/download/etemenanki-portable.zip) |
| Все релизы | [GitHub Releases](https://github.com/Baddysays/Etemenanki/releases) |

Обновления в приложении: **Настройки → Обновления → Проверить** (как в [Saylat](https://github.com/Baddysays/Saylat), через `releases/update.json` на GitHub).

## Быстрый старт (пользователь)

1. Скачайте [последний релиз](https://github.com/Baddysays/Etemenanki/releases/latest).
2. Установите [Ollama](https://ollama.com/download/windows) и запустите `ollama serve`.
3. Запустите Etemenanki — откроется **мастер настройки** (модели, pdf2zh, Python).
4. Загрузите файл → выберите языки → **Перевести** → **Сохранить**.

Подробнее: [docs/DLYA-POLZOVATELYA.md](docs/DLYA-POLZOVATELYA.md) · [docs/INSTALLER.md](docs/INSTALLER.md)

## Рекомендуемые локальные модели

| Профиль ПК | Модели |
|------------|--------|
| Слабый (≤11 GB RAM) | `translategemma:4b` |
| Средний | `translategemma:4b`, `qwen2.5:7b` |
| Мощный | `translategemma:12b` |

```powershell
ollama pull translategemma:4b
```

Каталог и рекомендации: `assets/models_catalog.json`, мастер настройки в приложении.

## Сборка из исходников

Требуется **Qt 6.8** (msvc2022_64), CMake 3.21+, Visual Studio 2022.

```powershell
git clone https://github.com/Baddysays/Etemenanki.git
cd Etemenanki
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r tools/requirements-pdf.txt

cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release
.\build\Release\EtemenankiQt.exe
```

Установщик (Inno Setup 6): `installer/EtemenankiSetup.iss`

## Возможности

- Локальный перевод (Ollama) и облако (DeepSeek, OpenAI-compatible)
- PDF / DOCX / TXT / MD / XLSX / субтитры и др.
- Интерфейс: RU, EN, DE, FR, ES, UK, ZH, PT, EL, LA
- PDFMathTranslate (pdf2zh) для PDF с формулами и вёрсткой

## Обратная связь

- 🐛 [Issues](https://github.com/Baddysays/Etemenanki/issues)
- 💬 [Discussions](https://github.com/Baddysays/Etemenanki/discussions)
- 🔧 Pull requests приветствуются

## Лицензия

См. репозиторий. PyMuPDF (AGPL) — учитывайте при распространении сборки с встроенным Python.

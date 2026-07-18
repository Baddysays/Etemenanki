<p align="center">
  <img src="assets/branding/logo-header.png" width="280" alt="Etemenanki — AI document translator" />
</p>

# Etemenanki — AI-переводчик документов

[![GitHub release](https://img.shields.io/github/v/release/Baddysays/Etemenanki)](https://github.com/Baddysays/Etemenanki/releases)
[![CI](https://github.com/Baddysays/Etemenanki/actions/workflows/ci.yml/badge.svg)](https://github.com/Baddysays/Etemenanki/actions/workflows/ci.yml)
[![Release Windows](https://github.com/Baddysays/Etemenanki/actions/workflows/release-windows.yml/badge.svg)](https://github.com/Baddysays/Etemenanki/actions/workflows/release-windows.yml)

**Нет легкого пути от земли к звездам** — перевод PDF, DOCX, TXT и других форматов. В установщике уже есть **TranslateGemma 4B** (без Ollama); при желании — Ollama или облако.

*by **baddysays*** · ✉️ [hello@baddysays.ru](mailto:hello@baddysays.ru) · 💬 [@baddysays](https://t.me/baddysays)

## Что это

Etemenanki — настольное приложение (Windows) на **Qt 6.8 + QML**:

- загрузка документа и извлечение текста (Python / PyMuPDF);
- перевод через **встроенную TranslateGemma**, **Ollama** или **облачный API**;
- сохранение результата (TXT, DOCX, PDF с вёрсткой — где поддерживается);
- простой первый запуск: статус компонентов → **Начать**.

## Скачать

| Способ | Ссылка |
|--------|--------|
| Установщик (всё включено) | [EtemenankiSetup](https://github.com/Baddysays/Etemenanki/releases/latest) |
| Портативная сборка | [etemenanki-portable.zip](https://github.com/Baddysays/Etemenanki/releases/latest/download/etemenanki-portable.zip) |
| Все релизы | [GitHub Releases](https://github.com/Baddysays/Etemenanki/releases) |

Обновления в приложении: **Настройки → Обновления → Проверить** (как в [Saylat](https://github.com/Baddysays/Saylat), через `releases/update.json` на GitHub).

## Быстрый старт (пользователь)

1. Скачайте [установщик](https://github.com/Baddysays/Etemenanki/releases/latest) (~2 ГБ — модель уже внутри).
2. Установите и откройте → **Начать**.
3. Загрузите файл → языки → **Перевести** → **Сохранить**.

Ollama **не обязательна**. Для более крупных моделей через Ollama см. таблицу ниже.

Подробнее: [docs/DLYA-POLZOVATELYA.md](docs/DLYA-POLZOVATELYA.md) · [docs/INSTALLER.md](docs/INSTALLER.md)

## Модели

| Профиль | Что использовать |
|---------|------------------|
| По умолчанию (установщик) | **TranslateGemma 4B** встроенная |
| Ollama, слабый ПК | `translategemma:4b` |
| Ollama, мощный ПК | `translategemma:12b` |

```powershell
ollama pull translategemma:4b
```

Каталог: `assets/models_catalog.json`.

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
.\build\Release\Etemenanki.exe
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

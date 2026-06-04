#!/usr/bin/env python3
"""Generate status/error UI strings for app_ui_strings_status.inc"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "cpp" / "app_ui_strings_status.inc"


def esc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def put(key, ru, en, de=None, fr=None, es=None, uk=None, zh=None, pt=None, el=None, la=None):
    de = de or en
    fr = fr or en
    es = es or en
    uk = uk or ru
    zh = zh or en
    pt = pt or en
    el = el or en
    la = la or en
    vals = [ru, en, de, fr, es, uk, zh, pt, el, la]
    parts = [f'    put(m, "{key}"']
    for v in vals:
        parts.append(f',\n        "{esc(v)}"')
    parts.append(");\n")
    return "".join(parts)


# (key, ru, en) — other UI langs fall back to en via text()
STATUS = [
    ("status_python_path", "Python: %1", "Python: %1"),
    ("status_pdf_copy_failed_after_translate",
     "Перевод готов; PDF: не удалось подготовить копию файла",
     "Translation done; PDF: could not prepare working copy"),
    ("status_building_translated_pdf", "Сборка переведённого PDF...", "Building translated PDF..."),
    ("status_translate_done_pdf_pages", "Перевод завершен — PDF готов (%1 стр.)",
     "Translation complete — PDF ready (%1 pages)"),
    ("status_translate_done_pdf_not_built", "Перевод готов, но PDF не собран",
     "Translation done, but PDF was not built"),
    ("status_translate_done_pdf_error", "Перевод готов; PDF: %1", "Translation done; PDF: %1"),
    ("status_file_not_found", "Файл не найден: %1", "File not found: %1"),
    ("status_loading_extracting", "Загрузка и извлечение текста...", "Loading and extracting text..."),
    ("status_no_text_in_doc", "Текст не найден в документе (скан без текстового слоя?)",
     "No text found in document (scan without text layer?)"),
    ("status_file_loaded", "%1 — %2 (%3 стр.)%4", "%1 — %2 (%3 pages)%4"),
    ("status_mode_pdf_layout", " [PDF layout]", " [PDF layout]"),
    ("status_mode_structure", " [структура]", " [structure]"),
    ("status_mode_segments", " [сегменты]", " [segments]"),
    ("status_pdf_preview_no_text",
     "Превью PDF доступно; текст для перевода не извлечён.",
     "PDF preview available; text was not extracted for translation."),
    ("status_pdf_copy_warning", " (копия не создана — см. %1)", " (copy not created — see %1)"),
    ("status_encoding_suffix", " (%1)", " (%1)"),
    ("status_no_save_file", "Не выбран файл для сохранения", "No file selected for saving"),
    ("status_pdf_saved", "PDF сохранён: %1", "PDF saved: %1"),
    ("status_pdf_save_failed", "Не удалось сохранить PDF", "Failed to save PDF"),
    ("status_file_saved", "Файл сохранён: %1", "File saved: %1"),
    ("status_export_fallback", "Экспорт: %1 — сохраняю как TXT", "Export: %1 — saving as TXT"),
    ("status_file_save_failed", "Не удалось сохранить файл", "Failed to save file"),
    ("status_cancel_pdf", "Отмена PDF…", "Cancelling PDF…"),
    ("status_translate_cancelled", "Перевод отменен", "Translation cancelled"),
    ("status_no_extracted_text",
     "Текст не извлечён — загрузите файл снова или установите pymupdf: pip install pymupdf",
     "Text not extracted — reload the file or install pymupdf: pip install pymupdf"),
    ("status_no_text_translate", "Нет текста для перевода", "No text to translate"),
    ("status_check_embedded", "Проверка встроенной модели...", "Checking built-in model..."),
    ("status_embedded_model_missing",
     "Встроенная модель не скачана — откройте мастер настройки или Настройки → Скачать модель",
     "Built-in model not downloaded — run setup wizard or Settings → Download model"),
    ("status_start_embedded",
     "Запуск встроенной модели…",
     "Starting built-in model…"),
    ("status_embedded_loading_model",
     "Загрузка встроенной модели %1… первый запрос может занять несколько минут",
     "Loading built-in model %1… first request may take a few minutes"),
    ("status_embedded_empty_reply",
     "Пустой ответ встроенной модели — перезапустите в Настройках",
     "Built-in model returned empty text — restart from Settings"),
    ("status_check_ollama", "Проверка Ollama...", "Checking Ollama..."),
    ("status_start_ollama", "Запустите Ollama: ollama serve", "Start Ollama: ollama serve"),
    ("status_ollama_model_missing",
     "Модель %1 не найдена в Ollama — выполните: ollama pull %1",
     "Model %1 not found in Ollama — run: ollama pull %1"),
    ("status_ollama_loading_model",
     "Ollama загружает модель %1… первый запрос может занять 2–10 мин",
     "Ollama is loading model %1… first request may take 2–10 min"),
    ("status_ollama_request_timeout",
     "Таймаут Ollama (%1). Модель не ответила — проверьте ollama pull и VRAM",
     "Ollama timeout (%1). Model did not respond — check ollama pull and VRAM"),
    ("status_ollama_empty_reply",
     "Ollama вернул пустой ответ — перезапустите ollama serve или смените модель",
     "Ollama returned empty text — restart ollama serve or try another model"),
    ("status_cloud_api_key", "Укажите API Key для облачного перевода",
     "Set API key for cloud translation"),
    ("status_pdf_extract_layout", "Извлечение структуры PDF (таблицы, лого)...",
     "Extracting PDF structure (tables, logos)..."),
    ("status_pdf_copy_layout_failed",
     "PDF: не удалось подготовить копию для извлечения структуры",
     "PDF: could not prepare copy for structure extraction"),
    ("status_pdf_layout_error", "PDF layout: %1", "PDF layout: %1"),
    ("status_pdf_engine_running", "PDF: %1…", "PDF: %1…"),
    ("status_pdf_copy_failed", "PDF: не удалось подготовить копию файла",
     "PDF: could not prepare working copy"),
    ("status_pdf_ready_engine", "PDF готов — %1", "PDF ready — %1"),
    ("status_external_pdf_failed", "Внешний PDF-движок завершился с ошибкой",
     "External PDF engine finished with an error"),
    ("status_pdf_translated_body", "PDF переведён движком %1. Откройте \"Исходный формат\".",
     "PDF translated by %1. Open \"Original format\"."),
    ("status_translate_pages_progress", "Перевод %1 стр...", "Translating %1 pages..."),
    ("status_translate_done_pages", "Перевод завершен (%1 стр.)", "Translation complete (%1 pages)"),
    ("status_network_error", "Ошибка сети: %1", "Network error: %1"),
    ("status_translate_progress", "Перевод: %1% (стр. %2/%3)",
     "Translating: %1% (page %2/%3)"),
]

# DocumentLoader / backend errors shown via setStatus(doc.error)
ERRORS = [
    ("err_open_file", "Не удалось открыть файл", "Failed to open file"),
    ("err_python_extract", "Не удалось запустить Python для извлечения текста",
     "Failed to start Python for text extraction"),
    ("err_extract_code", "Ошибка извлечения текста (код %1)", "Text extraction error (code %1)"),
    ("err_extract_bad_json", "Некорректный ответ скрипта извлечения", "Invalid extraction script response"),
    ("err_extract_no_text", "Не удалось извлечь текст из документа", "Failed to extract text from document"),
    ("err_no_extract_script", "Не найден tools/extract_document.py", "tools/extract_document.py not found"),
    ("err_install_python_deps", "Установите Python 3 и PyPDF2, python-docx",
     "Install Python 3 and PyPDF2, python-docx"),
    ("err_pdf_no_text_layer", "Нет текстового слоя в PDF (скан без OCR)",
     "No text layer in PDF (scan without OCR)"),
    ("err_unsupported_format", "Неподдерживаемый формат: .", "Unsupported format: ."),
    ("err_pdf_layout_script", "Не найден tools/pdf_layout.py", "tools/pdf_layout.py not found"),
    ("err_pdf_copy_failed", "Не удалось скопировать PDF (закройте файл в других программах)",
     "Failed to copy PDF (close the file in other programs)"),
    ("err_pdf_layout_run", "Не удалось запустить pdf_layout.py", "Failed to run pdf_layout.py"),
    ("err_pdf_layout_failed", "Ошибка pdf_layout.py", "pdf_layout.py error"),
    ("err_pdf_layout_json", "Некорректный JSON от extract: %1. Лог: %2",
     "Invalid JSON from extract: %1. Log: %2"),
    ("err_pdf_layout_extract", "Не удалось извлечь layout PDF", "Failed to extract PDF layout"),
    ("err_pdf_build_copy", "Не удалось скопировать PDF для сборки", "Failed to copy PDF for build"),
    ("err_pdf_structure_extract", "Не удалось извлечь структуру PDF", "Failed to extract PDF structure"),
    ("err_pdf_build_layout_tmp", "Не удалось создать временный layout",
     "Failed to create temporary layout"),
    ("err_pdf_build_failed", "Ошибка сборки PDF", "PDF build error"),
    ("err_pdf_build_json", "Некорректный JSON от build: %1. Лог: %2",
     "Invalid JSON from build: %1. Log: %2"),
    ("err_export_script", "Не найден tools/export_document.py", "tools/export_document.py not found"),
    ("err_export_meta", "Не удалось подготовить метаданные экспорта",
     "Failed to prepare export metadata"),
    ("err_export_python", "Не удалось запустить Python для экспорта",
     "Failed to start Python for export"),
    ("err_export_failed", "Ошибка экспорта документа", "Document export error"),
    ("err_export_code", "Ошибка экспорта (код %1)", "Export error (code %1)"),
    ("err_export_no_file", "Экспорт не создал файл", "Export did not create a file"),
    ("err_ext_pdf_start", "Не удалось запустить внешний PDF-движок",
     "Failed to start external PDF engine"),
    ("err_ext_pdf_engine", "Внешний PDF-движок", "External PDF engine"),
    ("err_ext_pdf_no_output", "Внешний движок не создал выходной PDF",
     "External engine did not create output PDF"),
    ("err_builtin_pdf_pipeline", "Встроенный движок использует стандартный пайплайн Etemenanki",
     "Built-in engine uses the standard Etemenanki pipeline"),
    ("err_ext_pdf_start_fmt", "Не удалось запустить внешний PDF-движок: %1",
     "Failed to start external PDF engine: %1"),
    ("err_probe_pdf_engines", "Не удалось запустить pdf_engines.py probe",
     "Failed to run pdf_engines.py probe"),
]

lines = ["static void addStatusUiStrings(KeyTable& m)\n{\n"]
for item in STATUS:
    lines.append(put(*item))
for item in ERRORS:
    lines.append(put(*item))
lines.append("}\n")

OUT.write_text("".join(lines), encoding="utf-8")
print(f"Wrote {OUT} ({len(STATUS)} status + {len(ERRORS)} error keys)")

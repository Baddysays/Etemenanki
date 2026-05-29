#!/usr/bin/env python3
"""Generate addMainUiStrings() body for app_ui_strings.cpp"""
from pathlib import Path

# ru, en, de, fr, es, uk, zh, pt, el, la
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
        esc = v.replace("\\", "\\\\").replace('"', '\\"')
        parts.append(f',\n        "{esc}"')
    parts.append(");\n")
    return "".join(parts)


entries = [
    ("main_tagline", "Нет легкого пути от земли к звездам", "No easy way from earth to the stars",
     "Kein leichter Weg von der Erde zu den Sternen", "Pas de chemin facile de la Terre aux étoiles",
     "No hay camino fácil de la Tierra a las estrellas", "Немає легкого шляху від землі до зірок",
     "从大地到星辰，没有轻松的路", "Não há caminho fácil da Terra às estrelas",
     "Δεν υπάρχει εύκολος δρόμος από τη γη στα άστρα", "Nulla via facilis a terra ad sidera"),
    ("main_brand_tooltip", "Etemenanki — Нет легкого пути от земли к звездам",
     "Etemenanki — No easy way from earth to the stars"),
    ("main_settings_tooltip", "Настройки", "Settings", "Einstellungen", "Paramètres", "Ajustes",
     "Налаштування", "设置", "Configurações", "Ρυθμίσεις", "Optiones"),
    ("main_source_panel", "Просмотр исходного файла", "Source file preview",
     "Quelldatei-Vorschau", "Aperçu du fichier source", "Vista previa del archivo de origen",
     "Перегляд вихідного файлу", "源文件预览", "Pré-visualização do ficheiro de origem",
     "Προεπισκόπηση αρχείου πηγής", "Praevisio fasciculi fontis"),
    ("main_result_panel", "Просмотр переведённого файла", "Translated file preview",
     "Übersetzungsvorschau", "Aperçu du fichier traduit", "Vista previa del archivo traducido",
     "Перегляд перекладеного файлу", "译文预览", "Pré-visualização do ficheiro traduzido",
     "Προεπισκόπηση μεταφρασμένου αρχείου", "Praevisio fasciculi translati"),
    ("main_no_file", "файл не выбран", "no file selected", "keine Datei gewählt", "aucun fichier",
     "ningún archivo", "файл не обрано", "未选择文件", "nenhum ficheiro", "κανένα αρχείο", "fasciculum non electum"),
    ("main_source_placeholder", "Выберите файл или вставьте текст", "Choose a file or paste text",
     "Datei wählen oder Text einfügen", "Choisir un fichier ou coller du texte",
     "Elija un archivo o pegue texto", "Оберіть файл або вставте текст",
     "选择文件或粘贴文本", "Escolha um ficheiro ou cole texto",
     "Επίλεξε αρχείο ή επικόλλησε κείμενο", "Elige fasciculum vel adglutina textum"),
    ("main_pdf_building", "Сборка PDF с сохранением формата...", "Building PDF with layout preserved...",
     "PDF wird mit Layout erstellt...", "Création du PDF avec mise en page...",
     "Generando PDF con diseño...", "Збірка PDF із збереженням формату...",
     "正在生成保留版式的 PDF...", "A gerar PDF com layout...",
     "Δημιουργία PDF με διάταξη...", "Componitur PDF cum compositione..."),
    ("main_pdf_failed",
     "PDF не собран. Проверьте Python в tools/python_path.txt\\nили нажмите \"Исходный формат\" ещё раз.",
     "PDF was not built. Check Python in tools/python_path.txt\\nor click \"Original format\" again."),
    ("main_format_pending", "Форматированный вид появится после перевода",
     "Formatted view will appear after translation"),
    ("main_result_placeholder",
     "Здесь появится перевод после нажатия \"Перевести\"",
     "Translation will appear here after you click \"Translate\""),
    ("main_format_tab", "Исходный формат (%1)", "Original format (%1)", "Originalformat (%1)",
     "Format d'origine (%1)", "Formato original (%1)", "Вихідний формат (%1)", "原始格式 (%1)",
     "Formato original (%1)", "Αρχική μορφή (%1)", "Forma originalis (%1)"),
    ("main_text_tab", "Переведённый текст", "Translated text", "Übersetzter Text", "Texte traduit",
     "Texto traducido", "Перекладений текст", "译文", "Texto traduzido", "Μεταφρασμένο κείμενο", "Textus translatus"),
    ("main_stat_speed", "Скорость", "Speed", "Geschwindigkeit", "Vitesse", "Velocidad", "Швидкість", "速度", "Velocidade", "Ταχύτητα", "Celeritas"),
    ("main_stat_quality", "Качество", "Quality", "Qualität", "Qualité", "Calidad", "Якість", "质量", "Qualidade", "Ποιότητα", "Qualitas"),
    ("main_done", "Готово", "Done", "Fertig", "Terminé", "Listo", "Готово", "完成", "Concluído", "Έτοιμο", "Factum"),
    ("main_elapsed", "Прошло:", "Elapsed:", "Vergangen:", "Écoulé :", "Transcurrido:", "Минуло:", "已用:", "Decorrido:", "Πέρασαν:", "Transactum:"),
    ("main_remaining_none", "Осталось: —", "Remaining: —"),
    ("main_remaining_unknown", "Осталось: …", "Remaining: …"),
    ("main_remaining", "Осталось: ~", "Remaining: ~"),
    ("main_pages_unit", "стр.", "pages", "S.", "p.", "pág.", "стор.", "页", "pág.", "σελ.", "pag."),
    ("main_file_type", "файл", "file", "Datei", "fichier", "archivo", "файл", "文件", "ficheiro", "αρχείο", "fasciculum"),
    ("dialog_open_title", "Загрузить файл", "Open file"),
    ("dialog_save_title", "Сохранить перевод", "Save translation"),
    ("dialog_save_accept", "Сохранить", "Save"),
    ("main_quality_high", "Высокое", "High", "Hoch", "Élevée", "Alta", "Високе", "高", "Alta", "Υψηλή", "Alta"),
    ("main_quality_very_high", "Очень высокое", "Very high", "Sehr hoch", "Très élevée", "Muy alta",
     "Дуже високе", "很高", "Muito alta", "Πολύ υψηλή", "Altissima"),
    ("main_speed_tokens", "%1 ток/с", "%1 tok/s"),
    ("hw_ram_usage", "RAM: %1 / %2 GB", "RAM: %1 / %2 GB"),
    ("hw_ram_unknown", "RAM: —", "RAM: —"),
    ("hw_vram_usage", "VRAM: %1 / %2 GB", "VRAM: %1 / %2 GB"),
    ("hw_vram_unknown", "VRAM: —", "VRAM: —"),
    ("hw_vram_required", "VRAM: треб. ~%1 GB", "VRAM: req. ~%1 GB", "VRAM: ca. ~%1 GB", "VRAM : env. ~%1 Go",
     "VRAM: aprox. ~%1 GB", "VRAM: потр. ~%1 GB", "VRAM：约 ~%1 GB", "VRAM: aprox. ~%1 GB",
     "VRAM: απαιτ. ~%1 GB", "VRAM: req. ~%1 GB"),
    ("status_ready", "Готово — центр перевода: PDF, DOCX, SRT, XLSX, JSON…",
     "Ready — translation hub: PDF, DOCX, SRT, XLSX, JSON…"),
    ("status_check_python", "Проверка Python...", "Checking Python..."),
]

out = []
out.append("static void addMainUiStrings(KeyTable& m)\n{\n")
for e in entries:
    out.append(put(*e))
out.append("}\n")

target = Path(__file__).resolve().parents[1] / "cpp" / "app_ui_strings_main.inc"
target.write_text("".join(out), encoding="utf-8")
print(f"Wrote {target}")

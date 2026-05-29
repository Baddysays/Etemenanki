#include "app_ui_strings.h"

#include <QVariantMap>

namespace AppUiStrings {

namespace {

void put(KeyTable& m,
         const char* key,
         const char* ru,
         const char* en,
         const char* de,
         const char* fr,
         const char* es,
         const char* uk,
         const char* zh,
         const char* pt,
         const char* el,
         const char* la)
{
    m.insert(QString::fromUtf8(key),
             LangTable{{QStringLiteral("ru"), QString::fromUtf8(ru)},
                       {QStringLiteral("en"), QString::fromUtf8(en)},
                       {QStringLiteral("de"), QString::fromUtf8(de)},
                       {QStringLiteral("fr"), QString::fromUtf8(fr)},
                       {QStringLiteral("es"), QString::fromUtf8(es)},
                       {QStringLiteral("uk"), QString::fromUtf8(uk)},
                       {QStringLiteral("zh"), QString::fromUtf8(zh)},
                       {QStringLiteral("pt"), QString::fromUtf8(pt)},
                       {QStringLiteral("el"), QString::fromUtf8(el)},
                       {QStringLiteral("la"), QString::fromUtf8(la)}});
}

#include "app_ui_strings_main.inc"
#include "app_ui_strings_status.inc"

KeyTable build()
{
    KeyTable m;
    put(m, "lang_auto", "Авто", "Auto", "Auto", "Auto", "Auto", "Авто", "自动", "Auto", "Αυτόματα", "Automatice");
    put(m, "load_file", "Загрузить файл", "Open file", "Datei öffnen", "Ouvrir un fichier", "Abrir archivo",
        "Відкрити файл", "打开文件", "Abrir arquivo", "Άνοιγμα αρχείου", "Fasciculum aperire");
    put(m, "translate", "Перевести", "Translate", "Übersetzen", "Traduire", "Traducir", "Перекласти", "翻译",
        "Traduzir", "Μετάφραση", "Transferre");
    put(m, "save", "Сохранить", "Save", "Speichern", "Enregistrer", "Guardar", "Зберегти", "保存", "Salvar",
        "Αποθήκευση", "Servare");
    put(m, "cancel", "Отмена", "Cancel", "Abbrechen", "Annuler", "Cancelar", "Скасувати", "取消", "Cancelar",
        "Ακύρωση", "Intermittere");
    put(m, "settings", "Настройки", "Settings", "Einstellungen", "Paramètres", "Ajustes", "Налаштування", "设置",
        "Configurações", "Ρυθμίσεις", "Optiones");
    put(m, "help", "Справка", "Help", "Hilfe", "Aide", "Ayuda", "Довідка", "帮助", "Ajuda", "Βοήθεια", "Auxilium");
    put(m, "source_lang", "Исходный язык", "Source language", "Quellsprache", "Langue source", "Idioma de origen",
        "Мова джерела", "源语言", "Idioma de origem", "Γλώσσα πηγής", "Lingua fontis");
    put(m, "target_lang", "Язык перевода", "Target language", "Zielsprache", "Langue cible", "Idioma de destino",
        "Мова перекладу", "目标语言", "Idioma de destino", "Γλώσσα στόχου", "Lingua finis");
    put(m, "local", "Локально", "Local", "Lokal", "Local", "Local", "Локально", "本地", "Local", "Τοπικά",
        "Loci");
    put(m, "cloud", "Облако", "Cloud", "Cloud", "Cloud", "Nube", "Хмара", "云端", "Nuvem", "Νέφος", "Nubes");
    put(m, "subtitle", "AI-переводчик документов", "AI document translator", "KI-Dokumentenübersetzer",
        "Traducteur de documents IA", "Traductor de documentos con IA", "AI-перекладач документів", "AI 文档翻译器",
        "Tradutor de documentos com IA", "AI μεταφραστής εγγράφων", "AI interpres documentorum");
    put(m, "no_local_models", "Нет локальных моделей — установите в Ollama",
        "No local models — install in Ollama", "Keine lokalen Modelle — in Ollama installieren",
        "Aucun modèle local — installez dans Ollama", "Sin modelos locales — instálelos en Ollama",
        "Немає локальних моделей — встановіть у Ollama", "无本地模型 — 请在 Ollama 中安装",
        "Sem modelos locais — instale no Ollama", "Δεν υπάρχουν τοπικά μοντέλα — εγκαταστήστε στο Ollama",
        "Nulla exemplaria localia — in Ollama pone");
    put(m, "no_cloud_models", "Нет облачных моделей — укажите API в настройках",
        "No cloud models — add API keys in Settings", "Keine Cloud-Modelle — API in den Einstellungen",
        "Aucun modèle cloud — ajoutez l'API dans Paramètres", "Sin modelos en la nube — API en Ajustes",
        "Немає хмарних моделей — API в налаштуваннях", "无云端模型 — 请在设置中添加 API",
        "Sem modelos na nuvem — API em Configurações", "Δεν υπάρχουν cloud μοντέλα — πρόσθεσε API στις Ρυθμίσεις",
        "Nulla exemplaria nubis — API in optionibus");
    put(m, "select_language", "Выберите язык", "Select language", "Sprache wählen", "Choisir la langue",
        "Elija idioma", "Оберіть мову", "选择语言", "Selecionar idioma", "Επίλεξε γλώσσα", "Elige linguam");
    put(m, "hub_title", "Центр перевода", "Translation hub", "Übersetzungszentrum", "Centre de traduction",
        "Centro de traducción", "Центр перекладу", "翻译中心", "Centro de tradução", "Κέντρο μετάφρασης",
        "Centrum translationis");
    put(m, "hub_pipeline", "Пайплайн", "Pipeline", "Pipeline", "Pipeline", "Flujo", "Пайплайн", "流程", "Pipeline",
        "Αγωγός", "Pipeline");
    put(m, "hub_export", "Экспорт", "Export", "Export", "Export", "Exportar", "Експорт", "导出", "Exportar",
        "Εξαγωγή", "Exportatio");
    {
        const char* kFormats = "PDF, DOCX, XLSX, SRT, JSON, EPUB, TXT, MD, HTML";
        put(m, "hub_formats", kFormats, kFormats, kFormats, kFormats, kFormats, kFormats, kFormats,
            kFormats, kFormats, kFormats);
    }
    put(m, "settings_close", "Закрыть", "Close", "Schließen", "Fermer", "Cerrar", "Закрити", "关闭", "Fechar",
        "Κλείσιμο", "Claudere");
    put(m, "settings_nav_general", "Общие", "General", "Allgemein", "Général", "General", "Загальне", "常规",
        "Geral", "Γενικά", "Generalia");
    put(m, "settings_nav_languages", "Языки", "Languages", "Sprachen", "Langues", "Idiomas", "Мови", "语言",
        "Idiomas", "Γλώσσες", "Linguae");
    put(m, "settings_nav_models", "Модели", "Models", "Modelle", "Modèles", "Modelos", "Моделі", "模型", "Modelos",
        "Μοντέλα", "Exemplaria");
    put(m, "settings_nav_cloud", "Облако API", "Cloud API", "Cloud-API", "API cloud", "API en la nube", "Хмара API",
        "云端 API", "API na nuvem", "Cloud API", "API nubis");
    put(m, "settings_nav_pdf", "Вёрстка PDF", "PDF layout", "PDF-Layout", "Mise en page PDF", "Diseño PDF",
        "Верстка PDF", "PDF 排版", "Layout PDF", "Διάταξη PDF", "Compositio PDF");
    put(m, "settings_nav_translation", "Перевод", "Translation", "Übersetzung", "Traduction", "Traducción",
        "Переклад", "翻译", "Tradução", "Μετάφραση", "Translatio");
    put(m, "settings_interface", "Интерфейс", "Interface", "Oberfläche", "Interface", "Interfaz", "Інтерфейс",
        "界面", "Interface", "Διεπαφή", "Interfacies");
    put(m, "settings_app_language", "Язык интерфейса", "Application language", "Anwendungssprache",
        "Langue de l'interface", "Idioma de la interfaz", "Мова інтерфейсу", "界面语言", "Idioma da interface",
        "Γλώσσα διεπαφής", "Lingua interfaciei");
    put(m, "settings_server_url", "Адрес сервера", "Server URL", "Server-URL", "URL du serveur", "URL del servidor",
        "Адреса сервера", "服务器地址", "URL do servidor", "Διεύθυνση διακομιστή", "URL servi");
    put(m, "settings_langs_pickers", "Языки в списках перевода", "Languages in translation lists",
        "Sprachen in Übersetzungslisten", "Langues dans les listes", "Idiomas en listas de traducción",
        "Мови у списках перекладу", "翻译列表中的语言", "Idiomas nas listas de tradução",
        "Γλώσσες στις λίστες μετάφρασης", "Linguae in indice translationis");
    put(m, "settings_local_installed", "Установлены в Ollama:", "Installed in Ollama:", "In Ollama installiert:",
        "Installés dans Ollama :", "Instalados en Ollama:", "Встановлено в Ollama:", "已在 Ollama 中安装：",
        "Instalados no Ollama:", "Εγκατεστημένα στο Ollama:", "In Ollama posita:");
    put(m, "settings_refresh_list", "Обновить список", "Refresh list", "Liste aktualisieren",
        "Actualiser la liste", "Actualizar lista", "Оновити список", "刷新列表", "Atualizar lista",
        "Ανανέωση λίστας", "Index renovare");
    put(m, "settings_cloud_configured", "Настроенные модели:", "Configured models:", "Konfigurierte Modelle:",
        "Modèles configurés :", "Modelos configurados:", "Налаштовані моделі:", "已配置的模型：",
        "Modelos configurados:", "Ρυθμισμένα μοντέλα:", "Exemplaria configurata:");
    put(m, "settings_save_provider", "Сохранить", "Save provider", "Speichern", "Enregistrer", "Guardar", "Зберегти",
        "保存", "Salvar", "Αποθήκευση", "Servare");
    put(m, "settings_pdf_intro",
        "Движок PDF. Внешние инструменты запускаются локально (Python).",
        "Choose a PDF engine. External tools run locally via Python.",
        "PDF-Engine. Externe Tools laufen lokal (Python).",
        "Moteur PDF. Les outils externes s'exécutent localement (Python).",
        "Motor PDF. Las herramientas externas se ejecutan localmente (Python).",
        "Рушій PDF. Зовнішні інструменти локально (Python).", "选择 PDF 引擎。外部工具在本地通过 Python 运行。",
        "Motor PDF. Ferramentas externas rodam localmente (Python).",
        "Μηχανή PDF. Εξωτερικά εργαλεία τοπικά (Python).",
        "Machina PDF. Instrumenta externa localiter (Python).");
    put(m, "settings_pdf_engine", "Движок PDF", "PDF engine", "PDF-Engine", "Moteur PDF", "Motor PDF", "Рушій PDF",
        "PDF 引擎", "Motor PDF", "Μηχανή PDF", "Machina PDF");
    put(m, "settings_authors_prefix", "Авторы: ", "Authors: ", "Autoren: ", "Auteurs : ", "Autores: ", "Автори: ",
        "作者：", "Autores: ", "Συγγραφείς: ", "Auctores: ");
    put(m, "settings_check", "Проверить", "Check", "Prüfen", "Vérifier", "Comprobar", "Перевірити", "检查",
        "Verificar", "Έλεγχος", "Probare");
    put(m, "settings_not_checked", "Не проверено", "Not checked", "Nicht geprüft", "Non vérifié", "Sin comprobar",
        "Не перевірено", "未检查", "Não verificado", "Δεν ελέγχθηκε", "Non probatum");
    put(m, "settings_pdf2zh_releases", "Релизы pdf2zh", "pdf2zh releases", "pdf2zh-Releases", "Versions pdf2zh",
        "Versiones pdf2zh", "Релізи pdf2zh", "pdf2zh 发布", "Lançamentos pdf2zh", "Εκδόσεις pdf2zh",
        "Editiones pdf2zh");
    put(m, "settings_auto_layout", "Авто layout", "Auto layout extract", "Layout automatisch",
        "Extraction auto du layout", "Layout automático", "Авто layout", "自动版式提取", "Layout automático",
        "Αυτόματη διάταξη", "Compositio automatica");
    put(m, "settings_preserve_tables", "Сохранять таблицы", "Preserve tables", "Tabellen erhalten",
        "Conserver les tableaux", "Conservar tablas", "Зберігати таблиці", "保留表格", "Preservar tabelas",
        "Διατήρηση πινάκων", "Tabulas servare");
    put(m, "settings_pdf_license",
        "PyMuPDF и pdf2zh — AGPL-3.0. См. engines/THIRD_PARTY.md.",
        "PyMuPDF and pdf2zh are AGPL-3.0. See engines/THIRD_PARTY.md.",
        "PyMuPDF und pdf2zh: AGPL-3.0. Siehe engines/THIRD_PARTY.md.",
        "PyMuPDF et pdf2zh : AGPL-3.0. Voir engines/THIRD_PARTY.md.",
        "PyMuPDF y pdf2zh: AGPL-3.0. Ver engines/THIRD_PARTY.md.",
        "PyMuPDF і pdf2zh — AGPL-3.0. Див. engines/THIRD_PARTY.md.",
        "PyMuPDF 与 pdf2zh 为 AGPL-3.0。见 engines/THIRD_PARTY.md。",
        "PyMuPDF e pdf2zh: AGPL-3.0. Ver engines/THIRD_PARTY.md.",
        "PyMuPDF και pdf2zh — AGPL-3.0. Βλ. engines/THIRD_PARTY.md.",
        "PyMuPDF et pdf2zh — AGPL-3.0. Vide engines/THIRD_PARTY.md.");
    put(m, "settings_glossary_prompts", "Глоссарий в промптах", "Use glossary in prompts", "Glossar in Prompts",
        "Glossaire dans les prompts", "Glosario en prompts", "Глосарій у промптах", "在提示中使用术语表",
        "Glossário nos prompts", "Γλωσσάριο στα prompts", "Glossarium in mandatis");
    put(m, "settings_glossary", "Глоссарий", "Glossary", "Glossar", "Glossaire", "Glosario", "Глосарій", "术语表",
        "Glossário", "Γλωσσάριο", "Glossarium");
    put(m, "settings_glossary_placeholder", "термин=перевод (по строке)", "term=translation (one per line)",
        "Begriff=Übersetzung (eine pro Zeile)", "terme=traduction (une par ligne)",
        "término=traducción (uno por línea)", "термін=переклад (по рядку)", "术语=译文（每行一条）",
        "termo=tradução (um por linha)", "όρος=μετάφραση (μία ανά γραμμή)", "verbum=versio (una per lineam)");
    put(m, "settings_parallel_requests", "Параллельных запросов", "Parallel requests", "Parallele Anfragen",
        "Requêtes parallèles", "Solicitudes paralelas", "Паралельних запитів", "并行请求数", "Pedidos paralelos",
        "Παράλληλα αιτήματα", "Petitions parallelae");
    put(m, "pdf_etemenanki_desc",
        "Встроенный движок: перевод блоков вашей моделью, сборка PDF через встроенный Python + PyMuPDF.",
        "Built-in engine: block translation via your selected model, PDF rebuild with embedded Python + PyMuPDF.",
        "Integrierte Engine: Blockübersetzung mit Ihrem Modell, PDF-Neuaufbau mit Python + PyMuPDF.",
        "Moteur intégré : traduction par blocs avec votre modèle, reconstruction PDF via Python + PyMuPDF.",
        "Motor integrado: traducción por bloques con su modelo, PDF con Python + PyMuPDF.",
        "Вбудований рушій: переклад блоків вашою моделлю, збірка PDF через Python + PyMuPDF.",
        "内置引擎：使用所选模型分块翻译，通过 Python + PyMuPDF 重建 PDF。",
        "Motor integrado: tradução por blocos com seu modelo, PDF via Python + PyMuPDF.",
        "Ενσωματωμένη μηχανή: μετάφραση με το μοντέλο σου, PDF με Python + PyMuPDF.",
        "Machina inserta: translatio fragmentorum cum exemplo tuo, PDF per Python + PyMuPDF.");
    put(m, "pdf_etemenanki_requires",
        "Python в engines/python/ или dev .venv (см. engines/python/README.md).",
        "Bundled Python in engines/python/ or dev .venv (see engines/python/README.md).",
        "Python in engines/python/ oder dev .venv (siehe engines/python/README.md).",
        "Python dans engines/python/ ou .venv (voir engines/python/README.md).",
        "Python en engines/python/ o .venv dev (ver engines/python/README.md).",
        "Python у engines/python/ або dev .venv (див. engines/python/README.md).",
        "Python 位于 engines/python/ 或开发用 .venv（见 engines/python/README.md）。",
        "Python em engines/python/ ou .venv dev (ver engines/python/README.md).",
        "Python στο engines/python/ ή dev .venv (βλ. engines/python/README.md).",
        "Python in engines/python/ vel .venv (vide engines/python/README.md).");
    put(m, "pdf_pdf2zh_desc",
        "Перевод научных PDF с формулами и вёрсткой (EMNLP 2025 Demo). Портативный .exe — Python не нужен.",
        "Scientific PDF translation with formulas, tables and layout preserved (EMNLP 2025 Demo). Portable .exe bundle — no Python install.",
        "Wissenschaftliche PDF-Übersetzung mit Formeln und Layout (EMNLP 2025 Demo). Portables .exe — kein Python.",
        "Traduction PDF scientifique avec formules et mise en page (EMNLP 2025 Demo). .exe portable.",
        "Traducción PDF científica con fórmulas y diseño (EMNLP 2025 Demo). .exe portable.",
        "Науковий PDF з формулами та версткою (EMNLP 2025 Demo). Портативний .exe.",
        "科学 PDF 翻译，保留公式与版式（EMNLP 2025 Demo）。便携 .exe。",
        "Tradução PDF científica com fórmulas e layout (EMNLP 2025 Demo). .exe portátil.",
        "Επιστημονικό PDF με τύπους και διάταξη (EMNLP 2025 Demo). Φορητό .exe.",
        "Translatio PDF scientifica cum formulis et compositione (EMNLP 2025 Demo). .exe portabile.");
    put(m, "pdf_pdf2zh_requires",
        "Запустите tools/setup_pdf2zh.ps1 или положите pdf2zh.exe в engines/pdf2zh/. Ollama из настроек Etemenanki.",
        "Run tools/setup_pdf2zh.ps1 or place pdf2zh.exe in engines/pdf2zh/. Uses Ollama from Etemenanki settings.",
        "tools/setup_pdf2zh.ps1 ausführen oder pdf2zh.exe in engines/pdf2zh/. Ollama aus den Einstellungen.",
        "Exécutez tools/setup_pdf2zh.ps1 ou placez pdf2zh.exe dans engines/pdf2zh/. Ollama depuis les paramètres.",
        "Ejecute tools/setup_pdf2zh.ps1 o coloque pdf2zh.exe en engines/pdf2zh/. Ollama desde ajustes.",
        "Запустіть tools/setup_pdf2zh.ps1 або покладіть pdf2zh.exe в engines/pdf2zh/. Ollama з налаштувань.",
        "运行 tools/setup_pdf2zh.ps1 或将 pdf2zh.exe 放入 engines/pdf2zh/。使用 Etemenanki 的 Ollama 设置。",
        "Execute tools/setup_pdf2zh.ps1 ou coloque pdf2zh.exe em engines/pdf2zh/. Ollama das configurações.",
        "Εκτέλεσε tools/setup_pdf2zh.ps1 ή βάλε pdf2zh.exe στο engines/pdf2zh/. Ollama από τις Ρυθμίσεις.",
        "Fac tools/setup_pdf2zh.ps1 vel pone pdf2zh.exe in engines/pdf2zh/. Ollama ex optionibus.");
    addMainUiStrings(m);
    addStatusUiStrings(m);
    return m;
}

} // namespace

const KeyTable& table()
{
    static const KeyTable kTable = build();
    return kTable;
}

QString text(const QString& key, const QString& lang)
{
    const KeyTable& keys = table();
    const auto keyIt = keys.constFind(key);
    if (keyIt == keys.constEnd())
        return key;

    const LangTable& langs = keyIt.value();
    const auto langIt = langs.constFind(lang);
    if (langIt != langs.constEnd())
        return langIt.value();

    if (lang == QStringLiteral("uk")) {
        const auto ruIt = langs.constFind(QStringLiteral("ru"));
        if (ruIt != langs.constEnd())
            return ruIt.value();
    }
    const auto enIt = langs.constFind(QStringLiteral("en"));
    if (enIt != langs.constEnd())
        return enIt.value();
    const auto ruIt = langs.constFind(QStringLiteral("ru"));
    if (ruIt != langs.constEnd())
        return ruIt.value();
    return key;
}

QString textArgs(const QString& key, const QString& lang, const QStringList& args)
{
    QString out = text(key, lang);
    for (const QString& arg : args)
        out = out.arg(arg);
    return out;
}

QString translateMessage(const QString& message, const QString& lang)
{
    if (lang == QStringLiteral("ru"))
        return message;

    const KeyTable& keys = table();
    for (auto it = keys.cbegin(); it != keys.cend(); ++it) {
        if (!it.key().startsWith(QStringLiteral("err_")))
            continue;
        const auto ruIt = it->find(QStringLiteral("ru"));
        if (ruIt == it->end())
            continue;
        if (ruIt.value() == message)
            return text(it.key(), lang);

        const QString& ru = ruIt.value();
        if (ru.contains(QLatin1Char('%')) && message.startsWith(ru.section(QLatin1Char('%'), 0, 0))) {
            const int prefixLen = ru.indexOf(QLatin1Char('%'));
            if (prefixLen > 0 && message.startsWith(ru.left(prefixLen))) {
                const QString suffix = message.mid(prefixLen);
                return text(it.key(), lang).arg(suffix);
            }
        }
    }
    return message;
}

QVariantList uiLanguageOptions()
{
    struct Entry {
        const char* code;
        const char* labelUtf8;
    };
    static const Entry kEntries[] = {
        {"ru", "Русский"},
        {"en", "English"},
        {"de", "Deutsch"},
        {"fr", "Français"},
        {"es", "Español"},
        {"uk", "Українська"},
        {"zh", "中文"},
        {"pt", "Português"},
        {"el", "Ελληνικά"},
        {"la", "Latina"},
    };
    QVariantList out;
    out.reserve(static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0])));
    for (const Entry& e : kEntries) {
        out.append(QVariantMap{
            {QStringLiteral("code"), QString::fromUtf8(e.code)},
            {QStringLiteral("label"), QString::fromUtf8(e.labelUtf8)},
        });
    }
    return out;
}

} // namespace AppUiStrings

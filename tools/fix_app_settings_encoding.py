#!/usr/bin/env python3
"""Fix corrupted UTF-8 in app_settings.cpp (allLanguages + helpers)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "cpp" / "app_settings.cpp"


def cpp_u8(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")

    helper = r'''
QString regionalFlag(const char* iso2)
{
    if (!iso2 || !iso2[0] || !iso2[1])
        return QString::fromUtf8(u8"\xF0\x9F\x8C\x90");
    const auto cp = [](char c) -> char32_t {
        return 0x1F1E6u + static_cast<unsigned char>(c) - static_cast<unsigned char>('A');
    };
    return QString::fromUcs4({cp(iso2[0]), cp(iso2[1])});
}

QVariantMap languageRow(const char* code, const QString& label, const char* iso2)
{
    return QVariantMap{
        {QStringLiteral("code"), QString::fromUtf8(code)},
        {QStringLiteral("label"), label},
        {QStringLiteral("flag"), regionalFlag(iso2)},
    };
}

'''
    if "QString regionalFlag" not in text:
        text = text.replace("QString providerForModelId", helper + "QString providerForModelId", 1)

    start = text.index("QVariantList AppSettings::allLanguages()")
    end = text.index("QVariantList AppSettings::enabledLanguages", start)

    rows = [
        ("auto", None, ""),
        ("en", "English (EN)", "US"),
        ("ru", "Русский (RU)", "RU"),
        ("de", "Deutsch (DE)", "DE"),
        ("fr", "Français (FR)", "FR"),
        ("es", "Español (ES)", "ES"),
        ("it", "Italiano (IT)", "IT"),
        ("pt", "Português (PT)", "PT"),
        ("pl", "Polski (PL)", "PL"),
        ("nl", "Nederlands (NL)", "NL"),
        ("sv", "Svenska (SV)", "SE"),
        ("uk", "Українська (UK)", "UA"),
        ("zh", "中文 (ZH)", "CN"),
        ("ja", "日本語 (JA)", "JP"),
        ("ko", "한국어 (KO)", "KR"),
        ("ar", "العربية (AR)", "SA"),
        ("tr", "Türkçe (TR)", "TR"),
        ("cs", "Čeština (CS)", "CZ"),
        ("ro", "Română (RO)", "RO"),
        ("hu", "Magyar (HU)", "HU"),
        ("fi", "Suomi (FI)", "FI"),
        ("no", "Norsk (NO)", "NO"),
        ("da", "Dansk (DA)", "DK"),
        ("el", "Ελληνικά (EL)", "GR"),
        ("he", "עברית (HE)", "IL"),
        ("hi", "हिन्दी (HI)", "IN"),
        ("vi", "Tiếng Việt (VI)", "VN"),
        ("th", "ไทย (TH)", "TH"),
        ("id", "Bahasa Indonesia (ID)", "ID"),
    ]

    out = [
        "QVariantList AppSettings::allLanguages() const",
        "{",
        "    return {",
    ]
    for code, label, iso in rows:
        if label is None:
            out.append(
                f'        languageRow("{code}", uiText(QStringLiteral("lang_auto")), "{iso}"),'
            )
        else:
            out.append(
                f'        languageRow("{code}", QString::fromUtf8(u8"{cpp_u8(label)}"), "{iso}"),'
            )
    out.extend(["    };", "}", ""])

    text = text[:start] + "\n".join(out) + "\n" + text[end:]
    TARGET.write_text(text, encoding="utf-8")
    print(f"Updated {TARGET}")


if __name__ == "__main__":
    main()

# Etemenanki branding (v5 horizontal + slogan)

Master: horizontal logo with tagline **«Нет легкого пути от земли к звездам»**.

## Files

| File | Use |
|------|-----|
| `app-icon-source.png` | Square app icon master (user upload) |
| `app-icon.png` | Square icon 512×512 |
| `logo-horizontal-source.png` | Horizontal logo master with slogan |
| `logo-horizontal.png` | Full horizontal logo (source export) |
| `logo-header.png` | App header (~72px height) |
| `logo-splash.png` | About / splash (~900px wide) |
| `icon-16.png` … `icon-512.png` | UI / store sizes |
| `app.ico` | Windows executable icon |

## Regenerate

```powershell
.\.venv\Scripts\python.exe tools\build_brand_assets.py
```

Masters are read from `assets/branding/` or Cursor `assets/etemenanki-brand-*-master.png`.

After rebuild, `cmake --build build --config Release` copies this folder next to `Etemenanki.exe`.

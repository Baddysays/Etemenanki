#!/usr/bin/env python3
"""Extract PDF text layout and rebuild a translated PDF with pymupdf."""
from __future__ import annotations

import contextlib
import io
import json
import re
import sys
import unicodedata
import warnings
from dataclasses import dataclass
from pathlib import Path

warnings.filterwarnings("ignore")


@contextlib.contextmanager
def _silence_stdout():
    """pymupdf sometimes prints hints to stdout; keep stdout JSON-only for Qt."""
    buffer = io.StringIO()
    old_stdout = sys.stdout
    sys.stdout = buffer
    try:
        yield
    finally:
        sys.stdout = old_stdout


def _rects_overlap(a: tuple, b: tuple, tol: float = 2.0) -> bool:
    return not (a[2] < b[0] - tol or a[0] > b[2] + tol or a[3] < b[1] - tol or a[1] > b[3] + tol)


def _span_bbox(span: dict) -> tuple:
    return tuple(span.get("bbox", (0, 0, 0, 0)))


def _merge_bbox(a: tuple, b: tuple) -> tuple:
    return (min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]))


def _contains_cjk(text: str) -> bool:
    for ch in text:
        if ch in "\u3000\u30fb":
            continue
        try:
            if unicodedata.name(ch).startswith(("CJK", "HIRAGANA", "KATAKANA", "HANGUL")):
                return True
        except ValueError:
            pass
        code = ord(ch)
        if (
            0x4E00 <= code <= 0x9FFF
            or 0x3400 <= code <= 0x4DBF
            or 0x3040 <= code <= 0x30FF
            or 0xAC00 <= code <= 0xD7AF
        ):
            return True
    return False


def _first_existing_path(candidates: list[Path]) -> str | None:
    for path in candidates:
        if path.is_file():
            return str(path)
    return None


def _pick_system_uni_paths() -> tuple[str, str | None]:
    regular = _first_existing_path(
        [
            Path(r"C:\Windows\Fonts\arialuni.ttf"),
            Path(r"C:\Windows\Fonts\ARIALUNI.TTF"),
            Path(r"C:\Windows\Fonts\segoeui.ttf"),
            Path(r"C:\Windows\Fonts\arial.ttf"),
            Path(r"C:\Windows\Fonts\calibri.ttf"),
            Path("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"),
            Path("/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf"),
            Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
            Path("/System/Library/Fonts/Supplemental/Arial Unicode.ttf"),
            Path("/System/Library/Fonts/Supplemental/Arial.ttf"),
        ]
    )
    bold = _first_existing_path(
        [
            Path(r"C:\Windows\Fonts\arialbd.ttf"),
            Path(r"C:\Windows\Fonts\segoeuib.ttf"),
            Path(r"C:\Windows\Fonts\calibrib.ttf"),
            Path("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf"),
            Path("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"),
        ]
    )
    return regular or "", bold


def _pick_system_cjk_path() -> str | None:
    return _first_existing_path(
        [
            Path(r"C:\Windows\Fonts\msyh.ttc"),
            Path(r"C:\Windows\Fonts\msyhbd.ttc"),
            Path(r"C:\Windows\Fonts\simsun.ttc"),
            Path(r"C:\Windows\Fonts\malgun.ttf"),
            Path(r"C:\Windows\Fonts\meiryo.ttc"),
            Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"),
            Path("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"),
            Path("/System/Library/Fonts/PingFang.ttc"),
        ]
    )


def _try_pymupdf_font(name: str):
    import fitz

    try:
        return fitz.Font(name)
    except Exception:
        return None


@dataclass
class MultilingualFonts:
    """Embedded fonts for translated PDF text (many scripts / languages)."""

    NAME_UNI: str = "ete-uni"
    NAME_BOLD: str = "ete-bold"
    NAME_CJK: str = "ete-cjk"

    def __init__(self) -> None:
        import fitz

        self._uni: fitz.Font | None = None
        self._bold: fitz.Font | None = None
        self._cjk: fitz.Font | None = None
        self._bold_name = self.NAME_UNI
        self._load()

    def _load(self) -> None:
        import fitz

        try:
            import pymupdf_fonts  # noqa: F401
        except ImportError:
            pass

        for name in ("figo", "tiro"):
            uni = _try_pymupdf_font(name)
            if uni is None:
                continue
            self._uni = uni
            bold = None
            for bold_name in (f"{name}-bold", f"{name}bold"):
                bold = _try_pymupdf_font(bold_name)
                if bold is not None:
                    break
            if bold is None:
                try:
                    bold = fitz.Font(name, is_bold=1)
                except Exception:
                    bold = None
            self._bold = bold or uni
            break

        if self._uni is None:
            regular_path, bold_path = _pick_system_uni_paths()
            if not regular_path:
                raise RuntimeError(
                    "No Unicode font for PDF output (install pymupdf-fonts or Arial/Segoe/Noto)"
                )
            self._uni = fitz.Font(fontfile=regular_path)
            self._bold = (
                fitz.Font(fontfile=bold_path)
                if bold_path and bold_path != regular_path
                else self._uni
            )

        for cjk_name in ("cjk", "china-s", "japan", "korea"):
            cjk = _try_pymupdf_font(cjk_name)
            if cjk is not None:
                self._cjk = cjk
                break

        if self._cjk is None:
            cjk_path = _pick_system_cjk_path()
            if cjk_path:
                try:
                    self._cjk = fitz.Font(fontfile=cjk_path)
                except (RuntimeError, ValueError, TypeError):
                    try:
                        self._cjk = fitz.Font(fontfile=cjk_path, fontnumber=0)
                    except (RuntimeError, ValueError, TypeError):
                        self._cjk = None

    def register_on_page(self, page) -> None:
        page.insert_font(fontname=self.NAME_UNI, fontbuffer=self._uni.buffer)
        if self._bold is not self._uni:
            page.insert_font(fontname=self.NAME_BOLD, fontbuffer=self._bold.buffer)
            self._bold_name = self.NAME_BOLD
        else:
            self._bold_name = self.NAME_UNI
        if self._cjk is not None:
            page.insert_font(fontname=self.NAME_CJK, fontbuffer=self._cjk.buffer)

    def fontname_for(self, text: str, bold: bool) -> str:
        if self._cjk is not None and _contains_cjk(text):
            return self.NAME_CJK
        if bold:
            return self._bold_name
        return self.NAME_UNI


_BULLET_CHARS = frozenset("•·▪▫‣⁃◦●○■□▪\u2022\u2023\u2043\u2219")
_BULLET_PRIVATE = re.compile(r"^[\uf0a0-\uf0ff\u0080-\u009f]\s*")


def _normalize_pdf_text(text: str) -> str:
    if not text:
        return text
    out = text.replace("\u00ad", "").replace("\ufffd", "")
    for src, dst in (
        ("\uf0b7", "•"),
        ("\uf0a7", "•"),
        ("\uf076", "•"),
        ("\x95", "•"),
        ("\u2013", "-"),
        ("\u2014", "-"),
        ("\u2018", "'"),
        ("\u2019", "'"),
        ("\u201c", '"'),
        ("\u201d", '"'),
    ):
        out = out.replace(src, dst)
    out = _BULLET_PRIVATE.sub("• ", out)
    return out.strip()


def _image_rects(page) -> list[tuple]:
    rects: list[tuple] = []
    for block in page.get_text("dict").get("blocks", []):
        if block.get("type") == 1:
            rects.append(tuple(block.get("bbox", (0, 0, 0, 0))))
    return rects


def _is_false_header_table(table, page_rect, image_rects: list[tuple]) -> bool:
    """Skip logo bands only — keep real metadata tables."""
    bbox = tuple(table.bbox)
    width = page_rect.width
    height = page_rect.height
    table_w = bbox[2] - bbox[0]
    table_h = bbox[3] - bbox[1]

    try:
        extracted = table.extract() or []
        rows = len(extracted)
        cols = len(extracted[0]) if extracted else 0
        filled = sum(1 for row in extracted for cell in row if str(cell or "").strip())
        if rows >= 2 and cols >= 2 and filled >= 3:
            return False
    except (AttributeError, RuntimeError, ValueError, TypeError):
        pass

    if table_h < 36 and table_w > width * 0.82 and bbox[1] < height * 0.14:
        return True
    for img in image_rects:
        if _rects_overlap(bbox, img, tol=0.0) and table_h < 50:
            return True
    return False


def _extract_table(page, table, page_idx: int, table_idx: int) -> dict | None:
    try:
        extracted = table.extract() or []
        cells = table.cells
    except (AttributeError, RuntimeError, ValueError, TypeError):
        return None

    rows = len(extracted)
    cols = len(extracted[0]) if extracted else 0
    if rows == 0 or cols == 0:
        return None

    table_id = f"p{page_idx}_t{table_idx}"
    cell_items: list[dict] = []
    for r in range(rows):
        for c in range(cols):
            try:
                bbox = tuple(cells[r * cols + c])
            except (IndexError, TypeError):
                continue
            if bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
                continue
            text = _normalize_pdf_text(str(extracted[r][c] or ""))
            cell_items.append(
                {
                    "id": f"{table_id}_r{r}_c{c}",
                    "row": r,
                    "col": c,
                    "bbox": list(bbox),
                    "text": text,
                    "bold": r == 0,
                    "size": 8.5 if r > 0 else 9.0,
                }
            )

    if sum(1 for cell in cell_items if cell["text"]) < 2:
        return None

    return {
        "id": table_id,
        "bbox": list(table.bbox),
        "rows": rows,
        "cols": cols,
        "cells": cell_items,
    }


def extract_pdf_layout(path: Path) -> dict:
    import fitz

    pages: list[dict] = []
    with fitz.open(path) as doc:
        for page_idx, page in enumerate(doc):
            page_rect = page.rect
            image_rects = _image_rects(page)
            table_rects: list[tuple] = []
            tables: list[dict] = []
            items: list[dict] = []
            item_idx = 0

            try:
                with _silence_stdout():
                    finder = page.find_tables()
                    table_idx = 0
                    for table in list(getattr(finder, "tables", []) or [])[:12]:
                        if _is_false_header_table(table, page_rect, image_rects):
                            continue
                        table_obj = _extract_table(page, table, page_idx, table_idx)
                        if table_obj is None:
                            continue
                        tables.append(table_obj)
                        table_rects.append(tuple(table.bbox))
                        table_idx += 1
            except (AttributeError, RuntimeError, ValueError, TypeError):
                pass

            data = page.get_text("dict")
            lines: list[dict] = []
            for block in data.get("blocks", []):
                if block.get("type") != 0:
                    continue
                block_bbox = tuple(block.get("bbox", (0, 0, 0, 0)))
                if any(_rects_overlap(block_bbox, rect, tol=1.5) for rect in table_rects):
                    continue
                for line in block.get("lines", []):
                    spans = line.get("spans", [])
                    if not spans:
                        continue
                    text = _normalize_pdf_text("".join(span.get("text", "") for span in spans))
                    if not text:
                        continue
                    bbox = _span_bbox(spans[0])
                    for span in spans[1:]:
                        bbox = _merge_bbox(bbox, _span_bbox(span))
                    if any(_rects_overlap(bbox, img, tol=1.0) for img in image_rects):
                        continue
                    sizes = [float(span.get("size", 10.0)) for span in spans]
                    fonts = [str(span.get("font", "")) for span in spans]
                    max_size = max(sizes) if sizes else 10.0
                    bold = any("bold" in f.lower() for f in fonts)
                    kind = "text"
                    if text and text[0] in _BULLET_CHARS:
                        kind = "list"
                    lines.append({"text": text, "bbox": bbox, "size": max_size, "bold": bold, "kind": kind})

            lines.sort(key=lambda ln: (round(ln["bbox"][1], 1), ln["bbox"][0]))
            for ln in lines:
                item_idx += 1
                items.append(
                    {
                        "id": f"p{page_idx}_i{item_idx}",
                        "kind": ln.get("kind", "text"),
                        "bbox": list(ln["bbox"]),
                        "text": ln["text"],
                        "size": round(float(ln["size"]), 1),
                        "bold": ln["bold"],
                    }
                )

            pages.append(
                {
                    "index": page_idx,
                    "width": float(page_rect.width),
                    "height": float(page_rect.height),
                    "items": items,
                    "tables": tables,
                }
            )

    return {"pages": pages}


def _insert_fitted(
    page,
    rect,
    text: str,
    fonts: MultilingualFonts,
    start_size: float,
    bold: bool = False,
) -> None:
    import fitz

    fontname = fonts.fontname_for(text, bold)
    box = fitz.Rect(rect)
    size = max(6.0, min(start_size, 22.0))
    min_size = 6.0
    for _ in range(12):
        if page.insert_textbox(
            box,
            text,
            fontname=fontname,
            fontsize=size,
            color=(0, 0, 0),
            align=fitz.TEXT_ALIGN_LEFT,
        ) >= 0:
            return
        size = max(min_size, size - 0.5)
    page.insert_textbox(
        box,
        text,
        fontname=fontname,
        fontsize=min_size,
        color=(0, 0, 0),
        align=fitz.TEXT_ALIGN_LEFT,
    )


def _draw_table_grid(page, table: dict) -> None:
    import fitz

    for cell in table.get("cells", []):
        bbox = cell.get("bbox")
        if not bbox or len(bbox) != 4:
            continue
        page.draw_rect(fitz.Rect(bbox), color=(0.45, 0.45, 0.45), width=0.6)
    outer = table.get("bbox")
    if outer and len(outer) == 4:
        page.draw_rect(fitz.Rect(outer), color=(0.35, 0.35, 0.35), width=0.8)


def build_translated_pdf(source_path: Path, layout: dict, output_path: Path) -> None:
    import fitz

    fonts = MultilingualFonts()

    with fitz.open(str(source_path)) as doc:
        for page_data in layout.get("pages", []):
            page_idx = int(page_data.get("index", 0))
            if page_idx < 0 or page_idx >= len(doc):
                continue
            page = doc[page_idx]

            redact_rects: list = []
            for item in page_data.get("items", []):
                bbox = item.get("bbox")
                if not bbox or len(bbox) != 4:
                    continue
                if not str(item.get("text") or "").strip():
                    continue
                rect = fitz.Rect(bbox)
                pad = 0.3
                redact_rects.append(fitz.Rect(rect.x0 - pad, rect.y0 - pad, rect.x1 + pad, rect.y1 + pad))

            for table in page_data.get("tables", []):
                for cell in table.get("cells", []):
                    if not str(cell.get("text") or "").strip():
                        continue
                    bbox = cell.get("bbox")
                    if not bbox or len(bbox) != 4:
                        continue
                    rect = fitz.Rect(bbox)
                    pad = 0.2
                    redact_rects.append(
                        fitz.Rect(rect.x0 + pad, rect.y0 + pad, rect.x1 - pad, rect.y1 - pad)
                    )

            for rect in redact_rects:
                page.add_redact_annot(rect, fill=(1, 1, 1))
            if redact_rects:
                page.apply_redactions(images=fitz.PDF_REDACT_IMAGE_NONE)

            fonts.register_on_page(page)

            for item in page_data.get("items", []):
                text = _normalize_pdf_text(
                    str(item.get("text_translated") or item.get("text") or "")
                )
                if not text:
                    continue
                bbox = item.get("bbox")
                if not bbox or len(bbox) != 4:
                    continue
                rect = fitz.Rect(bbox)
                size = float(item.get("size", 10.0))
                if item.get("kind") == "list" and not text.startswith("•"):
                    text = "• " + text.lstrip("•").lstrip()
                _insert_fitted(page, rect, text, fonts, size, bool(item.get("bold")))

            for table in page_data.get("tables", []):
                _draw_table_grid(page, table)
                for cell in table.get("cells", []):
                    text = _normalize_pdf_text(
                        str(cell.get("text_translated") or cell.get("text") or "")
                    )
                    if not text:
                        continue
                    bbox = cell.get("bbox")
                    if not bbox or len(bbox) != 4:
                        continue
                    rect = fitz.Rect(bbox)
                    inset = 1.5
                    inner = fitz.Rect(rect.x0 + inset, rect.y0 + inset, rect.x1 - inset, rect.y1 - inset)
                    size = float(cell.get("size", 8.5))
                    _insert_fitted(page, inner, text, fonts, size, bool(cell.get("bold")))

        output_path.parent.mkdir(parents=True, exist_ok=True)
        try:
            doc.subset_fonts()
        except (AttributeError, RuntimeError, ValueError):
            pass
        doc.save(str(output_path), garbage=4, deflate=True)


def emit(payload: dict) -> None:
    sys.stdout.buffer.write(json.dumps(payload, ensure_ascii=True).encode("utf-8"))
    sys.stdout.buffer.write(b"\n")
    sys.stdout.buffer.flush()


def main() -> int:
    if len(sys.argv) < 2:
        emit({"ok": False, "error": "usage: pdf_layout.py extract <pdf> | build <src.pdf> <layout.json> <out.pdf>"})
        return 2

    cmd = sys.argv[1].lower()
    try:
        if cmd == "extract" and len(sys.argv) >= 3:
            path = Path(sys.argv[2])
            if not path.is_file():
                emit({"ok": False, "error": f"file not found: {path}"})
                return 2
            layout = extract_pdf_layout(path)
            emit({"ok": True, "pdf_layout": layout, "page_count": len(layout.get("pages", []))})
            return 0

        if cmd == "build" and len(sys.argv) >= 5:
            source = Path(sys.argv[2])
            layout_path = Path(sys.argv[3])
            output = Path(sys.argv[4])
            if not source.is_file():
                emit({"ok": False, "error": f"source not found: {source}"})
                return 2
            layout = json.loads(layout_path.read_text(encoding="utf-8-sig"))
            build_translated_pdf(source, layout, output)
            emit({"ok": True, "output": str(output.resolve())})
            return 0

        emit({"ok": False, "error": "usage: pdf_layout.py extract <pdf> | build <src.pdf> <layout.json> <out.pdf>"})
        return 2
    except Exception as exc:  # noqa: BLE001
        import traceback

        traceback.print_exc(file=sys.stderr)
        msg = str(exc).strip() or repr(exc)
        emit({"ok": False, "error": msg or type(exc).__name__})
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

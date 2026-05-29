#!/usr/bin/env python3
"""Create a minimal PDF for pipeline smoke tests."""
from pathlib import Path
import sys

import fitz

out = Path(sys.argv[1] if len(sys.argv) > 1 else "test.pdf")
doc = fitz.open()
page = doc.new_page()
page.insert_text((72, 72), "Hello PDF layout test", fontsize=14)
doc.save(str(out))
doc.close()
print(f"created {out.resolve()} ({out.stat().st_size} bytes)")

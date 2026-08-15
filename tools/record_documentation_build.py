#!/usr/bin/env python3
"""Record the exact TeX and PDF hashes for a completed manual render."""

import hashlib
import json
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "output/pdf/manual-build.json"
STEMS = (
    "volume-1-basic-architecture", "volume-2-instruction-reference",
    "volume-3-system-programming", "volume-4-system-registers",
    "volume-5-binary-interfaces",
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


manuals = {}
for stem in STEMS:
    source = ROOT / "docs" / f"{stem}.tex"
    pdf = ROOT / "output/pdf" / f"{stem}.pdf"
    log = ROOT / "output/pdf" / f"{stem}.log"
    if not source.is_file() or not pdf.is_file() or not log.is_file():
        raise SystemExit(f"cannot record incomplete manual build: {stem}")
    log_text = log.read_text(encoding="utf-8", errors="replace")
    if "! LaTeX Error" in log_text or "Overfull \\hbox" in log_text:
        raise SystemExit(f"cannot record manual with TeX errors: {stem}")
    manuals[stem] = {
        "source_sha256": digest(source),
        "pdf_sha256": digest(pdf),
    }

record = {
    "schema_version": 1,
    "architecture": "SeaBird 3.2",
    "sdk": "1.0.0",
    "rendered": date.today().isoformat(),
    "engine": "Tectonic 0.16.9 (XeTeX-compatible)",
    "manuals": manuals,
}
OUT.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
print(f"recorded {len(manuals)} rendered manuals in {OUT.name}")

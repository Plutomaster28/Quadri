#!/usr/bin/env python3
"""Validate SeaBird release-document authority, links, and rendered manuals."""

import json
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
errors = []


def fail(message):
    errors.append(message)


def text(path):
    target = ROOT / path
    if not target.is_file() or target.stat().st_size == 0:
        fail(f"missing or empty documentation file: {path}")
        return ""
    return target.read_text(encoding="utf-8", errors="replace")


version = text("VERSION").strip()
release = dict(
    line.split("=", 1) for line in text("RELEASE").splitlines() if "=" in line
)
layouts = json.loads(text("spec/architectural-layouts.json") or "{}")
isa = json.loads(text("spec/seabird-isa.json") or "{}")
manual_build = json.loads(text("output/pdf/manual-build.json") or "{}")
if version != "1.0.0" or release.get("version") != version:
    fail("SDK version files do not identify 1.0.0")
if release.get("tag") != "marlin" or release.get("architecture") != "SeaBird 3.2":
    fail("release identity is not marlin / SeaBird 3.2")
if layouts.get("architecture_version") != "3.2" or isa.get("architecture_version") != "3.2":
    fail("machine-readable architecture is not 3.2")

required = (
    "README.md", "docs/DOCUMENTATION_INDEX.md", "docs/PAE32_EXTENSION.md",
    "docs/REGISTER_WINDOWING_EXTENSION.md", "docs/TOOLCHAIN_GUIDE.md",
    "docs/COMPILER_STATUS.md", "docs/V1_0_RATIFICATION_REPORT.md",
    "docs/releases/v1.0.0.md", "docs/releases/v1.0.0-validation.md",
)
contents = {path: text(path) for path in required}
for path in ("docs/TOOLCHAIN_GUIDE.md", "packaging/SDK_README.md",
             "examples/programs/README.md"):
    value = text(path)
    for stale in ("v0.1 developer workflow", "`tuna`", "tag/tuna",
                  "v0.1.0-tuna", "developer alpha"):
        if stale in value:
            fail(f"{path}: stale release text {stale!r}")

for path in ("SeaBird_PAE_Reference.md", "SeaBird_Register_Windowing_Reference.md"):
    if "Non-normative and superseded" not in text(path):
        fail(f"{path}: missing superseded-document banner")
if "Non-normative design rationale" not in text(
        "SeaBird Performance Extension & Marker System (Design Notes).md"):
    fail("performance-marker design note lacks an authority banner")

for markdown in ROOT.rglob("*.md"):
    relative_parts = markdown.relative_to(ROOT).parts
    if (not markdown.is_file() or
            any(part == "build" or part.startswith(".") for part in relative_parts)):
        continue
    body = markdown.read_text(encoding="utf-8", errors="replace")
    for match in re.finditer(r"(?<!!)\[[^\]]*\]\(([^)]+)\)", body):
        target = match.group(1).strip().split("#", 1)[0].strip("<>")
        if not target or "://" in target or target.startswith(("mailto:", "/")):
            continue
        if not (markdown.parent / target).exists():
            fail(f"{markdown.relative_to(ROOT)}: missing link target {target}")

for number, stem in enumerate((
        "volume-1-basic-architecture", "volume-2-instruction-reference",
        "volume-3-system-programming", "volume-4-system-registers",
        "volume-5-binary-interfaces"), 1):
    source = ROOT / "docs" / f"{stem}.tex"
    pdf = ROOT / "output/pdf" / f"{stem}.pdf"
    log = ROOT / "output/pdf" / f"{stem}.log"
    if not source.is_file() or not pdf.is_file() or pdf.stat().st_size < 1024:
        fail(f"Volume {number}: source or rendered PDF is missing")
        continue
    recorded = manual_build.get("manuals", {}).get(stem, {})
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    pdf_hash = hashlib.sha256(pdf.read_bytes()).hexdigest()
    if recorded.get("source_sha256") != source_hash:
        fail(f"Volume {number}: TeX source differs from rendered-manual record")
    if recorded.get("pdf_sha256") != pdf_hash:
        fail(f"Volume {number}: PDF differs from rendered-manual record")
    log_text = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
    if "! LaTeX Error" in log_text or "Overfull \\hbox" in log_text:
        fail(f"Volume {number}: TeX log contains a layout error")

if errors:
    print("DOCUMENTATION CHECK FAILED", file=sys.stderr)
    for error in errors:
        print(f"- {error}", file=sys.stderr)
    sys.exit(1)

print("documentation checks passed: authority, release identity, links, and 5 PDFs")

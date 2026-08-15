#!/usr/bin/env python3
"""Split the SeaBird architecture source into stable topical volumes."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "main (3).tex"
DOCS = ROOT / "docs"


def extract(command: str, title: str, text: str) -> str:
    marker = f"\\{command}{{{title}}}"
    start = text.find(marker)
    if start < 0:
        raise ValueError(f"missing {command}: {title}")
    level = {"section": 1, "subsection": 2}[command]
    pattern = re.compile(r"^\\(section|subsection)\{", re.M)
    end = len(text)
    for match in pattern.finditer(text, start + len(marker)):
        found_level = {"section": 1, "subsection": 2}[match.group(1)]
        if found_level <= level:
            end = match.start()
            break
    return text[start:end].strip()


def preamble(volume: str, subtitle: str) -> str:
    return rf"""\documentclass[11pt]{{article}}
\usepackage[utf8]{{inputenc}}
\usepackage[margin=0.85in,includefoot]{{geometry}}
\usepackage{{graphicx,float,longtable,booktabs,enumitem,listings,xcolor,fancyhdr,titlesec,array,hyperref,etoolbox}}
\usepackage[strings]{{underscore}}
\newcolumntype{{L}}[1]{{>{{\raggedright\arraybackslash}}p{{#1}}}}
\newcolumntype{{C}}[1]{{>{{\centering\arraybackslash}}p{{#1}}}}
\newcolumntype{{R}}[1]{{>{{\raggedleft\arraybackslash}}p{{#1}}}}
\setlength{{\tabcolsep}}{{2pt}}\renewcommand{{\arraystretch}}{{1.1}}
\AtBeginEnvironment{{longtable}}{{\footnotesize}}\AtBeginEnvironment{{table}}{{\footnotesize}}
\lstset{{basicstyle=\ttfamily\small,breaklines=true,showstringspaces=false,frame=single,numbers=left,numberstyle=\tiny\color{{gray}},backgroundcolor=\color{{gray!8}}}}
\pagestyle{{fancy}}\fancyhf{{}}
\fancyhead[L]{{\small\sffamily SeaBird ISA {volume}}}\fancyhead[R]{{\small\sffamily Architecture 3.2 / SDK 1.0}}
\fancyfoot[C]{{\thepage}}\setlength{{\headheight}}{{14pt}}
\titleformat{{\section}}{{\Large\bfseries\sffamily}}{{\thesection}}{{0.8em}}{{}}
\titleformat{{\subsection}}{{\large\bfseries\sffamily}}{{\thesubsection}}{{0.8em}}{{}}
\titleformat{{\subsubsection}}{{\normalsize\bfseries\sffamily}}{{\thesubsubsection}}{{0.8em}}{{}}
\hypersetup{{colorlinks=true,linkcolor=black,urlcolor=black}}
\sloppy\emergencystretch=1em
\begin{{document}}
\begin{{titlepage}}\centering\vspace*{{2cm}}
{{\Huge\bfseries SeaBird ISA\\[0.3cm]}}{{\LARGE {volume}: {subtitle}\\[0.5cm]}}
{{\Large Architecture 3.2 / SDK 1.0}}\vfill
{{\large Machine-readable architecture 3.2\\August 14, 2026}}\vfill
\end{{titlepage}}
\tableofcontents\clearpage
"""


def write_volume(filename: str, volume: str, subtitle: str, chunks: list[str]) -> None:
    content = preamble(volume, subtitle) + "\n\n".join(chunks) + "\n\\end{document}\n"
    (DOCS / filename).write_text(content, encoding="utf-8")


def main() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    volume1_titles = [
        "Introduction", "Operating Modes and Memory Model", "Registers",
        "Architectural Programming Model", "Instruction Syntax", "Addressing Modes",
        "Instruction Formats", "Instruction Encoding", "FLAGS Register",
    ]
    volume3_titles = [
        "System Architecture", "Exception \\& Interrupt Handling", "Control Registers",
        "Memory Management", "Implementation Notes",
    ]
    volume4_subsections = [
        "System-Register Namespace", "Debug, Performance, Power, and Reliability",
    ]
    write_volume(
        "volume-1-basic-architecture.tex", "Volume 1", "Basic Architecture",
        [extract("section", title, text) for title in volume1_titles],
    )
    write_volume(
        "volume-3-system-programming.tex", "Volume 3", "System Programming",
        [extract("section", title, text) for title in volume3_titles],
    )
    write_volume(
        "volume-4-system-registers.tex", "Volume 4", "System Registers",
        [extract("subsection", title, text) for title in volume4_subsections]
        + [extract("section", "Control Registers", text)],
    )
    print("generated Volume 1, Volume 3, and Volume 4 LaTeX sources")


if __name__ == "__main__":
    main()

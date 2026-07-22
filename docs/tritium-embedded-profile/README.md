# SeaBird Tritium Embedded Profile

- Edit `profile-config.tex` for profile names, revisions, dates, and target identity.
- Edit `main.tex` for architectural requirements and document styling.
- Build from this directory with UCRT64 XeLaTeX:

```powershell
C:\msys64\ucrt64\bin\xelatex.exe main.tex
C:\msys64\ucrt64\bin\xelatex.exe main.tex
```

XeLaTeX is preferred because the minimal UCRT64 pdfLaTeX installation may not
include all bitmap font-generation dependencies.

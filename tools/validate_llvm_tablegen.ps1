param(
    [string]$LlvmInclude = "C:/tmp/llvm-project-22.1.4/llvm/include",
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$target = Join-Path $root "llvm/SeaBird/SeaBird.td"
if (-not $OutputDir) {
    $OutputDir = Join-Path $root "build/llvm-tablegen"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

python (Join-Path $PSScriptRoot "generate_llvm_tablegen.py") --check

$generators = @{
    "register-info" = "SeaBirdGenRegisterInfo.inc"
    "instr-info" = "SeaBirdGenInstrInfo.inc"
    "callingconv" = "SeaBirdGenCallingConv.inc"
    "dag-isel" = "SeaBirdGenDAGISel.inc"
    "subtarget" = "SeaBirdGenSubtargetInfo.inc"
    "asm-writer" = "SeaBirdGenAsmWriter.inc"
}

foreach ($entry in $generators.GetEnumerator()) {
    llvm-tblgen "-gen-$($entry.Key)" -I $LlvmInclude -I (Split-Path $target) `
        -o (Join-Path $OutputDir $entry.Value) $target
    if ($LASTEXITCODE -ne 0) {
        throw "llvm-tblgen -gen-$($entry.Key) failed with exit code $LASTEXITCODE"
    }
}

Write-Host "SeaBird LLVM TableGen validation passed ($($generators.Count) backends)."

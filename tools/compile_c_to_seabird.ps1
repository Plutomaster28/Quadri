param(
    [Parameter(Mandatory = $true)]
    [string]$InputFile,
    [string]$OutputPrefix = "",
    [string]$BuildRoot = "C:/tmp/llvm-build-seabird",
    [string]$TargetTriple = "seabird64-unknown-none",
    [string]$CPU = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$input = (Resolve-Path $InputFile).Path
if (-not $OutputPrefix) {
    $OutputPrefix = Join-Path (Split-Path $input) `
        ([IO.Path]::GetFileNameWithoutExtension($input))
}
$outputDirectory = Split-Path -Parent $OutputPrefix
if ($outputDirectory) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$clang = Join-Path $BuildRoot "bin/clang.exe"
$llc = Join-Path $BuildRoot "bin/llc.exe"
$llvmMc = Join-Path $BuildRoot "bin/llvm-mc.exe"
$objcopy = Join-Path $BuildRoot "bin/llvm-objcopy.exe"
if (-not (Test-Path $objcopy)) {
    $objcopy = "C:/msys64/ucrt64/bin/llvm-objcopy.exe"
}
$cpuArgs = @()
if ($CPU) {
    $cpuArgs += "-mcpu=$CPU"
}
$ir = "$OutputPrefix.ll"
$assembly = "$OutputPrefix.s"
$object = "$OutputPrefix.o"
$binary = "$OutputPrefix.bin"

& $clang -target $TargetTriple @cpuArgs -O2 -ffreestanding -fno-builtin -fno-ident `
    -S -emit-llvm $input -o $ir
if ($LASTEXITCODE -ne 0) { throw "Clang C-to-LLVM-IR compilation failed" }
& $llc "-mtriple=$TargetTriple" @cpuArgs -O2 -filetype=asm $ir -o $assembly
if ($LASTEXITCODE -ne 0) { throw "SeaBird LLVM instruction selection failed" }
& $llvmMc "-triple=$TargetTriple" @cpuArgs -filetype=obj $assembly -o $object
if ($LASTEXITCODE -ne 0) { throw "SeaBird ELF assembly failed" }
& $objcopy -O binary --only-section=.text $object $binary
if ($LASTEXITCODE -ne 0) { throw "SeaBird raw binary extraction failed" }

Write-Host "LLVM IR:  $ir"
Write-Host "Assembly: $assembly"
Write-Host "ELF64:    $object"
Write-Host "Raw text: $binary"

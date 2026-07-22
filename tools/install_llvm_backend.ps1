param(
    [Parameter(Mandatory = $true)]
    [string]$LlvmProjectRoot
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$llvmRoot = Join-Path (Resolve-Path $LlvmProjectRoot) "llvm"
$targetRoot = Join-Path $llvmRoot "lib/Target/SeaBird"
$clangTargetRoot = Join-Path (Resolve-Path $LlvmProjectRoot) `
    "clang/lib/Basic/Targets"
$patches = @(
    (Join-Path $root "llvm/patches/llvm-22-seabird-triple.patch"),
    (Join-Path $root "llvm/patches/llvm-22-seabird-elf.patch"),
    (Join-Path $root "llvm/patches/clang-22-seabird-target.patch")
)

if (-not (Test-Path (Join-Path $llvmRoot "CMakeLists.txt"))) {
    throw "Not an llvm-project source root: $LlvmProjectRoot"
}
if (-not (Test-Path $clangTargetRoot)) {
    throw "LLVM checkout does not contain Clang sources: $LlvmProjectRoot"
}

New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null
Copy-Item -Path (Join-Path $root "llvm/SeaBird/*") -Destination $targetRoot `
    -Recurse -Force
Copy-Item -Path (Join-Path $root "clang/SeaBird.h") `
    -Destination $clangTargetRoot -Force
Copy-Item -Path (Join-Path $root "clang/SeaBird.cpp") `
    -Destination $clangTargetRoot -Force

$safeRoot = $LlvmProjectRoot -replace '\\','/'
foreach ($patch in $patches) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    git -c "safe.directory=$safeRoot" -C $LlvmProjectRoot apply --reverse --check $patch `
        2>$null
    $alreadyApplied = $LASTEXITCODE -eq 0

    if (-not $alreadyApplied) {
        git -c "safe.directory=$safeRoot" -C $LlvmProjectRoot apply --check $patch `
            2>$null
        $canApply = $LASTEXITCODE -eq 0
        if (-not $canApply) {
            $ErrorActionPreference = $savedPreference
            throw "LLVM patch neither applies cleanly nor is already applied: $patch"
        }
        git -c "safe.directory=$safeRoot" -C $LlvmProjectRoot apply $patch
        if ($LASTEXITCODE -ne 0) {
            $ErrorActionPreference = $savedPreference
            throw "failed to apply LLVM patch: $patch"
        }
    }
    $ErrorActionPreference = $savedPreference
}

Write-Host "Installed SeaBird LLVM backend and Clang target sources"

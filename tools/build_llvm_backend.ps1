param(
    [string]$LlvmProjectRoot = "C:/tmp/llvm-project-22.1.4",
    [string]$BuildRoot = "C:/tmp/llvm-build-seabird",
    [int]$Jobs = 8
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$cmake = "C:/msys64/ucrt64/bin/cmake.exe"
$clang = "C:/msys64/ucrt64/bin/clang.exe"
$clangxx = "C:/msys64/ucrt64/bin/clang++.exe"

& (Join-Path $PSScriptRoot "install_llvm_backend.ps1") `
    -LlvmProjectRoot $LlvmProjectRoot

$arguments = @(
    "-S", (Join-Path $LlvmProjectRoot "llvm"),
    "-B", $BuildRoot,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_C_COMPILER=$clang",
    "-DCMAKE_CXX_COMPILER=$clangxx",
    "-DLLVM_TARGETS_TO_BUILD=",
    "-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SeaBird",
    "-DLLVM_ENABLE_PROJECTS=clang",
    "-DLLVM_INCLUDE_TESTS=OFF",
    "-DLLVM_INCLUDE_BENCHMARKS=OFF",
    "-DLLVM_INCLUDE_EXAMPLES=OFF",
    "-DLLVM_ENABLE_ZLIB=OFF",
    "-DLLVM_ENABLE_ZSTD=OFF",
    "-DLLVM_ENABLE_LIBXML2=OFF",
    "-DLLVM_ENABLE_TERMINFO=OFF"
)

& $cmake @arguments
if ($LASTEXITCODE -ne 0) { throw "LLVM CMake configuration failed" }
& $cmake --build $BuildRoot --target clang llc llvm-mc llvm-readobj llvm-objdump `
    llvm-nm llvm-ar llvm-ranlib llvm-objcopy llvm-strip -j $Jobs
if ($LASTEXITCODE -ne 0) { throw "SeaBird llvm-mc build failed" }

Write-Host "Built the SeaBird compiler, MC tools, object inspector, symbol, archive, conversion, and strip utilities in $BuildRoot/bin"

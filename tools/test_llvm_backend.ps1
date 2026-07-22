param(
    [string]$BuildRoot = "C:/tmp/llvm-build-seabird"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$clang = Join-Path $BuildRoot "bin/clang.exe"
$llvmMc = Join-Path $BuildRoot "bin/llvm-mc.exe"
$llc = Join-Path $BuildRoot "bin/llc.exe"
$llvmReadobj = Join-Path $BuildRoot "bin/llvm-readobj.exe"
$llvmObjdump = Join-Path $BuildRoot "bin/llvm-objdump.exe"
$llvmNm = Join-Path $BuildRoot "bin/llvm-nm.exe"
$llvmAr = Join-Path $BuildRoot "bin/llvm-ar.exe"
$llvmRanlib = Join-Path $BuildRoot "bin/llvm-ranlib.exe"
$llvmObjcopy = Join-Path $BuildRoot "bin/llvm-objcopy.exe"
$llvmStrip = Join-Path $BuildRoot "bin/llvm-strip.exe"
$smoke = Join-Path $root "tests/llvm/smoke.s"
$roundtrip = Join-Path $root "tests/llvm/encoding-roundtrip.s"
$external = Join-Path $root "tests/llvm/external-reloc.s"
$tritium = Join-Path $root "tests/llvm/tritium-mc.s"
$tritiumExcluded = Join-Path $root "tests/llvm/tritium-excluded.s"
$tritiumDirectives = Join-Path $root "tests/llvm/tritium-directives.s"
$tritiumCodegen = Join-Path $root "tests/llvm/tritium-codegen.ll"
$tritiumAtomics = Join-Path $root "tests/llvm/tritium-atomics.ll"
$fpCore = Join-Path $root "tests/llvm/fp-core.ll"
$simdCore = Join-Path $root "tests/llvm/simd-core.ll"
$sysxExtended = Join-Path $root "tests/llvm/sysx-extended.s"
$fpxExtended = Join-Path $root "tests/llvm/fpx-extended.s"
$fpxCore = Join-Path $root "tests/llvm/fpx-core.ll"
$fp32Core = Join-Path $root "tests/llvm/fp32-core.s"
$fp128Core = Join-Path $root "tests/llvm/fp128-core.s"
$cryptoExtended = Join-Path $root "tests/llvm/crypto-extended.s"
$dspExtended = Join-Path $root "tests/llvm/dsp-extended.s"
$avxExtended = Join-Path $root "tests/llvm/avx-extended.s"
$txnAtomics = Join-Path $root "tests/llvm/txn-atomics.s"
$gpAtomics = Join-Path $root "tests/llvm/gp-atomics.ll"
$assemblerDirectives = Join-Path $root "tests/llvm/assembler-directives.s"
$globalAddress = Join-Path $root "tests/llvm/global-address.ll"
$gpCompareSelect = Join-Path $root "tests/llvm/gp-compare-select.ll"
$fpConstants = Join-Path $root "tests/llvm/fp-constants.ll"
$registerSpills = Join-Path $root "tests/llvm/register-spills.ll"
$gpIntegerCore = Join-Path $root "tests/llvm/gp-integer-core.ll"
$dynamicAlloca = Join-Path $root "tests/llvm/dynamic-alloca.ll"
$largeSwitch = Join-Path $root "tests/llvm/large-switch.ll"
$varargs = Join-Path $root "tests/llvm/varargs.ll"
$varargsSB32 = Join-Path $root "tests/llvm/varargs-sb32.ll"
$nativeVarargs = Join-Path $root "tests/llvm/c-native-varargs.c"
$nativeTritium = Join-Path $root "tests/llvm/c-native-tritium.c"
$nativeFP32 = Join-Path $root "tests/llvm/c-native-fp32.c"
$nativeAggregate = Join-Path $root "tests/llvm/c-native-aggregate.c"
$nativeFPCompare = Join-Path $root "tests/llvm/c-native-fp-compare.c"
$nativeFPUnsigned = Join-Path $root "tests/llvm/c-native-fp-unsigned.c"
$nativeFP128 = Join-Path $root "tests/llvm/c-native-fp128.c"
$testOutput = Join-Path $root "build/llvm-tests"

if (-not (Test-Path $clang)) { throw "missing SeaBird clang: $clang" }
if (-not (Test-Path $llvmMc)) { throw "missing SeaBird llvm-mc: $llvmMc" }
if (-not (Test-Path $llc)) { throw "missing SeaBird llc: $llc" }
if (-not (Test-Path $llvmReadobj)) { throw "missing llvm-readobj: $llvmReadobj" }
if (-not (Test-Path $llvmObjdump)) { throw "missing llvm-objdump: $llvmObjdump" }
if (-not (Test-Path $llvmNm)) { throw "missing llvm-nm: $llvmNm" }
if (-not (Test-Path $llvmAr)) { throw "missing llvm-ar: $llvmAr" }
if (-not (Test-Path $llvmRanlib)) { throw "missing llvm-ranlib: $llvmRanlib" }
if (-not (Test-Path $llvmObjcopy)) { throw "missing llvm-objcopy: $llvmObjcopy" }
if (-not (Test-Path $llvmStrip)) { throw "missing llvm-strip: $llvmStrip" }
New-Item -ItemType Directory -Force -Path $testOutput | Out-Null

$version = & $llvmMc --version 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or
    $version -notmatch "seabird32 - SeaBird 32-bit" -or
    $version -notmatch "seabird64 - SeaBird 64-bit") {
    throw "SeaBird is not registered in llvm-mc"
}

$tritiumObject = Join-Path $testOutput "tritium-mc.o"
& $llvmMc -triple=seabird32-unknown-none -mcpu=tritium-v1 -filetype=obj `
    $tritium -o $tritiumObject
if ($LASTEXITCODE -ne 0) { throw "failed to emit Tritium SeaBird32 ELF" }
$tritiumHeaders = & $llvmReadobj --file-headers $tritiumObject `
    2>&1 | Out-String
foreach ($expected in "Format: elf32-seabird", "Arch: seabird32",
    "Machine: 0x5342") {
    if ($tritiumHeaders -notmatch [regex]::Escape($expected)) {
        throw "Tritium ELF inspection is missing '$expected'"
    }
}
$tritiumDisassembly = & $llvmObjdump -d $tritiumObject 2>&1 | Out-String
foreach ($expected in "mov`t", "movzx`t", "movsx`t", "movhi`t", "movlo`t",
    "movswp`t", "ldi`t", "ld`t", "st`t", "leas`t", "xchg`t",
    "add`t", "ldw`t", "stw`t", "cmp`t", "slt`t",
    "mul`t", "mulh`t", "div`t", "mod`t", "umul`t", "udiv`t",
    "neg`t", "inc`t", "dec`t", "not`t", "abs`t", "clz`t", "ctz`t",
    "popc`t",
    "addi`t", "subi`t", "muli`t", "divi`t", "modi`t", "cmpi`t", "tsti`t",
    "cmps`t", "cmpu`t", "tst`t",
    "jo`t", "jno`t", "js`t", "jns`t", "jzr`t", "jnzr`t", "jmpa`t",
    "brr`t", "trap`t", "yield",
    "push`t", "pop`t", "enter`t", "leave", "pushf", "popf",
    "hlt", "reset", "rdcr`t", "wrcr`t", "iret", "cli", "sti", "wfi",
    "rdtime`t", "rdts`t", "sleep`t",
    "query`t", "eoi`t", "savectx`t", "loadctx`t", "getcpl`t",
    "cmpxchg`t", "atadd`t", "atsub`t", "atand`t", "ator`t", "atxor`t",
    "ll`t", "sc`t",
    "fence", "lfence", "sfence", "mfence",
    "adds`t", "addu`t", "subs`t", "subu`t", "nand`t", "nor`t", "xnor`t",
    "rol`t", "ror`t", "bset`t", "bclr`t", "btog`t", "btst`t", "mask`t",
    "ext`t", "max`t", "min`t", "sgt`t", "jne`t", "ret`t") {
    if ($tritiumDisassembly -notmatch [regex]::Escape($expected)) {
        throw "Tritium disassembly is missing '$expected'"
    }
}

$tritiumDirectiveObject = Join-Path $testOutput "tritium-directives.o"
& $llvmMc -triple=seabird32-unknown-none -filetype=obj `
    $tritiumDirectives -o $tritiumDirectiveObject
if ($LASTEXITCODE -ne 0) {
    throw "Tritium inline .arch/.cpu/.mode directives failed"
}

$tritiumExcludedOutput = & $llvmMc -triple=seabird32-unknown-none `
    -mcpu=tritium-v1 -filetype=asm $tritiumExcluded 2>&1 | Out-String
if ($LASTEXITCODE -eq 0 -or
    $tritiumExcludedOutput -notmatch "instruction requires an unavailable feature") {
    throw "Tritium accepted an excluded GP-only or optional-extension instruction"
}

$tritiumCodegenAssembly = Join-Path $testOutput "tritium-codegen.s"
$tritiumCodegenObject = Join-Path $testOutput "tritium-codegen.o"
& $llc -mtriple=seabird32-unknown-none -mcpu=tritium-v1 -O2 `
    -filetype=asm $tritiumCodegen -o $tritiumCodegenAssembly
if ($LASTEXITCODE -ne 0) { throw "Tritium SelectionDAG lowering failed" }
$tritiumGenerated = Get-Content $tritiumCodegenAssembly -Raw
foreach ($expected in "tritium_mix:", "add`tr0, r1", "xor`tr0, r2",
    "tritium_unsigned_less:", "slt`tr0, r1", "tritium_equal:",
    "tritium_div32:", "div`tr0, r1", "tritium_mod32:", "mod`tr0, r1",
    "tritium_udiv32:", "udiv`tr0, r1", "tritium_adds32:", "adds`tr0, r1",
    "tritium_addi32:", "addi`tr0, 123456", "tritium_subi32:",
    "addi`tr0, -2345", "tritium_muli32:", "muli`tr0, 37",
    "tritium_divi32:", "divi`tr0, 7", "tritium_modi32:", "modi`tr0, 7",
    "tritium_mask32:", "mask`tr0, r0, 65535",
    "tritium_cmpi32:", "cmpi`tr0, 123456",
    "tritium_neg32:", "neg`tr0", "tritium_inc32:", "inc`tr0",
    "tritium_dec32:", "dec`tr0", "tritium_not32:", "not`tr0",
    "tritium_abs32:", "abs`tr0, r0", "tritium_clz32:", "clz`tr0, r0",
    "tritium_ctz32:", "ctz`tr0, r0", "tritium_popc32:", "popc`tr0, r0",
    "tritium_addu32:", "addu`tr0, r1", "tritium_subs32:", "subs`tr0, r1",
    "tritium_subu32:", "subu`tr0, r1", "tritium_rol32:", "rol`tr0, r1",
    "tritium_ror32:", "ror`tr0, r1", "tritium_max32:", "max`tr0, r1",
    "tritium_min32:", "min`tr0, r1",
    "tritium_add64:", "tritium_sub64:", "tritium_call64:",
    "tritium_load64:", "tritium_store64:",
    "tritium_shl64:", "tritium_lshr64:", "tritium_ashr64:",
    "tritium_mul64:", "mulh`t", "tritium_select64_uge:",
    "tritium_branch64_uge:", "jne`t", "tritium_udiv64:", "call`t__udivdi3",
    "tritium_sdiv64:", "call`t__divdi3", "tritium_urem64:",
    "call`t__umoddi3", "tritium_srem64:", "call`t__moddi3",
    "movi`tr0, 305419896", "ldw`tr0, [r0]", "stw`t[r0], r1",
    "movi`tr30, 16", "call`ttritium_external_eleven") {
    if ($tritiumGenerated -notmatch [regex]::Escape($expected)) {
        throw "Tritium-generated assembly is missing '$expected'"
    }
}
& $llvmMc -triple=seabird32-unknown-none -mcpu=tritium-v1 -filetype=obj `
    $tritiumCodegenAssembly -o $tritiumCodegenObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble Tritium-generated code" }
$tritiumCodegenHeaders = & $llvmReadobj --file-headers --relocations `
    $tritiumCodegenObject 2>&1 | Out-String
foreach ($expected in "Format: elf32-seabird", "AddressSize: 32bit",
    "R_SB_PCREL32 __udivdi3", "R_SB_PCREL32 __divdi3",
    "R_SB_PCREL32 __umoddi3", "R_SB_PCREL32 __moddi3",
    "R_SB_PCREL32 tritium_external64",
    "R_SB_PCREL32 tritium_external_eleven") {
    if ($tritiumCodegenHeaders -notmatch [regex]::Escape($expected)) {
        throw "Tritium-generated ELF inspection is missing '$expected'"
    }
}
$tritiumCodegenDisassembly = & $llvmObjdump -d $tritiumCodegenObject `
    2>&1 | Out-String
if ($tritiumCodegenDisassembly -notmatch
    [regex]::Escape("01 c0 78 56 34 12")) {
    throw "Tritium MOVI did not use its 32-bit immediate encoding"
}
if ($tritiumCodegenDisassembly -notmatch "5a [0-9a-f]{2}.*slt") {
    throw "Tritium software-pair carry lowering did not encode SLT"
}

$tritiumAtomicsAssembly = Join-Path $testOutput "tritium-atomics.s"
$tritiumAtomicsObject = Join-Path $testOutput "tritium-atomics.o"
& $llc -mtriple=seabird32-unknown-none -mcpu=tritium-v1 -O2 `
    -filetype=asm $tritiumAtomics -o $tritiumAtomicsAssembly
if ($LASTEXITCODE -ne 0) { throw "Tritium atomic lowering failed" }
$tritiumAtomicGenerated = Get-Content $tritiumAtomicsAssembly -Raw
foreach ($expected in "atadd`tr1, [r0]", "atsub`tr1, [r0]",
    "atand`tr1, [r0]", "ator`tr1, [r0]", "atxor`tr1, [r0]",
    "cmpxchg`tr1, r2, [r0]", "tritium_cmpxchg_success:",
    "tritium_atomic_load:", "ldw`tr0, [r0]",
    "tritium_atomic_store:", "stw`t[r0], r1", "mfence") {
    if ($tritiumAtomicGenerated -notmatch [regex]::Escape($expected)) {
        throw "Tritium atomic assembly is missing '$expected'"
    }
}
& $llvmMc -triple=seabird32-unknown-none -mcpu=tritium-v1 -filetype=obj `
    $tritiumAtomicsAssembly -o $tritiumAtomicsObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble Tritium atomic code" }
$tritiumAtomicDisassembly = & $llvmObjdump -d $tritiumAtomicsObject `
    2>&1 | Out-String
foreach ($expected in "atadd`t", "atsub`t", "atand`t", "ator`t", "atxor`t",
    "cmpxchg`t", "ldw`t", "stw`t", "mfence") {
    if ($tritiumAtomicDisassembly -notmatch [regex]::Escape($expected)) {
        throw "Tritium atomic disassembly is missing '$expected'"
    }
}

$assembly = & $llvmMc -triple=seabird64-unknown-none -filetype=asm $smoke `
    2>&1 | Out-String
if ($LASTEXITCODE -ne 0) { throw "SeaBird assembly smoke test failed`n$assembly" }
foreach ($expected in "mov`t", "add`t", "movi`t", "cmp`t", "je`t", "xor`t", "ret`t") {
    if ($assembly -notmatch [regex]::Escape($expected)) {
        throw "SeaBird assembly output is missing '$expected'"
    }
}

$encoding = & $llvmMc -triple=seabird64-unknown-none -show-encoding $roundtrip `
    2>&1 | Out-String
if ($LASTEXITCODE -ne 0) { throw "SeaBird encoding test failed`n$encoding" }
foreach ($expected in
    "[0xfe,0x80,0x00,0xf8,0x0b]",
    "[0xfe,0x80,0x01,0xc0,0x04,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11]",
    "[0xfe,0x80,0x20,0xc8,0x05]") {
    if ($encoding -notmatch [regex]::Escape($expected)) {
        throw "SeaBird encoding output is missing '$expected'"
    }
}

$roundtripObject = Join-Path $testOutput "roundtrip.o"
$externalObject = Join-Path $testOutput "external.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj $roundtrip `
    -o $roundtripObject
if ($LASTEXITCODE -ne 0) { throw "failed to emit SeaBird round-trip ELF" }
& $llvmMc -triple=seabird64-unknown-none -filetype=obj $external `
    -o $externalObject
if ($LASTEXITCODE -ne 0) { throw "failed to emit SeaBird relocation ELF" }

$headers = & $llvmReadobj --file-headers --relocations $externalObject `
    2>&1 | Out-String
foreach ($expected in "Format: elf64-seabird", "Arch: seabird64",
    "Machine: 0x5342", "R_SB_PCREL32 external_target 0xFFFFFFFFFFFFFFFC",
    "R_SB_ABS16 external_data 0x0", "R_SB_ABS32 external_data 0x0",
    "R_SB_ABS64 external_data 0x0") {
    if ($headers -notmatch [regex]::Escape($expected)) {
        throw "SeaBird ELF inspection is missing '$expected'"
    }
}

$disassembly = & $llvmObjdump -d $roundtripObject 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) { throw "SeaBird disassembly failed`n$disassembly" }
foreach ($expected in "mov`t", "movi`t", "add`t", "sub`t", "and`t", "or`t",
    "xor`t", "cmp`t", "je`t", "jne`t", "jmp`t", "call`t", "ret`t",
    "push`t", "pop`t", "pusha", "popa", "enter`t4294967296", "leave",
    "pushf", "popf", "pushq`t", "popq`t", "sysret", "getpid`t",
    "gettid`t", "pdep`t", "pext`t", "lzcnt`t", "tzcnt`t", "popcnt`t",
    "bextr`t", "binsert`t", "blsi`t", "blsmsk`t", "blsr`t", "rorx`t",
    "shlx`t", "shrx`t", "andn`t", "bzhi`t", "tzcntv`t", "prefetch`t",
    "flush`t", "invic`t", "invdc`t", "ldx`t", "stx`t", "ldn`t", "stn`t",
    "cpyb`t", "cpyw`t", "memfill`t", "ldp`t", "stp`t",
    "fsqrt`t", "fcmp`t", "fneg`t", "fabs`t",
    "vdiv`t", "vshl`t", "vshr`t", "vdup`t", "vabs`t", "vmax`t", "vmin`t") {
    if ($disassembly -notmatch [regex]::Escape($expected)) {
        throw "SeaBird disassembly is missing '$expected'"
    }
}

$fpCoreAssembly = Join-Path $testOutput "fp-core.s"
$fpCoreObject = Join-Path $testOutput "fp-core.o"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $fpCore -o $fpCoreAssembly
if ($LASTEXITCODE -ne 0) { throw "scalar FP core lowering failed" }
$fpCoreGenerated = Get-Content $fpCoreAssembly -Raw
foreach ($expected in "fsqrt`tv0, v0", "fneg`tv0, v0", "fabs`tv0, v0") {
    if ($fpCoreGenerated -notmatch [regex]::Escape($expected)) {
        throw "scalar FP core assembly is missing '$expected'"
    }
}
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $fpCoreAssembly -o $fpCoreObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble scalar FP core code" }

$simdCoreAssembly = Join-Path $testOutput "simd-core.s"
$simdCoreObject = Join-Path $testOutput "simd-core.o"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $simdCore -o $simdCoreAssembly
if ($LASTEXITCODE -ne 0) { throw "SIMD core lowering failed" }
$simdCoreGenerated = Get-Content $simdCoreAssembly -Raw
foreach ($expected in "vdiv`tv0, v0, v1", "vabs`tv0, v0",
    "vmax`tv0, v0, v1", "vmin`tv0, v0, v1") {
    if ($simdCoreGenerated -notmatch [regex]::Escape($expected)) {
        throw "SIMD core assembly is missing '$expected'"
    }
}
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $simdCoreAssembly -o $simdCoreObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble SIMD core code" }

$sysxObject = Join-Path $testOutput "sysx-extended.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $sysxExtended -o $sysxObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble extended SYSX code" }
$sysxDisassembly = & $llvmObjdump -d $sysxObject 2>&1 | Out-String
foreach ($expected in "in`t", "out`t", "xsave`t", "xrstor`t", "isync",
    "invtlb`t", "invtlbasid`t", "invtlball", "sendipi`t", "vmenter`t",
    "vmresume`t", "vmread`t", "vmwrite`t", "endbr", "wrss`t", "rdpmc`t",
    "rngget`t", "setmode`t") {
    if ($sysxDisassembly -notmatch [regex]::Escape($expected)) {
        throw "extended SYSX disassembly is missing '$expected'"
    }
}

$fpxObject = Join-Path $testOutput "fpx-extended.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $fpxExtended -o $fpxObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble FPX code" }
$fpxDisassembly = & $llvmObjdump -d $fpxObject 2>&1 | Out-String
foreach ($expected in "fmadd`t", "fmsub`t", "fnmadd`t", "fnmsub`t",
    "fmin`t", "fmax`t", "frecip`t", "frsqrt`t", "frnd`t", "frndz`t",
    "fcvt.s2d`t", "fcvt.d2s`t", "fcvtint`t", "fclass`t", "fchs`t",
    "ftest`t", "fld`t", "fst`t") {
    if ($fpxDisassembly -notmatch [regex]::Escape($expected)) {
        throw "FPX disassembly is missing '$expected'"
    }
}

$fpxCoreAssembly = Join-Path $testOutput "fpx-core.s"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $fpxCore -o $fpxCoreAssembly
if ($LASTEXITCODE -ne 0) { throw "FPX core lowering failed" }
$fpxCoreGenerated = Get-Content $fpxCoreAssembly -Raw
foreach ($expected in "fmadd`tv0, v0, v1, v2", "fmsub`tv0, v0, v1, v2",
    "fmin`tv0, v0, v1", "fmax`tv0, v0, v1") {
    if ($fpxCoreGenerated -notmatch [regex]::Escape($expected)) {
        throw "FPX core assembly is missing '$expected'"
    }
}

$cryptoObject = Join-Path $testOutput "crypto-extended.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $cryptoExtended -o $cryptoObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble CRYPTO code" }
$cryptoDisassembly = & $llvmObjdump -d $cryptoObject 2>&1 | Out-String
foreach ($expected in "aesenc`t", "aesdec`t", "aesimc`t", "pclmulqdq`t",
    "ghash`t", "sha1_msg1`t", "sha1_msg2`t", "sha256_sig0`t",
    "sha256_sig1`t", "poly_mul`t") {
    if ($cryptoDisassembly -notmatch [regex]::Escape($expected)) {
        throw "CRYPTO disassembly is missing '$expected'"
    }
}

$dspObject = Join-Path $testOutput "dsp-extended.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $dspExtended -o $dspObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble DSP code" }
$dspDisassembly = & $llvmObjdump -d $dspObject 2>&1 | Out-String
foreach ($expected in "mac32`t", "mac64`t", "macs`t", "msub`t",
    "satsub`t", "satadd`t", "fixed_mul`t", "fixed_add`t", "cmplx_mul`t",
    "bitrev`t", "pack_sat`t", "unpack_exp`t", "clamp`t", "accumulate`t",
    "dotp`t", "sumdotp`t", "rshift_round`t", "lshift`t", "sllv`t",
    "srlv`t", "srav`t", "rndq`t", "clz_fast`t", "tzcnt_fast`t",
    "mad32`t") {
    if ($dspDisassembly -notmatch [regex]::Escape($expected)) {
        throw "DSP disassembly is missing '$expected'"
    }
}

$avxObject = Join-Path $testOutput "avx-extended.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $avxExtended -o $avxObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble AVX code" }
$avxDisassembly = & $llvmObjdump -d $avxObject 2>&1 | Out-String
foreach ($expected in "vfmadd`t", "vfmsub`t", "vfnmadd`t",
    "vfmadd_round`t", "vpermute`t", "vshuffle`t", "vblend`t", "vtest`t",
    "vpmadd`t", "vreduce_add`t", "vreduce_mul`t", "vcompare_lt`t",
    "vcompare_gt`t", "vinsert`t", "vextract`t", "vgather`t",
    "vscatter`t", "valign`t", "vbswap`t", "vpack`t", "vunpack`t",
    "vpmul`t", "vperm2`t", "vcompress`t", "vexpand`t", "vround`t",
    "vrecip_est`t", "vrsqrt_est`t", "vfmadd_sub`t", "vzeroupper",
    "vzeroall", "vpmax`t", "vpmin`t", "vgatherq`t", "vscatterq`t",
    "vfpclass`t", "vreduce_max`t", "vreduce_min`t", "vmuladdsub`t") {
    if ($avxDisassembly -notmatch [regex]::Escape($expected)) {
        throw "AVX disassembly is missing '$expected'"
    }
}

$txnAtomicsObject = Join-Path $testOutput "txn-atomics.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $txnAtomics -o $txnAtomicsObject
if ($LASTEXITCODE -ne 0) { throw "failed to assemble TXN/ATOMICS code" }
$txnAtomicsDisassembly = & $llvmObjdump -d $txnAtomicsObject 2>&1 | Out-String
foreach ($expected in "xbegin`t", "xbegina`t", "xend", "xabort`t255",
    "xabort`tr31", "xtest`t", "xstatus`t", "xchg`t", "xchg128`t") {
    if ($txnAtomicsDisassembly -notmatch [regex]::Escape($expected)) {
        throw "TXN/ATOMICS disassembly is missing '$expected'"
    }
}

$gpAtomicsAssembly = Join-Path $testOutput "gp-atomics.s"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $gpAtomics -o $gpAtomicsAssembly
if ($LASTEXITCODE -ne 0) { throw "GP atomic exchange lowering failed" }
$gpAtomicsGenerated = Get-Content $gpAtomicsAssembly -Raw
if ($gpAtomicsGenerated -notmatch [regex]::Escape("xchg`tr1, [r0]")) {
    throw "GP atomic exchange assembly is missing native XCHG"
}

$directiveObject = Join-Path $testOutput "assembler-directives.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $assemblerDirectives -o $directiveObject
if ($LASTEXITCODE -ne 0) { throw "SeaBird assembler directive test failed" }
$directiveInspection = & $llvmReadobj --sections --symbols --relocations `
    $directiveObject 2>&1 | Out-String
foreach ($expected in ".rodata", ".data", ".bss", "directive_entry",
    "directive_data", "directive_pointer", "directive_zeros",
    "R_SB_ABS64 directive_data") {
    if ($directiveInspection -notmatch [regex]::Escape($expected)) {
        throw "Assembler directive object is missing '$expected'"
    }
}

foreach ($target in @(
    @{ Name = "sb64"; Triple = "seabird64-unknown-none"; CPU = @();
       Format = "elf64-seabird"; Reloc = "R_SB_ABS64" },
    @{ Name = "sb32"; Triple = "seabird32-unknown-none";
       CPU = @("-mcpu=tritium-v1"); Format = "elf32-seabird";
       Reloc = "R_SB_ABS32" })) {
    $globalObject = Join-Path $testOutput "global-address-$($target.Name).o"
    & $llc "-mtriple=$($target.Triple)" @($target.CPU) -O2 -filetype=obj `
        $globalAddress -o $globalObject
    if ($LASTEXITCODE -ne 0) {
        throw "Global-address lowering failed for $($target.Name)"
    }
    $globalInspection = & $llvmReadobj --file-headers --relocations `
        $globalObject 2>&1 | Out-String
    foreach ($expected in $target.Format, $target.Reloc, "local_data",
        "external_data") {
        if ($globalInspection -notmatch [regex]::Escape($expected)) {
            throw "Global-address object for $($target.Name) is missing '$expected'"
        }
    }
}

$varargsObject = Join-Path $testOutput "varargs.o"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=obj `
    $varargs -o $varargsObject
if ($LASTEXITCODE -ne 0) {
    throw "SB64 variadic object emission failed"
}
$varargsLinkOutput = & python (Join-Path $PSScriptRoot "link_seabird.py") `
    --entry call_sum_three -o (Join-Path $testOutput "varargs-linked.bin") `
    $varargsObject 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or
    $varargsLinkOutput -notmatch "call_sum_three" -or
    $varargsLinkOutput -notmatch "call_sum_two_fp") {
    throw "Variadic constant-subsection static link failed"
}

$nativeVarargsAssembly = Join-Path $testOutput "c-native-varargs.s"
$nativeVarargsObject = Join-Path $testOutput "c-native-varargs.o"
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeVarargs -o $nativeVarargsAssembly
if ($LASTEXITCODE -ne 0) { throw "native SB64 Clang assembly emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeVarargs -o $nativeVarargsObject
if ($LASTEXITCODE -ne 0) { throw "native SB64 Clang object emission failed" }
$nativeVarargsGenerated = Get-Content $nativeVarargsAssembly -Raw
foreach ($expected in "seabird_native_sum:", "seabird_native_call:",
    "seabird_native_fp_sum:", "seabird_native_fp_call:", "call`t",
    "ldw`t", "shl`t", "sar`t", "fld`t", "fadd`t") {
    if ($nativeVarargsGenerated -notmatch [regex]::Escape($expected)) {
        throw "native SB64 Clang assembly is missing '$expected'"
    }
}
$nativeVarargsHeaders = & $llvmReadobj --file-headers $nativeVarargsObject `
    2>&1 | Out-String
foreach ($expected in "Format: elf64-seabird", "Arch: seabird64",
    "Machine: 0x5342") {
    if ($nativeVarargsHeaders -notmatch [regex]::Escape($expected)) {
        throw "native SB64 Clang object is missing '$expected'"
    }
}

$nativeTritiumAssembly = Join-Path $testOutput "c-native-tritium.s"
$nativeTritiumObject = Join-Path $testOutput "c-native-tritium.o"
& $clang -target seabird32-unknown-none -mcpu=tritium-v1 -O2 `
    -ffreestanding -fno-builtin -S $nativeTritium -o $nativeTritiumAssembly
if ($LASTEXITCODE -ne 0) { throw "native Tritium Clang assembly emission failed" }
& $clang -target seabird32-unknown-none -mcpu=tritium-v1 -O2 `
    -ffreestanding -fno-builtin -c $nativeTritium -o $nativeTritiumObject
if ($LASTEXITCODE -ne 0) { throw "native Tritium Clang object emission failed" }
$nativeTritiumGenerated = Get-Content $nativeTritiumAssembly -Raw
foreach ($expected in "tritium_native_sum:", "tritium_native_call:",
    "call`t", "ldw`t", "stw`t") {
    if ($nativeTritiumGenerated -notmatch [regex]::Escape($expected)) {
        throw "native Tritium Clang assembly is missing '$expected'"
    }
}
$nativeTritiumHeaders = & $llvmReadobj --file-headers $nativeTritiumObject `
    2>&1 | Out-String
foreach ($expected in "Format: elf32-seabird", "Arch: seabird32",
    "Machine: 0x5342") {
    if ($nativeTritiumHeaders -notmatch [regex]::Escape($expected)) {
        throw "native Tritium Clang object is missing '$expected'"
    }
}

$nativeFP32Assembly = Join-Path $testOutput "c-native-fp32.s"
$nativeFP32Object = Join-Path $testOutput "c-native-fp32.o"
$fp32CoreObject = Join-Path $testOutput "fp32-core.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $fp32Core -o $fp32CoreObject
if ($LASTEXITCODE -ne 0) { throw "binary32 assembly fixture failed" }
$fp32CoreDisassembly = & $llvmObjdump -d $fp32CoreObject 2>&1 | Out-String
foreach ($expected in "fadd.s`t", "fsub.s`t", "fmul.s`t", "fdiv.s`t",
    "fneg.s`t", "fabs.s`t", "fsqrt.s`t", "fcmp.s`t", "fmadd.s`t", "fmsub.s`t",
    "fmin.s`t", "fmax.s`t", "fld.s`t", "fst.s`t", "fcvti.s`t",
    "fcvts.s`t") {
    if ($fp32CoreDisassembly -notmatch [regex]::Escape($expected)) {
        throw "binary32 fixture disassembly is missing '$expected'"
    }
}
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeFP32 -o $nativeFP32Assembly
if ($LASTEXITCODE -ne 0) { throw "native SB64 binary32 assembly emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeFP32 -o $nativeFP32Object
if ($LASTEXITCODE -ne 0) { throw "native SB64 binary32 object emission failed" }
$nativeFP32Generated = Get-Content $nativeFP32Assembly -Raw
foreach ($expected in "seabird_f32_arithmetic:", "seabird_f32_memory:",
    "seabird_f32_call:", "seabird_f32_arithmetic_call:",
    "seabird_f32_stack_call:",
    "seabird_f32_from_long:",
    "seabird_f32_to_long:", "seabird_f32_widen:",
    "seabird_f32_narrow:", "fadd.s`t", "fmul.s`t", "fld.s`t", "fst.s`t",
    "fcvti.s`t", "fcvts.s`t", "fcvt.s2d`t", "fcvt.d2s`t", "call`t") {
    if ($nativeFP32Generated -notmatch [regex]::Escape($expected)) {
        throw "native SB64 binary32 assembly is missing '$expected'"
    }
}
$nativeFP32RoundTripObject = Join-Path $testOutput "c-native-fp32-roundtrip.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $nativeFP32Assembly -o $nativeFP32RoundTripObject
if ($LASTEXITCODE -ne 0) {
    throw "compiler-generated binary32 assembly did not round-trip"
}
$nativeFP32Disassembly = & $llvmObjdump -d $nativeFP32RoundTripObject `
    2>&1 | Out-String
foreach ($expected in "fadd.s`t", "fmul.s`t", "fld.s`t", "fst.s`t",
    "fcvti.s`t", "fcvts.s`t") {
    if ($nativeFP32Disassembly -notmatch [regex]::Escape($expected)) {
        throw "binary32 round-trip disassembly is missing '$expected'"
    }
}
$nativeFP32Headers = & $llvmReadobj --file-headers $nativeFP32Object `
    2>&1 | Out-String
foreach ($expected in "Format: elf64-seabird", "Arch: seabird64",
    "Machine: 0x5342") {
    if ($nativeFP32Headers -notmatch [regex]::Escape($expected)) {
        throw "native SB64 binary32 object is missing '$expected'"
    }
}

$nativeAggregateAssembly = Join-Path $testOutput "c-native-aggregate.s"
$nativeAggregateObject = Join-Path $testOutput "c-native-aggregate.o"
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeAggregate -o $nativeAggregateAssembly
if ($LASTEXITCODE -ne 0) { throw "native SB64 aggregate assembly emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeAggregate -o $nativeAggregateObject
if ($LASTEXITCODE -ne 0) { throw "native SB64 aggregate object emission failed" }
$nativeAggregateGenerated = Get-Content $nativeAggregateAssembly -Raw
foreach ($expected in "seabird_pair_make:", "seabird_pair_sum:",
    "seabird_pair_call:", "seabird_float_pair_make:",
    "seabird_float_pair_sum:", "seabird_float_pair_call:",
    "stq`t", "ldq`t", "fst.s`t", "fld.s`t", "fadd.s`t", "call`t") {
    if ($nativeAggregateGenerated -notmatch [regex]::Escape($expected)) {
        throw "native SB64 aggregate assembly is missing '$expected'"
    }
}
$nativeAggregateRoundTripObject = Join-Path $testOutput "c-native-aggregate-roundtrip.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $nativeAggregateAssembly -o $nativeAggregateRoundTripObject
if ($LASTEXITCODE -ne 0) {
    throw "compiler-generated aggregate assembly did not round-trip"
}

$nativeFPCompareAssembly = Join-Path $testOutput "c-native-fp-compare.s"
$nativeFPCompareObject = Join-Path $testOutput "c-native-fp-compare.o"
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeFPCompare -o $nativeFPCompareAssembly
if ($LASTEXITCODE -ne 0) { throw "native scalar FP comparison emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeFPCompare -o $nativeFPCompareObject
if ($LASTEXITCODE -ne 0) { throw "native scalar FP comparison object failed" }
$nativeFPCompareGenerated = Get-Content $nativeFPCompareAssembly -Raw
foreach ($expected in "seabird_f32_lt:", "seabird_f32_eq:",
    "seabird_f32_unordered:", "seabird_f32_select:", "seabird_f64_gt:",
    "seabird_f64_ne:", "seabird_f64_select:",
    "seabird_fp_compare_call:", "fcmp.s`t", "fcmp`t", "je`t", "jc`t",
    "jnzr`t") {
    if ($nativeFPCompareGenerated -notmatch [regex]::Escape($expected)) {
        throw "native scalar FP comparison assembly is missing '$expected'"
    }
}
$nativeFPCompareRoundTripObject = Join-Path $testOutput "c-native-fp-compare-roundtrip.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $nativeFPCompareAssembly -o $nativeFPCompareRoundTripObject
if ($LASTEXITCODE -ne 0) {
    throw "compiler-generated scalar FP comparison assembly did not round-trip"
}

$nativeFPUnsignedAssembly = Join-Path $testOutput "c-native-fp-unsigned.s"
$nativeFPUnsignedObject = Join-Path $testOutput "c-native-fp-unsigned.o"
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeFPUnsigned -o $nativeFPUnsignedAssembly
if ($LASTEXITCODE -ne 0) { throw "native unsigned FP conversion emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeFPUnsigned -o $nativeFPUnsignedObject
if ($LASTEXITCODE -ne 0) { throw "native unsigned FP conversion object failed" }
$nativeFPUnsignedGenerated = Get-Content $nativeFPUnsignedAssembly -Raw
foreach ($expected in "seabird_u64_to_f32:", "seabird_u64_to_f64:",
    "seabird_f32_to_u64:", "seabird_f64_to_u64:",
    "seabird_fp_unsigned_call:", "fcvti.s`t", "fcvti`t", "fcvts.s`t",
    "fcvts`t", "fcmp.s`t", "fcmp`t") {
    if ($nativeFPUnsignedGenerated -notmatch [regex]::Escape($expected)) {
        throw "native unsigned FP conversion assembly is missing '$expected'"
    }
}
$nativeFPUnsignedRoundTripObject = Join-Path $testOutput "c-native-fp-unsigned-roundtrip.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $nativeFPUnsignedAssembly -o $nativeFPUnsignedRoundTripObject
if ($LASTEXITCODE -ne 0) {
    throw "compiler-generated unsigned FP conversion assembly did not round-trip"
}

$fp128CoreObject = Join-Path $testOutput "fp128-core.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $fp128Core -o $fp128CoreObject
if ($LASTEXITCODE -ne 0) { throw "binary128 assembly fixture failed" }
$fp128CoreDisassembly = & $llvmObjdump -d $fp128CoreObject 2>&1 | Out-String
foreach ($expected in "fadd.q`t", "fsub.q`t", "fmul.q`t", "fdiv.q`t",
    "fneg.q`t", "fabs.q`t", "fsqrt.q`t", "fcmp.q`t", "fmadd.q`t",
    "fmsub.q`t", "fmin.q`t", "fmax.q`t", "fld.q`t", "fst.q`t",
    "fcvti.q`t", "fcvts.q`t") {
    if ($fp128CoreDisassembly -notmatch [regex]::Escape($expected)) {
        throw "binary128 fixture disassembly is missing '$expected'"
    }
}
$nativeFP128Assembly = Join-Path $testOutput "c-native-fp128.s"
$nativeFP128Object = Join-Path $testOutput "c-native-fp128.o"
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -S $nativeFP128 -o $nativeFP128Assembly
if ($LASTEXITCODE -ne 0) { throw "native binary128 assembly emission failed" }
& $clang -target seabird64-unknown-none -O2 -ffreestanding -fno-builtin `
    -c $nativeFP128 -o $nativeFP128Object
if ($LASTEXITCODE -ne 0) { throw "native binary128 object emission failed" }
$nativeFP128Generated = Get-Content $nativeFP128Assembly -Raw
foreach ($expected in "sb_quad_math:", "sb_quad_to_long:",
    "sb_quad_select:", "sb_quad_ninth:", "sb_native_fp128_wrapper:", "fadd.q`t", "fld.q`t",
    "fst.q`t", "fcvts.q`t", "fcmp.q`t", "jnzr`t") {
    if ($nativeFP128Generated -notmatch [regex]::Escape($expected)) {
        throw "native binary128 assembly is missing '$expected'"
    }
}
$nativeFP128RoundTripObject = Join-Path $testOutput "c-native-fp128-roundtrip.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    $nativeFP128Assembly -o $nativeFP128RoundTripObject
if ($LASTEXITCODE -ne 0) {
    throw "compiler-generated binary128 assembly did not round-trip"
}

$nativeFP128Symbols = & $llvmNm $nativeFP128Object 2>&1 | Out-String
foreach ($expected in "sb_quad_math", "sb_quad_select",
    "sb_native_fp128_wrapper") {
    if ($nativeFP128Symbols -notmatch [regex]::Escape($expected)) {
        throw "SeaBird symbol listing is missing '$expected'"
    }
}
$utilityArchive = Join-Path $testOutput "libseabird-fp128.a"
& $llvmAr rcs $utilityArchive $nativeFP128Object
if ($LASTEXITCODE -ne 0) { throw "SeaBird static archive creation failed" }
& $llvmRanlib $utilityArchive
if ($LASTEXITCODE -ne 0) { throw "SeaBird archive indexing failed" }
$archiveMembers = & $llvmAr t $utilityArchive 2>&1 | Out-String
if ($archiveMembers -notmatch "c-native-fp128.o") {
    throw "SeaBird archive member listing failed"
}
$utilityRaw = Join-Path $testOutput "c-native-fp128.raw"
$utilityHex = Join-Path $testOutput "c-native-fp128.hex"
& $llvmObjcopy -O binary $nativeFP128Object $utilityRaw
if ($LASTEXITCODE -ne 0 -or (Get-Item $utilityRaw).Length -eq 0) {
    throw "SeaBird raw-binary conversion failed"
}
& $llvmObjcopy -O ihex $nativeFP128Object $utilityHex
if ($LASTEXITCODE -ne 0 -or (Get-Content $utilityHex -First 1) -notmatch '^:') {
    throw "SeaBird Intel HEX conversion failed"
}
$utilityStripped = Join-Path $testOutput "c-native-fp128-stripped.o"
& $llvmStrip --strip-debug $nativeFP128Object -o $utilityStripped
if ($LASTEXITCODE -ne 0) { throw "SeaBird strip operation failed" }
$strippedHeader = & $llvmReadobj --file-headers $utilityStripped 2>&1 | Out-String
if ($strippedHeader -notmatch "Format: elf64-seabird") {
    throw "stripped SeaBird ELF object was not readable"
}

$gpCompareAssembly = Join-Path $testOutput "gp-compare-select.s"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $gpCompareSelect -o $gpCompareAssembly
if ($LASTEXITCODE -ne 0) {
    throw "GP comparison/select lowering failed"
}
$gpCompareGenerated = Get-Content $gpCompareAssembly -Raw
foreach ($expected in "gp_signed_less:", "gp_unsigned_less:", "gp_equal:",
    "gp_select:", "gp_compare_select:", "slt`t", "xor`t", "and`t") {
    if ($gpCompareGenerated -notmatch [regex]::Escape($expected)) {
        throw "GP comparison/select assembly is missing '$expected'"
    }
}

$fpConstantsObject = Join-Path $testOutput "fp-constants.o"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=obj `
    $fpConstants -o $fpConstantsObject
if ($LASTEXITCODE -ne 0) {
    throw "FP constant-pool lowering failed"
}
$fpConstantsInspection = & $llvmReadobj --sections --relocations `
    $fpConstantsObject 2>&1 | Out-String
foreach ($expected in ".rodata.cst8", "R_SB_ABS64") {
    if ($fpConstantsInspection -notmatch [regex]::Escape($expected)) {
        throw "FP constant-pool object is missing '$expected'"
    }
}

$registerSpillsObject = Join-Path $testOutput "register-spills.o"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=obj `
    $registerSpills -o $registerSpillsObject
if ($LASTEXITCODE -ne 0) {
    throw "Register spill/reload lowering failed"
}
$registerSpillsDisassembly = & $llvmObjdump -d `
    $registerSpillsObject 2>&1 | Out-String
foreach ($expected in "stq`t", "ldq`t", "vst`t", "vld`t", "call`t") {
    if ($registerSpillsDisassembly -notmatch [regex]::Escape($expected)) {
        throw "Register spill/reload disassembly is missing '$expected'"
    }
}

$gpIntegerAssembly = Join-Path $testOutput "gp-integer-core.s"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $gpIntegerCore -o $gpIntegerAssembly
if ($LASTEXITCODE -ne 0) {
    throw "GP integer-core lowering failed"
}
$gpIntegerGenerated = Get-Content $gpIntegerAssembly -Raw
foreach ($expected in "gp_mul:", "gp_sdiv:", "gp_udiv:", "gp_srem:",
    "gp_urem:", "gp_abs:", "gp_clz:", "gp_ctz:", "gp_popcount:",
    "mul`t", "div`t", "udiv`t", "mod`t", "abs`t", "clz`t", "ctz`t",
    "popc`t") {
    if ($gpIntegerGenerated -notmatch [regex]::Escape($expected)) {
        throw "GP integer-core assembly is missing '$expected'"
    }
}

foreach ($target in @(
    @{ Name = "sb64"; Triple = "seabird64-unknown-none"; CPU = @();
       Store = "stq`t"; Load = "ldq`t" },
    @{ Name = "sb32"; Triple = "seabird32-unknown-none";
       CPU = @("-mcpu=tritium-v1"); Store = "stw`t"; Load = "ldw`t" })) {
    $dynamicAssembly = Join-Path $testOutput "dynamic-alloca-$($target.Name).s"
    & $llc "-mtriple=$($target.Triple)" @($target.CPU) -O2 -filetype=asm `
        $dynamicAlloca -o $dynamicAssembly
    if ($LASTEXITCODE -ne 0) {
        throw "Dynamic alloca lowering failed for $($target.Name)"
    }
    $dynamicGenerated = Get-Content $dynamicAssembly -Raw
    foreach ($expected in "dynamic_alloca:", "dynamic_alloca_call:",
        "dynamic_alloca_with_fixed:", "mov`tr6, r7", "mov`tr7, r6",
        $target.Store, $target.Load) {
        if ($dynamicGenerated -notmatch [regex]::Escape($expected)) {
            throw "Dynamic alloca for $($target.Name) is missing '$expected'"
        }
    }
}

$largeSwitchAssembly = Join-Path $testOutput "large-switch.s"
& $llc -mtriple=seabird64-unknown-none -O2 -filetype=asm `
    $largeSwitch -o $largeSwitchAssembly
if ($LASTEXITCODE -ne 0) {
    throw "Large switch lowering failed"
}
$largeSwitchGenerated = Get-Content $largeSwitchAssembly -Raw
foreach ($expected in "large_switch:", "cmp`t", "jg`t", "je`t") {
    if ($largeSwitchGenerated -notmatch [regex]::Escape($expected)) {
        throw "Large switch assembly is missing '$expected'"
    }
}
if ($largeSwitchGenerated -match "\.LJTI" -or
    $largeSwitchGenerated -match "br_jt") {
    throw "Large switch unexpectedly used an unsupported jump table"
}

foreach ($target in @(
    @{ Name = "sb64"; Triple = "seabird64-unknown-none"; CPU = @();
       Input = $varargs; Store = "stq`t"; Load = "ldq`t";
       Callee = "sum_three:"; Caller = "call_sum_three:" },
    @{ Name = "sb32"; Triple = "seabird32-unknown-none";
       CPU = @("-mcpu=tritium-v1"); Input = $varargsSB32;
       Store = "stw`t"; Load = "ldw`t";
       Callee = "sum_three32:"; Caller = "call_sum_three32:" })) {
    $varargsAssembly = Join-Path $testOutput "varargs-$($target.Name).s"
    & $llc "-mtriple=$($target.Triple)" @($target.CPU) -O2 -filetype=asm `
        $target.Input -o $varargsAssembly
    if ($LASTEXITCODE -ne 0) {
        throw "Variadic lowering failed for $($target.Name)"
    }
    $varargsGenerated = Get-Content $varargsAssembly -Raw
    foreach ($expected in $target.Callee, $target.Caller, "call`t",
        $target.Store, $target.Load) {
        if ($varargsGenerated -notmatch [regex]::Escape($expected)) {
            throw "Variadic $($target.Name) assembly is missing '$expected'"
        }
    }
}

foreach ($expected in "je`t15", "jne`t10", "jmp`t5", "call`t0") {
    if ($disassembly -notmatch [regex]::Escape($expected)) {
        throw "SeaBird resolved rel32 disassembly is missing '$expected'"
    }
}

$cPrefix = Join-Path $testOutput "c-smoke"
& (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
    -InputFile (Join-Path $root "tests/llvm/c-smoke.c") `
    -OutputPrefix $cPrefix -BuildRoot $BuildRoot
if ($LASTEXITCODE -ne 0) { throw "C-to-SeaBird pipeline failed" }

$cAssembly = Get-Content "$cPrefix.s" -Raw
foreach ($expected in "seabird_mix:", "add`tr1, r0", "xor`tr1, r2",
    "sub`tr1, r3", "mov`tr0, r1", "seabird_constant:",
    "movi`tr1, 1234605616436508552", "ret") {
    if ($cAssembly -notmatch [regex]::Escape($expected)) {
        throw "C-generated SeaBird assembly is missing '$expected'"
    }
}

$cDisassembly = & $llvmObjdump -d "$cPrefix.o" 2>&1 | Out-String
foreach ($expected in "file format elf64-seabird", "20 c8", "42 ca",
    "22 cb", "00 c1", "01 c1 88 77 66 55 44 33 22 11") {
    if ($cDisassembly -notmatch [regex]::Escape($expected)) {
        throw "C-generated SeaBird object is missing '$expected'"
    }
}
$rawHex = [BitConverter]::ToString([IO.File]::ReadAllBytes("$cPrefix.bin"))
$expectedHex = "20-C8-42-CA-22-CB-00-C1-60-01-C1-88-77-66-55-44-33-22-11-42-C1-60"
if ($rawHex -ne $expectedHex) {
    throw "unexpected C-generated SeaBird raw bytes: $rawHex"
}

foreach ($name in "c-call", "c-branch", "c-memory", "c-stack", "c-ordered",
    "c-indirect", "c-stack-args", "c-fp-vector") {
    $prefix = Join-Path $testOutput $name
    & (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
        -InputFile (Join-Path $root "tests/llvm/$name.c") `
        -OutputPrefix $prefix -BuildRoot $BuildRoot
    if ($LASTEXITCODE -ne 0) { throw "$name C-to-SeaBird pipeline failed" }
}
$vectorMemoryPrefix = Join-Path $testOutput "c-vector-memory"
& (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
    -InputFile (Join-Path $root "tests/llvm/c-vector-memory.c") `
    -OutputPrefix $vectorMemoryPrefix -BuildRoot $BuildRoot
if ($LASTEXITCODE -ne 0) { throw "vector memory compilation failed" }
$fpConvertPrefix = Join-Path $testOutput "c-fp-convert"
& (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
    -InputFile (Join-Path $root "tests/llvm/c-fp-convert.c") `
    -OutputPrefix $fpConvertPrefix -BuildRoot $BuildRoot
if ($LASTEXITCODE -ne 0) { throw "FP conversion compilation failed" }

$fpMemoryPrefix = Join-Path $testOutput "c-fp-memory"
& (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
    -InputFile (Join-Path $root "tests/llvm/c-fp-memory.c") `
    -OutputPrefix $fpMemoryPrefix -BuildRoot $BuildRoot
if ($LASTEXITCODE -ne 0) { throw "scalar FP memory compilation failed" }

foreach ($name in "c-ordered", "c-stack-args") {
    & python (Join-Path $PSScriptRoot "link_seabird.py") `
        -o (Join-Path $testOutput "$name-linked.bin") `
        (Join-Path $testOutput "$name.o")
    if ($LASTEXITCODE -ne 0) { throw "$name static link failed" }
}

$fpAssembly = Get-Content (Join-Path $testOutput "c-fp-vector.s") -Raw
foreach ($expected in "fadd`tv0, v0, v1", "fmul`tv0, v0, v2",
    "vadd`tv1, v1, v0", "vxor`tv0, v1, v0") {
    if ($fpAssembly -notmatch [regex]::Escape($expected)) {
        throw "C-generated FP/vector assembly is missing '$expected'"
    }
}

$sibObject = Join-Path $testOutput "sib-bases.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    (Join-Path $root "tests/llvm/sib-bases.s") -o $sibObject
if ($LASTEXITCODE -ne 0) { throw "SeaBird SIB base assembly failed" }
$sibDisassembly = & $llvmObjdump -d $sibObject 2>&1 | Out-String
foreach ($expected in "ldq`tr0, [r4]", "stq`t[r5], r1",
    "ldq`tr16, [r12]", "stq`t[r13], r17",
    "ldq`tr2, [r1 + r3*4 + 16]", "stq`t[r8 + r9*8 - 32], r10") {
    if ($sibDisassembly -notmatch [regex]::Escape($expected)) {
        throw "SeaBird SIB disassembly is missing '$expected'"
    }
}

$sectionObject = Join-Path $testOutput "sectioned-data.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    (Join-Path $root "tests/llvm/sectioned-data.s") -o $sectionObject
& python (Join-Path $PSScriptRoot "link_seabird.py") `
    -o (Join-Path $testOutput "sectioned-data.bin") $sectionObject
if ($LASTEXITCODE -ne 0) { throw "sectioned data link failed" }
$sectionBytes = [IO.File]::ReadAllBytes((Join-Path $testOutput "sectioned-data.bin"))
if ($sectionBytes.Length -ne 48 -or $sectionBytes[8] -ne 0x88 -or
    $sectionBytes[16] -ne 8 -or $sectionBytes[32] -ne 0) {
    throw "sectioned data, ABS64, or BSS layout is incorrect"
}
& python (Join-Path $PSScriptRoot "link_seabird.py") --format elf `
    --entry section_entry -o (Join-Path $testOutput "sectioned-data.elf") $sectionObject
$sectionHeaders = & $llvmReadobj --program-headers `
    (Join-Path $testOutput "sectioned-data.elf") 2>&1 | Out-String
foreach ($expected in "Flags [ (0x5)", "Flags [ (0x4)", "Flags [ (0x6)",
    "FileSize: 8", "MemSize: 32") {
    if ($sectionHeaders -notmatch [regex]::Escape($expected)) {
        throw "page-separated ELF is missing '$expected'"
    }
}

$tlsObject = Join-Path $testOutput "tls-data.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    (Join-Path $root "tests/llvm/tls-data.s") -o $tlsObject
& python (Join-Path $PSScriptRoot "link_seabird.py") --format elf `
    --entry tls_entry -o (Join-Path $testOutput "tls-data.elf") $tlsObject
if ($LASTEXITCODE -ne 0) { throw "static TLS link failed" }
$tlsHeaders = & $llvmReadobj --program-headers (Join-Path $testOutput "tls-data.elf") 2>&1 | Out-String
foreach ($expected in "PT_TLS", "FileSize: 8", "MemSize: 16") {
    if ($tlsHeaders -notmatch [regex]::Escape($expected)) {
        throw "static TLS ELF is missing '$expected'"
    }
}

$dynamicObject = Join-Path $testOutput "dynamic-relocs.o"
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    (Join-Path $root "tests/llvm/dynamic-relocs.s") -o $dynamicObject
& python (Join-Path $PSScriptRoot "link_seabird.py") --format elf `
    --entry dynamic_entry -o (Join-Path $testOutput "dynamic-relocs.elf") $dynamicObject
if ($LASTEXITCODE -ne 0) { throw "dynamic-compatible relocation link failed" }
$dynamicBytes = [IO.File]::ReadAllBytes((Join-Path $testOutput "dynamic-relocs.elf"))
if ([BitConverter]::ToUInt64($dynamicBytes, 0x2000) -ne 0x11234 -or
    [BitConverter]::ToUInt64($dynamicBytes, 0x2008) -ne 0x10001 -or
    [BitConverter]::ToUInt64($dynamicBytes, 0x2010) -ne 0x10001) {
    throw "RELATIVE/GLOB_DAT/JUMP_SLOT relocation values are incorrect"
}

$stackAssembly = Get-Content (Join-Path $testOutput "c-stack.s") -Raw
foreach ($expected in "movi`tr30, 16", "sub`tr7, r30", "stq`t[r30], r0",
    "ldq`tr0, [r30]", "add`tr7, r30") {
    if ($stackAssembly -notmatch [regex]::Escape($expected)) {
        throw "C-generated stack assembly is missing '$expected'"
    }
}

$memoryDisassembly = & $llvmObjdump -d (Join-Path $testOutput "c-memory.o") 2>&1 | Out-String
foreach ($expected in "16 00", "ldq`tr0, [r0]", "1a 08", "stq`t[r0], r1") {
    if ($memoryDisassembly -notmatch [regex]::Escape($expected)) {
        throw "C-generated memory object is missing '$expected'"
    }
}

foreach ($part in "caller", "callee") {
    $prefix = Join-Path $testOutput "c-link-$part"
    & (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
        -InputFile (Join-Path $root "tests/llvm/c-link-$part.c") `
        -OutputPrefix $prefix -BuildRoot $BuildRoot
    if ($LASTEXITCODE -ne 0) { throw "cross-object $part compilation failed" }
}
& python (Join-Path $PSScriptRoot "link_seabird.py") `
    -o (Join-Path $testOutput "c-linked.bin") `
    (Join-Path $testOutput "c-link-caller.o") `
    (Join-Path $testOutput "c-link-callee.o")
if ($LASTEXITCODE -ne 0) { throw "SeaBird static link failed" }
$linked = [IO.File]::ReadAllBytes((Join-Path $testOutput "c-linked.bin"))
if ($linked.Length -ne 33 -or $linked[11] -ne 13 -or $linked[12] -ne 0) {
    throw "SeaBird static linker did not resolve the external PCREL32 call"
}

foreach ($unit in @(
    @{ Name = "crt0"; Source = (Join-Path $root "runtime/crt0.c") },
    @{ Name = "hosted-main"; Source = (Join-Path $root "tests/llvm/hosted-main.c") })) {
    & (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
        -InputFile $unit.Source -OutputPrefix (Join-Path $testOutput $unit.Name) `
        -BuildRoot $BuildRoot
    if ($LASTEXITCODE -ne 0) { throw "$($unit.Name) compilation failed" }
}
& $llvmMc -triple=seabird64-unknown-none -filetype=obj `
    (Join-Path $root "runtime/syscalls.s") -o (Join-Path $testOutput "syscalls.o")
if ($LASTEXITCODE -ne 0) { throw "hosted syscall runtime assembly failed" }
& python (Join-Path $PSScriptRoot "link_seabird.py") --format elf --entry _start `
    -o (Join-Path $testOutput "hosted.elf") `
    (Join-Path $testOutput "crt0.o") (Join-Path $testOutput "hosted-main.o") `
    (Join-Path $testOutput "syscalls.o")
if ($LASTEXITCODE -ne 0) { throw "hosted ELF link failed" }
$hostedHeaders = & $llvmReadobj --file-headers --program-headers `
    (Join-Path $testOutput "hosted.elf") 2>&1 | Out-String
foreach ($expected in "Type: Executable", "Entry: 0x10000", "PT_LOAD", "PF_X") {
    if ($hostedHeaders -notmatch [regex]::Escape($expected)) {
        throw "hosted ELF is missing '$expected'"
    }
}

foreach ($unit in @(
    @{ Name = "string"; Source = (Join-Path $root "runtime/string.c") },
    @{ Name = "hosted-libc-main"; Source = (Join-Path $root "tests/llvm/hosted-libc-main.c") })) {
    & (Join-Path $PSScriptRoot "compile_c_to_seabird.ps1") `
        -InputFile $unit.Source -OutputPrefix (Join-Path $testOutput $unit.Name) `
        -BuildRoot $BuildRoot
    if ($LASTEXITCODE -ne 0) { throw "$($unit.Name) compilation failed" }
}
& python (Join-Path $PSScriptRoot "link_seabird.py") --format elf --entry _start `
    -o (Join-Path $testOutput "hosted-libc.elf") `
    (Join-Path $testOutput "crt0.o") (Join-Path $testOutput "hosted-libc-main.o") `
    (Join-Path $testOutput "string.o")
if ($LASTEXITCODE -ne 0) { throw "hosted libc ELF link failed" }

Write-Host "SeaBird C-to-LLVM-to-assembly-to-binary pipeline passed."

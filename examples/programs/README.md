# SeaBird Compiler Exercise Programs

This directory contains ten freestanding C programs used to exercise the
SeaBird64 compiler, assembler, linker, object tools, console ABI, and reference
model together.

| Program | Primary coverage |
|---|---|
| `add-two-numbers.c` | Console input, addition, output |
| `simple-calculator.c` | Character input, branches, add/subtract/multiply/divide |
| `count-to-n.c` | Looping, comparison, increment |
| `factorial.c` | Range branches, loop, multiplication |
| `number-guessing-game.c` | Repeated input and ordered branches |
| `even-or-odd.c` | Remainder and conditional output |
| `bmi-calculator.c` | Multiply, divide, compound conditions |
| `array-sum-find-max.c` | Stack memory, indexed loads, loop, maximum |
| `password-checker.c` | Equality comparison |
| `rock-paper-scissors.c` | Input validation and multi-way decisions |

Build every program with the official `marlin` SDK:

```sh
python3 tools/build_example_programs.py \
  --llvm-build /path/to/seabird-sdk-v1.0.0-marlin-linux-x86_64
```

Artifacts are written under `build/example-programs/<program>/`:

- `.ll`: LLVM IR
- `.s`: generated SeaBird assembly
- `.o`: relocatable SeaBird ELF object
- `.elf`: linked SeaBird ELF executable
- `.bin`: linked flat binary
- `.hex`: address-oriented readable hex dump
- `.ihex`: Intel HEX image
- `.disasm.txt`: object disassembly with relocations
- `.object.txt`: ELF headers, sections, symbols, and relocations

Build the reference model and execute the regression matrix:

```sh
g++ -std=c++17 -O2 src/seabird_ref.cpp -o build/seabird-ref
python3 tools/test_example_programs.py \
  --reference build/seabird-ref
```

Run one program with scripted inputs:

```sh
build/seabird-ref --console \
  build/example-programs/simple-calculator/simple-calculator.bin \
  20 '*' 3
```

The development-console syscall interface used by these programs is documented
in `runtime/seabird_console.h`. It is an early emulator/monitor ABI, not yet a
final operating-system syscall contract.

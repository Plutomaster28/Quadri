// SeaBird v3 exposes all 32 GPRs in every mode. Tetra and Dragonet use the
// same physical registers with distinct LLVM value types.

class SeaBirdReg<bits<16> number, string name> : Register<name> {
  let Namespace = "SeaBird";
  let HWEncoding = number;
}

foreach I = 0-31 in {
  def R#I : SeaBirdReg<I, "r"#I>, DwarfRegNum<[I]>;
}

// IP is an architectural, non-allocatable pseudo-register used for DWARF's
// return-address column. FLAGS follows the ratified binary-interface mapping.
def IP : SeaBirdReg<32, "ip">, DwarfRegNum<[32]>;
def FLAGS : SeaBirdReg<33, "flags">, DwarfRegNum<[33]>;
// Internal code-generation sentinel for the M-type "no index" encoding.
// Keeping it distinct from architectural R4 prevents a false data dependency
// when R4 simultaneously carries an ABI argument.
def NOIDX : SeaBirdReg<4, "noidx">;

foreach I = 0-31 in {
  def V#I : SeaBirdReg<!add(I, 64), "v"#I>, DwarfRegNum<[!add(I, 64)]>;
}

def GPR32 : RegisterClass<"SeaBird", [i32], 32, (add (sequence "R%u", 0, 31))>;
def GPR64 : RegisterClass<"SeaBird", [i64], 64, (add (sequence "R%u", 0, 31))>;
def FPR32 : RegisterClass<"SeaBird", [f32], 32, (add (sequence "V%u", 0, 31))>;
def FPR64 : RegisterClass<"SeaBird", [f64], 64, (add (sequence "V%u", 0, 31))>;
def FPR128 : RegisterClass<"SeaBird", [f128], 128, (add (sequence "V%u", 0, 31))>;
def VR128 : RegisterClass<"SeaBird", [v2i64], 128, (add (sequence "V%u", 0, 31))>;

// ABI aliases are modeled as register variables in C++, not duplicate physical
// registers. R7 is SP and R6 is the optional frame pointer. CALL/RET use a
// memory return-address stack, so SeaBird has no architectural link register.

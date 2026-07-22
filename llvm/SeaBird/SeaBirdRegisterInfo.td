// SeaBird v3 exposes all 32 GPRs in every mode. Tetra and Dragonet use the
// same physical registers with distinct LLVM value types.

class SeaBirdReg<bits<16> number, string name> : Register<name> {
  let Namespace = "SeaBird";
  let HWEncoding = number;
}

foreach I = 0-31 in {
  def R#I : SeaBirdReg<I, "r"#I>, DwarfRegNum<[I]>;
}

def FLAGS : SeaBirdReg<32, "flags">, DwarfRegNum<[33]>;

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
// registers. R7 is SP; R6 is the optional frame pointer; R27 is the link register.

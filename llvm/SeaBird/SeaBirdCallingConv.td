// SB-System64, Volume 1. Integer/pointer arguments use R0-R5 then R16-R19.
// R0/R1 carry returns. Larger and excess values are assigned to the stack.

def CC_SeaBird : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i64],
      CCAssignToStack<8, 8>>>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[f64],
      CCAssignToStack<8, 8>>>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[f128],
      CCAssignToStack<16, 16>>>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[v2i64],
      CCAssignToStack<16, 16>>>>,
  CCIfType<[f32], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[f64], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[f128], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[v2i64], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[i64], CCAssignToReg<[R0, R1, R2, R3, R4, R5,
                                 R16, R17, R18, R19]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>,
  CCIfType<[f64], CCAssignToStack<8, 8>>,
  CCIfType<[f128], CCAssignToStack<16, 16>>,
  CCIfType<[v2i64], CCAssignToStack<16, 16>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>
]>;

def RetCC_SeaBird : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfType<[f32, f64, f128, v2i64], CCAssignToReg<[V0, V1]>>,
  CCIfType<[i64], CCAssignToReg<[R0, R1]>>
]>;

// SB32/Tritium uses the same register order with ILP32 values and 4-byte
// stack slots. The base profile is soft-float.
def CC_SeaBird32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i32],
      CCAssignToStack<4, 4>>>>,
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5,
                                 R16, R17, R18, R19]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def RetCC_SeaBird32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R0, R1]>>
]>;

// Windowed ABI. Calls write outgoing R24-R31; the callee observes the same
// storage as incoming R8-R15. A callee returns in R8/R9 and the caller observes
// those values in R24/R25 after RET restores its window.
def CC_SeaBirdWindowOut64 : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i64], CCAssignToStack<8, 8>>>>,
  CCIfType<[f32, f64, f128, v2i64], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[i64], CCAssignToReg<[R24, R25, R26, R27, R28, R29, R30, R31]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>,
  CCIfType<[f64, i64], CCAssignToStack<8, 8>>,
  CCIfType<[f128, v2i64], CCAssignToStack<16, 16>>
]>;
def CC_SeaBirdWindowIn64 : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i64], CCAssignToStack<8, 8>>>>,
  CCIfType<[f32, f64, f128, v2i64], CCAssignToReg<[V0, V1, V2, V3, V4, V5, V6, V7]>>,
  CCIfType<[i64], CCAssignToReg<[R8, R9, R10, R11, R12, R13, R14, R15]>>,
  CCIfType<[f32], CCAssignToStack<4, 4>>,
  CCIfType<[f64, i64], CCAssignToStack<8, 8>>,
  CCIfType<[f128, v2i64], CCAssignToStack<16, 16>>
]>;
def RetCC_SeaBirdWindowCallee64 : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfType<[f32, f64, f128, v2i64], CCAssignToReg<[V0, V1]>>,
  CCIfType<[i64], CCAssignToReg<[R8, R9]>>
]>;
def RetCC_SeaBirdWindowCaller64 : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
  CCIfType<[f32, f64, f128, v2i64], CCAssignToReg<[V0, V1]>>,
  CCIfType<[i64], CCAssignToReg<[R24, R25]>>
]>;

def CC_SeaBirdWindowOut32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i32], CCAssignToStack<4, 4>>>>,
  CCIfType<[i32], CCAssignToReg<[R24, R25, R26, R27, R28, R29, R30, R31]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;
def CC_SeaBirdWindowIn32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfVarArg<CCIfArgVarArg<CCIfType<[i32], CCAssignToStack<4, 4>>>>,
  CCIfType<[i32], CCAssignToReg<[R8, R9, R10, R11, R12, R13, R14, R15]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;
def RetCC_SeaBirdWindowCallee32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R8, R9]>>
]>;
def RetCC_SeaBirdWindowCaller32 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[R24, R25]>>
]>;

def CSR_SeaBird : CalleeSavedRegs<
  (add R6, R12, R13, R14, R15, R24, R25, R26, R27)>;
def CSR_SeaBirdWindowSpills : CalleeSavedRegs<(add R6)>;
def CSR_SeaBirdWindowPreserved : CalleeSavedRegs<
  (add R6, R8, R9, R10, R11, R12, R13, R14, R15,
       R16, R17, R18, R19, R20, R21, R22, R23)>;

    .text
    .globl fp32_core
fp32_core:
    fadd.s v0, v1, v2
    fsub.s v3, v4, v5
    fmul.s v6, v7, v8
    fdiv.s v9, v10, v11
    fneg.s v12, v13
    fabs.s v14, v15
    fsqrt.s v16, v17
    fcmp.s v0, v1
    fmadd.s v18, v19, v20, v21
    fmsub.s v22, v23, v24, v25
    fmin.s v26, v27, v28
    fmax.s v29, v30, v31
    fld.s v8, [r1]
    fst.s [r2 + 4], v9
    fcvti.s v10, r3
    fcvts.s r4, v11
    ret

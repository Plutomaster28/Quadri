#!/usr/bin/env python3
"""Build the SeaBird Tritium embedded subset profile PDF."""

from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import inch
from reportlab.platypus import (
    BaseDocTemplate, Frame, KeepTogether, PageBreak, PageTemplate, Paragraph,
    Spacer, Table, TableStyle,
)

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "output" / "pdf" / "seabird-tritium-embedded-profile.pdf"

NAVY = colors.HexColor("#132A3A")
TEAL = colors.HexColor("#087E8B")
GOLD = colors.HexColor("#E0A12B")
INK = colors.HexColor("#1B252C")
MUTED = colors.HexColor("#5D6B73")
PALE = colors.HexColor("#EAF3F4")
LIGHT = colors.HexColor("#F4F6F7")
RED = colors.HexColor("#A33A32")


class ProfileDoc(BaseDocTemplate):
    def __init__(self, filename):
        super().__init__(filename, pagesize=letter, rightMargin=0.62 * inch,
                         leftMargin=0.62 * inch, topMargin=0.70 * inch,
                         bottomMargin=0.60 * inch, title="SeaBird Tritium Embedded Profile")
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height,
                      id="normal")
        self.addPageTemplates(PageTemplate(id="profile", frames=frame,
                                           onPage=self.decorate))

    def decorate(self, canvas, doc):
        canvas.saveState()
        width, height = letter
        if doc.page == 1:
            canvas.setFillColor(NAVY)
            canvas.rect(0, 0, width, height, fill=1, stroke=0)
        else:
            canvas.setStrokeColor(colors.HexColor("#CBD5D9"))
            canvas.line(self.leftMargin, height - 0.46 * inch,
                        width - self.rightMargin, height - 0.46 * inch)
            canvas.setFont("Helvetica-Bold", 8)
            canvas.setFillColor(NAVY)
            canvas.drawString(self.leftMargin, height - 0.34 * inch,
                              "SEABIRD - TRITIUM EMBEDDED PROFILE")
            canvas.setFont("Helvetica", 8)
            canvas.setFillColor(MUTED)
            canvas.drawRightString(width - self.rightMargin, 0.34 * inch,
                                   f"Profile draft 1.0  |  Page {doc.page}")
        canvas.restoreState()


styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="CoverKicker", parent=styles["Normal"],
                          fontName="Helvetica-Bold", fontSize=10,
                          textColor=GOLD, leading=13, spaceAfter=18))
styles.add(ParagraphStyle(name="CoverTitle", parent=styles["Title"],
                          fontName="Helvetica-Bold", fontSize=34,
                          textColor=colors.white, leading=37, alignment=TA_LEFT,
                          spaceAfter=16))
styles.add(ParagraphStyle(name="CoverSub", parent=styles["Normal"],
                          fontName="Helvetica", fontSize=14,
                          textColor=colors.HexColor("#D6E4E8"), leading=20,
                          spaceAfter=20))
styles.add(ParagraphStyle(name="H1x", parent=styles["Heading1"],
                          fontName="Helvetica-Bold", fontSize=19,
                          textColor=NAVY, leading=23, spaceBefore=4,
                          spaceAfter=10))
styles.add(ParagraphStyle(name="H2x", parent=styles["Heading2"],
                          fontName="Helvetica-Bold", fontSize=12,
                          textColor=TEAL, leading=15, spaceBefore=10,
                          spaceAfter=5))
styles.add(ParagraphStyle(name="Bodyx", parent=styles["BodyText"],
                          fontName="Helvetica", fontSize=9.2,
                          textColor=INK, leading=13, spaceAfter=6))
styles.add(ParagraphStyle(name="Smallx", parent=styles["BodyText"],
                          fontName="Helvetica", fontSize=7.8,
                          textColor=INK, leading=10.4, spaceAfter=3))
styles.add(ParagraphStyle(name="HeaderSmall", parent=styles["Smallx"],
                          fontName="Helvetica-Bold", textColor=colors.white))
styles.add(ParagraphStyle(name="CoverTable", parent=styles["Smallx"],
                          textColor=colors.white))
styles.add(ParagraphStyle(name="CoverTableBold", parent=styles["CoverTable"],
                          fontName="Helvetica-Bold"))
styles.add(ParagraphStyle(name="Callout", parent=styles["BodyText"],
                          fontName="Helvetica-Bold", fontSize=10,
                          textColor=NAVY, backColor=PALE, borderColor=TEAL,
                          borderWidth=0.7, borderPadding=8, leading=14,
                          spaceBefore=5, spaceAfter=9))
styles.add(ParagraphStyle(name="Mono", parent=styles["BodyText"],
                          fontName="Courier", fontSize=7.6, leading=10,
                          textColor=INK, backColor=LIGHT, borderPadding=5))


def P(text, style="Bodyx"):
    return Paragraph(text, styles[style])


def heading(text, level=1):
    return P(text, "H1x" if level == 1 else "H2x")


def bullet(text):
    return Paragraph("&bull; " + text, ParagraphStyle(
        name="bullet-temp", parent=styles["Bodyx"], leftIndent=12,
        firstLineIndent=-8, spaceAfter=3))


def table(rows, widths, header=True, font=7.6):
    cooked = []
    for row_index, row in enumerate(rows):
        cell_style = styles["HeaderSmall"] if header and row_index == 0 else styles["Smallx"]
        cooked.append([cell if hasattr(cell, "wrap") else
                       Paragraph(str(cell), cell_style) for cell in row])
    t = Table(cooked, colWidths=widths, repeatRows=1 if header else 0,
              hAlign="LEFT")
    commands = [
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#BCC8CD")),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
        ("ROWBACKGROUNDS", (0, 1 if header else 0), (-1, -1),
         [colors.white, LIGHT]),
    ]
    if header:
        commands += [("BACKGROUND", (0, 0), (-1, 0), NAVY),
                     ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
                     ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold")]
    t.setStyle(TableStyle(commands))
    return t


def story():
    s = []
    s += [Spacer(1, 0.72 * inch), P("MEISEI PROCESSOR ARCHITECTURE", "CoverKicker"),
          P("SeaBird Tritium<br/>Embedded Profile", "CoverTitle"),
          P("A deterministic, safety-oriented 32-bit subset for the Tritium v1 processor",
            "CoverSub"), Spacer(1, 0.25 * inch)]
    cover_rows = [
        ["Profile", "SB-TRITIUM32-v1"], ["Parent ISA", "SeaBird architecture 3.2"],
        ["Execution mode", "Tetra (32-bit), little-endian"],
        ["Target core", "In-order, single-issue, 5-stage, dual lockstep"],
        ["Memory model", "TCM + scratchpad + region MPU; no cache or paging"],
        ["Document status", "Implementation profile draft 1.0 - July 2026"],
    ]
    cover_rows = [[P(label, "CoverTableBold"), P(value, "CoverTable")]
                  for label, value in cover_rows]
    cover = table(cover_rows, [1.35 * inch, 4.65 * inch], header=False)
    cover.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#19394B")),
                               ("TEXTCOLOR", (0, 0), (-1, -1), colors.white),
                               ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#3A6272")),
                               ("FONTNAME", (0, 0), (0, -1), "Helvetica-Bold")]))
    s += [cover, Spacer(1, 0.30 * inch),
          P("This profile is a conforming implementation subset, not a new instruction set. "
            "Software built for SB-TRITIUM32-v1 uses standard SeaBird encodings and can run on "
            "a full SeaBird Tetra implementation.", "CoverSub"), PageBreak()]

    s += [heading("1. Purpose and Conformance"),
          P("The Tritium profile defines the minimum SeaBird architectural behavior required "
            "for a deterministic embedded processor aimed at industrial, automotive, and "
            "aerospace control. It removes implementation features that complicate worst-case "
            "execution-time analysis while preserving the SeaBird programming model, object "
            "format, calling convention, exception precision, and toolchain compatibility."),
          P("A processor conforms when every required feature in this document is implemented "
            "exactly, excluded instructions trap as INVALID_OP, QUERY reports the profile "
            "accurately, and the implementation passes the profile test suite.", "Callout"),
          heading("1.1 Design Goals", 2)]
    for item in [
        "Bounded instruction and interrupt timing suitable for WCET analysis.",
        "A small verifiable in-order implementation with no speculative architectural effects.",
        "Source and binary compatibility with SeaBird Tetra software within the advertised feature set.",
        "First-class MPU, ECC, lockstep fault reporting, and deterministic recovery hooks.",
        "Enough integer, atomic, system, and debug functionality for bare metal and an MPU-aware RTOS.",
    ]: s.append(bullet(item))
    s += [heading("1.2 Profile Rules", 2),
          table([
              ["Term", "Requirement"],
              ["Required", "Must be implemented in every Tritium v1 core."],
              ["Optional", "May be implemented only when QUERY exposes the corresponding feature bit."],
              ["Excluded", "Must decode as unsupported and raise INVALID_OP before side effects."],
              ["Platform-defined", "Specified by the Tritium SoC/board binding, not by instruction semantics."],
          ], [1.25 * inch, 5.35 * inch]), PageBreak()]

    s += [heading("2. Hardware Mapping"),
          table([
              ["Tritium characteristic", "SeaBird profile consequence"],
              ["Tetra 32-bit mode", "32-bit GPR operations, pointers, IP, stack slots, and return addresses."],
              ["5-stage in-order pipeline", "Precise retirement; no younger write commits after an older fault."],
              ["No speculation", "No predictor, speculative fetch side-channel contract, OoO state, or rename state."],
              ["Fixed-latency multiply/divide", "MUL, DIV, MOD and signed/unsigned variants have published cycle counts."],
              ["TCM only", "No cache-control dependency; instruction/data timing comes from TCM and bus arbitration tables."],
              ["Region MPU", "Flat 32-bit addresses with region permission checks; paging instructions excluded."],
              ["Dual-core lockstep", "One logical CPU context; comparator checks architectural commit state."],
              ["SEC-DED ECC", "Corrected and uncorrectable events enter the machine-check/fault-supervisor path."],
          ], [2.0 * inch, 4.6 * inch]),
          heading("2.1 Conceptual Datapath", 2),
          table([
              ["FETCH", "DECODE", "EXECUTE", "MEMORY", "WRITEBACK / COMMIT"],
              ["TCM instruction read", "SeaBird variable-length decode", "ALU / fixed MUL-DIV / branch", "TCM, MPU, ECC, MMIO", "Lockstep compare, then architectural commit"],
          ], [1.22 * inch] * 5),
          P("Both lockstep lanes execute the complete pipeline. The comparator is placed at the "
            "architectural commit boundary. A mismatch prevents both lanes from committing the "
            "divergent result."),
          heading("2.2 Determinism Contract", 2)]
    for item in [
        "Each implemented instruction has a documented core-cycle latency and issue occupancy.",
        "No instruction latency depends on operand data except DIV/MOD only if a separately reported variable-latency option is enabled; Tritium v1 selects fixed latency.",
        "TCM accesses have fixed latency. Scratchpad and peripheral accesses use platform-published bounds.",
        "Interrupt recognition points, pipeline drain cost, and vector fetch cost are published in the timing annex.",
        "Frequency is fixed for v1; software-visible clock transitions are excluded.",
    ]: s.append(bullet(item))
    s.append(PageBreak())

    s += [heading("3. Architectural State"),
          table([
              ["State", "Tritium requirement", "Notes"],
              ["R0-R31", "Required, 32 bits each", "All architectural names remain visible; no reduced-register encoding."],
              ["IP", "Required, 32 bits", "Flat Tetra instruction address."],
              ["FLAGS", "Required", "CF, ZF, SF, OF and interrupt/privilege controls used by profile instructions."],
              ["R7 / SP", "Required", "Downward-growing stack; valid at all public boundaries."],
              ["R6 / FP", "Required as ordinary GPR", "Compiler may use it as frame pointer."],
              ["MPU control", "Required", "Region descriptors exposed through Tritium control registers."],
              ["Cycle/time counter", "Required", "Monotonic fixed-frequency counter for diagnostics and RTOS timing."],
              ["Fault supervisor state", "Required", "Reason, PC, status, timestamp, stage, retry-window count."],
              ["V0-V31, K0-K7, FPCR/FPSR", "Excluded in base v1", "Optional FP/SIMD derivative must advertise and save complete enabled state."],
              ["Paging/TLB/ASID state", "Excluded", "MPU replaces translation and page tables."],
              ["VM, transaction, CET state", "Excluded", "Virtualization, TSX-style transactions, shadow stack not in v1."],
          ], [1.35 * inch, 1.55 * inch, 3.7 * inch]),
          heading("3.1 Register and Encoding Compatibility", 2),
          P("Tritium implements all 32 SeaBird GPRs. OREX remains required because R8-R31 are "
            "architecturally visible in Tetra mode. ModR/M, SIB, signed displacement, and width "
            "prefix rules are unchanged. Subregister writes zero-extend into the 32-bit Tetra GPR."),
          P("A smaller physical implementation may use a compact register-file macro, but it may "
            "not hide registers or reinterpret their encodings.", "Callout"), PageBreak()]

    required_groups = [
        ["Data movement", "MOV MOVI MOVZX MOVSX MOVHI MOVLO MOVSWP LDI LDB LDH LDW STB STH STW LEA LEAS XCHG"],
        ["Arithmetic", "ADD ADDI SUB SUBI MUL MULI DIV DIVI MOD MODI NEG INC DEC MULH UMUL UDIV ADDS ADDU SUBS SUBU ABS CLZ CTZ POPC"],
        ["Logic and shifts", "AND OR XOR NOT NAND NOR XNOR SHL SHR SAR ROL ROR BSET BCLR BTOG BTST MASK EXT"],
        ["Compare", "CMP CMPI CMPS CMPU TST TSTI MAX MIN SLT SGT"],
        ["Control flow", "JMP JMPA CALL CALLA RET JE JNE JG JGE JL JLE JC JNC JO JNO JS JNS JZR JNZR BRR TRAP YIELD"],
        ["Stack", "PUSH POP ENTER LEAVE PUSHF POPF"],
        ["Atomic and ordering", "XCHG CMPXCHG ATADD ATSUB ATAND ATOR ATXOR LL SC FENCE LFENCE SFENCE MFENCE"],
        ["System", "HLT RESET RDCR WRCR IRET CLI STI WFI RDTIME RDTS SLEEP QUERY EOI SAVECTX LOADCTX GETCPL"],
    ]
    s += [heading("4. Required Instruction Subset"),
          P("The following families form the SB-TRITIUM32-v1 mandatory software target. Generic "
            "LD/ST select the active Tetra width; explicit LDQ/STQ and PUSHQ/POPQ are excluded from "
            "the base profile because they require 64-bit architectural data paths."),
          table([["Family", "Required mnemonics"]] + required_groups,
                [1.35 * inch, 5.25 * inch], font=7.1),
          heading("4.1 Required Semantic Properties", 2)]
    for item in [
        "Integer divide by zero raises the architected arithmetic exception before destination write.",
        "Unaligned ordinary accesses follow the parent ISA; the profile may require alignment through MPU/FLAGS policy for WCET simplicity.",
        "Atomic operations are fixed-latency for TCM and must be naturally aligned.",
        "FENCE variants have published completion bounds and order DMA-visible memory according to the SoC binding.",
        "QUERY returns zero for unsupported leaves and reports TRITIUM32, MPU, atomics, ECC diagnostics, and implemented counters.",
    ]: s.append(bullet(item))
    s.append(PageBreak())

    s += [heading("5. Optional and Excluded Features"),
          table([
              ["Feature", "v1 status", "Implementation behavior"],
              ["Scalar FP", "Optional derivative", "If absent, all FP/FPX opcodes raise INVALID_OP; compiler uses soft-float ABI."],
              ["SIMD/vector/mask", "Excluded", "V, K, VectorCtl, gather/scatter, AVX and matrix operations unavailable."],
              ["64/128-bit scalar data", "Excluded", "Dragonet/Droplet execution and explicit wide operations unavailable."],
              ["Paging/TLB/ASID", "Excluded", "INVTLB family unavailable; flat addresses pass directly to MPU."],
              ["Caches", "Excluded", "PREFETCH/FLUSH/INVIC/INVDC are unavailable or platform-defined no-ops only if parent semantics permit."],
              ["SMP/IPI", "Excluded", "Lockstep lanes are not software CPUs; SENDIPI unavailable."],
              ["Virtualization", "Excluded", "VM control instructions raise INVALID_OP."],
              ["Transactions", "Excluded", "Transactional memory opcodes raise INVALID_OP."],
              ["Cryptographic/DSP extensions", "Optional future SKU", "Must be fixed-latency or documented for WCET and independently advertised."],
              ["Dynamic frequency", "Excluded", "No runtime frequency-changing interface in v1."],
          ], [1.45 * inch, 1.25 * inch, 3.9 * inch]),
          P("Unsupported instructions must not silently emulate in privileged firmware unless the "
            "trap ABI explicitly identifies software emulation. Safety builds should treat an "
            "unexpected INVALID_OP as a software-integrity event."), PageBreak()]

    s += [heading("6. Memory and MPU Profile"),
          P("Tritium exposes a flat 32-bit physical address space. Instruction TCM, data TCM, the "
            "on-package scratchpad, and MMIO windows occupy platform-defined regions. Every fetch, "
            "load, and store is checked by the region MPU before commit."),
          heading("6.1 Minimum MPU Descriptor", 2),
          table([
              ["Field", "Minimum behavior"],
              ["BASE / LIMIT", "32-bit inclusive region bounds; naturally aligned implementation granule."],
              ["ENABLE", "Disabled descriptors never match."],
              ["R / W / X", "Independent permissions for load, store, and instruction fetch."],
              ["PRIV", "User, privileged, or both."],
              ["MEMTYPE", "TCM/scratchpad normal memory or strongly ordered device memory."],
              ["LOCK", "Prevents modification until reset when enabled by safety firmware."],
              ["ECC policy", "Correct-and-report or fail-stop behavior selected by platform safety policy."],
          ], [1.55 * inch, 5.05 * inch]),
          heading("6.2 Region Resolution", 2),
          P("Tritium v1 implements 16 regions through native SeaBird system registers 0x0300-0x033F. "
            "MPUCTRL is 0x0300 and MPUINFO is 0x0301. For region n, MPUBASE is 0x0310+3n, "
            "MPULIMIT is 0x0311+3n, and MPUATTR is 0x0312+3n. MPUATTR contains enable, R/W/X, "
            "privilege, memory-type, and lock fields as defined by the parent System-Register volume."),
          P("The highest-numbered matching region wins and default-deny must be enabled before "
            "unprivileged execution. Firmware clears the region enable bit, writes BASE and LIMIT, "
            "then publishes MPUATTR. A locked descriptor is immutable until reset. A complete "
            "multi-byte access must be permitted; a denied store has no partial effect."),
          heading("6.3 Memory Timing Classes", 2),
          table([
              ["Class", "Required timing publication"],
              ["Instruction TCM", "Fixed fetch latency and branch refill cost."],
              ["Data TCM", "Fixed load/store latency, including atomic operations."],
              ["Scratchpad die", "Read/write bounds including bridge arbitration and ECC correction."],
              ["MMIO", "Peripheral-specific maximum response or bus-timeout exception."],
              ["ECC event", "Correction penalty and uncorrectable-error delivery bound."],
          ], [1.65 * inch, 4.95 * inch]), PageBreak()]

    s += [heading("7. Interrupt and Exception Profile"),
          P("Tritium v1 uses fixed-priority vectored interrupts. The implementation target is a "
            "12-cycle worst-case interrupt latency, but the guaranteed figure becomes normative only "
            "after post-synthesis analysis and must be published per clock and memory configuration."),
          table([
              ["Property", "Requirement"],
              ["Priority", "Fixed and platform-published; ties resolved deterministically."],
              ["Nesting", "Disabled in v1 base profile. Future nesting must be an opt-in feature."],
              ["Recognition", "At an architecturally precise retirement boundary."],
              ["Saved state", "Return IP, FLAGS, privilege state, error/vector fields, and SP as defined by Tetra frame rules."],
              ["Vector table", "Naturally aligned, MPU-protected, initialized before interrupts are enabled."],
              ["EOI", "Required for external interrupt-controller completion."],
              ["Return", "IRET validates and atomically restores the saved context."],
          ], [1.5 * inch, 5.1 * inch]),
          heading("7.1 Exception Priority", 2),
          P("Tritium preserves the parent SeaBird exception order. Invalid encoding or unavailable "
            "feature is checked before privilege, address, alignment, translation/protection, "
            "arithmetic, and debug conditions. MPU denial occupies the protection/address stage; "
            "paging exceptions cannot occur because paging is absent."),
          heading("7.2 RTOS Expectations", 2)]
    for item in [
        "WFI enters a deterministic interrupt-wait state without changing clock frequency.",
        "CLI/STI and interrupt entry/return are bounded and serializing with respect to interrupt visibility.",
        "SAVECTX/LOADCTX may accelerate RTOS switching only when their exact memory image and timing are implemented.",
        "The RTOS programs an MPU context before returning to an unprivileged task.",
    ]: s.append(bullet(item))
    s.append(PageBreak())

    s += [heading("8. Lockstep and Fault Supervisor"),
          P("The dual pipelines form one architectural processor. Software cannot schedule them "
            "independently. Before commit, the comparator checks destination register/value, memory "
            "request, next IP, FLAGS, privilege changes, interrupt acceptance, and architecturally "
            "visible control-state updates."),
          table([
              ["Event", "Required response"],
              ["Comparator match", "Commit once and continue."],
              ["Comparator mismatch", "Freeze commit on both lanes, discard in-flight work, latch diagnostics, assert FAULT_OUT."],
              ["Correctable ECC", "Correct data; increment/log diagnostic counter according to policy."],
              ["Uncorrectable ECC", "Prevent corrupted commit and enter the fault-supervisor path."],
              ["First recoverable mismatch", "Software may clear state and request discard-and-refetch from latched PC."],
              ["Second fault in policy window", "Enter safe mode immediately; no unbounded retry loop."],
          ], [1.75 * inch, 4.85 * inch]),
          heading("8.1 Minimum Latched Diagnostic Record", 2),
          P("FAULT_REASON, FAULT_PC, FAULT_FLAGS, FAULT_TIME, FAULT_STAGE, lane comparison syndrome, "
            "ECC syndrome when applicable, retry-window count, and a valid/frozen indicator. The SoC "
            "binding assigns control-register or MMIO addresses."),
          heading("8.2 Recovery Ordering", 2),
          P("The software supervisor must first place outputs in a safe state, snapshot diagnostics, "
            "clear or reinitialize affected architectural state, then issue the platform recovery "
            "command. FAULT_OUT remains asserted until an explicit acknowledged clear. Recovery uses "
            "discard-and-refetch; resume-in-place is not supported in v1."), PageBreak()]

    s += [heading("9. SB32 Embedded ABI"),
          table([
              ["Item", "Rule"],
              ["Data model", "ILP32: int, long, and pointers are 32 bits; long long is 64 bits in software pairs."],
              ["Arguments", "R0-R5, then R16-R19, then 4-byte stack slots from low to high address."],
              ["Returns", "R0 and R1; larger aggregates use a hidden pointer."],
              ["Stack", "R7, grows downward, 16-byte aligned at public call boundaries, no red zone."],
              ["Frame pointer", "R6 when used."],
              ["Callee-saved", "R6, R12-R15, R24-R27."],
              ["Volatile", "R0-R5, R8-R11, R16-R23, R28-R31, condition flags."],
              ["FP ABI", "Soft-float for the base profile; no V-register argument convention is used."],
              ["Object format", "Little-endian ELF32 SeaBird profile; static linking is required for safety builds."],
          ], [1.45 * inch, 5.15 * inch]),
          heading("9.1 Toolchain Profile", 2),
          P("Recommended target identity: <font name='Courier'>seabird32-unknown-none</font>, CPU "
            "<font name='Courier'>tritium-v1</font>, feature bundle "
            "<font name='Courier'>+tetra,+mpu,+atomics,+fixed-muldiv,-fp,-simd,-paging</font>. "
            "These names describe the required LLVM work; they are not claimed as already shipped."),
          P("Safety builds should disable dynamic linking, exceptions that require an unwinder unless "
            "the unwinder is qualified, and transformations whose timing model is not represented in "
            "the WCET tool. Linker scripts place vectors and critical code/data in TCM."), PageBreak()]

    s += [heading("10. Reset and Boot Sequence"),
          table([
              ["Step", "Firmware responsibility"],
              ["1", "Reset enters Tetra profile bootstrap state at the platform reset vector with interrupts disabled."],
              ["2", "Perform BIST and inspect lockstep/ECC reset diagnostics."],
              ["3", "Initialize SP, TCM contents, .data, .bss, and optional scratchpad test."],
              ["4", "Program default-deny MPU regions, then lock safety-critical descriptors."],
              ["5", "Install the vector table and configure fixed interrupt priorities."],
              ["6", "Initialize fault supervisor policy and FAULT_OUT handling."],
              ["7", "Initialize RTOS or call the application entry point."],
              ["8", "Enable interrupts only after all protected state is valid."],
          ], [0.55 * inch, 6.05 * inch]),
          heading("10.1 Safe-State Requirements", 2),
          P("The board profile must define safe output values, watchdog behavior, external supervisor "
            "interaction, and whether a fault response may reset the core or must remain diagnosable "
            "until an external controller acknowledges it."),
          heading("11. Verification and Conformance", 1),
          table([
              ["Area", "Minimum evidence"],
              ["Decode", "Every required opcode and reserved/unsupported encoding; malformed prefix priority."],
              ["Arithmetic", "Boundary vectors, divide faults, flags, fixed-latency assertions."],
              ["Memory/MPU", "All overlap priorities, cross-region accesses, execute protection, device ordering."],
              ["Interrupts", "Every vector/priority, masked arrival, exact saved frame, measured worst case."],
              ["Lockstep", "Injected register, control, address, data, stage, and timing divergences before commit."],
              ["ECC", "Single-bit correction and all supported double-bit detection paths."],
              ["Software", "Compiler ABI tests, RTOS context switching, atomic litmus tests, linker placement checks."],
              ["Formal", "Fault supervisor state machine, no-divergent-commit property, precise exception property."],
          ], [1.3 * inch, 5.3 * inch]), PageBreak()]

    s += [heading("12. Implementation Checklist"),
          table([
              ["Gate", "Exit criterion"],
              ["Architecture", "Profile feature list frozen; QUERY leaves and control registers assigned."],
              ["RTL", "Scalar pipeline, MPU, TCM, atomics, interrupt path, lockstep comparator complete."],
              ["Timing", "Instruction table and interrupt WCET derived from post-synthesis configuration."],
              ["Safety", "Fault injection demonstrates no divergent commit and correct escalation policy."],
              ["Toolchain", "SeaBird32/Tritium target, assembler, disassembler, linker script, soft-float libraries."],
              ["Firmware", "Boot, MPU setup, vector table, diagnostics, safe mode, watchdog integration."],
              ["RTOS", "MPU-aware task model and bounded context-switch path."],
              ["Release", "Profile conformance corpus passes on RTL and gate-level simulation."],
          ], [1.25 * inch, 5.35 * inch]),
          heading("13. Open Decisions", 1)]
    for item in [
        "Finalize the guaranteed interrupt latency after synthesis; retain 12 cycles as a target until then.",
        "Choose the implementation granule reported by MPUINFO for each Tritium SKU.",
        "Assign concrete fault-supervisor CSRs/MMIO addresses and diagnostic syndrome format.",
        "Publish the board memory map, vector-table base, timer, interrupt controller, watchdog, and FAULT_OUT protocol.",
        "Decide whether the first SKU includes optional scalar FP or remains soft-float only.",
        "Select certification objectives and required diagnostic coverage for ISO 26262 or DO-178C programs.",
    ]: s.append(bullet(item))
    s += [Spacer(1, 10), P("Recommended profile identifier", "H2x"),
          P("SB-TRITIUM32-v1 / SeaBird 3.2 / Tetra / deterministic safety subset", "Mono"),
          Spacer(1, 8), P("Source basis", "H2x"),
          P("Meisei Tritium Embedded Processor Preliminary Datasheet v1 Draft; SeaBird 3.2 "
            "machine-readable specification and architecture manuals in this repository. This "
            "document defines an implementation profile and does not override parent instruction semantics.")]
    return s


def main():
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    doc = ProfileDoc(str(OUTPUT))
    doc.build(story())
    print(OUTPUT)


if __name__ == "__main__":
    main()

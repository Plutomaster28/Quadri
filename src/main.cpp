#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../generated/seabird_opcodes.hpp"

namespace {

enum class XReg : int {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3, RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15,
    NONE = -1
};

struct Mem {
    bool present = false;
    XReg base = XReg::NONE;
    XReg index = XReg::NONE;
    int scale = 1;
    int64_t disp = 0;
    bool rip_relative = false;
};

struct Operand {
    enum Kind { REG, MEM, IMM } kind = IMM;
    XReg reg = XReg::NONE;
    Mem mem;
    int64_t imm = 0;

    static Operand regop(XReg r) {
        Operand o;
        o.kind = REG;
        o.reg = r;
        return o;
    }
    static Operand memop(const Mem &m) {
        Operand o;
        o.kind = MEM;
        o.mem = m;
        return o;
    }
    static Operand immop(int64_t v) {
        Operand o;
        o.kind = IMM;
        o.imm = v;
        return o;
    }
};

struct XInst {
    uint64_t pc = 0;
    size_t size = 0;
    std::string mnemonic;
    std::vector<Operand> ops;
    std::vector<uint8_t> bytes;
};

struct Emitted {
    std::vector<std::string> asm_lines;
    std::vector<uint8_t> bytes;
    std::vector<std::string> notes;
};

class Reader {
public:
    explicit Reader(const std::vector<uint8_t> &bytes) : bytes_(bytes) {}
    bool eof() const { return pos_ >= bytes_.size(); }
    size_t pos() const { return pos_; }
    uint8_t u8() {
        if (eof()) throw std::runtime_error("unexpected end of input");
        return bytes_[pos_++];
    }
    int8_t i8() { return static_cast<int8_t>(u8()); }
    uint32_t u32() {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v |= uint32_t(u8()) << (8 * i);
        return v;
    }
    int32_t i32() { return static_cast<int32_t>(u32()); }
    uint64_t u64() {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= uint64_t(u8()) << (8 * i);
        return v;
    }
    std::vector<uint8_t> slice(size_t start, size_t end) const {
        return std::vector<uint8_t>(bytes_.begin() + static_cast<long long>(start),
                                    bytes_.begin() + static_cast<long long>(end));
    }

private:
    const std::vector<uint8_t> &bytes_;
    size_t pos_ = 0;
};

int seabird_reg(XReg r) {
    switch (r) {
    case XReg::RAX: return 0;
    case XReg::RBX: return 1;
    case XReg::RCX: return 2;
    case XReg::RDX: return 3;
    case XReg::RSI: return 4;
    case XReg::RDI: return 5;
    case XReg::RBP: return 6;
    case XReg::RSP: return 7;
    case XReg::R8: return 8;
    case XReg::R9: return 9;
    case XReg::R10: return 10;
    case XReg::R11: return 11;
    case XReg::R12: return 12;
    case XReg::R13: return 13;
    case XReg::R14: return 14;
    case XReg::R15: return 15;
    default: return -1;
    }
}

std::string sr(XReg r) {
    int n = seabird_reg(r);
    if (n < 0) return "NONE";
    return "R" + std::to_string(n);
}

std::string hex_u(uint64_t value, int width = 0) {
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << std::setfill('0');
    if (width) os << std::setw(width);
    os << value;
    return os.str();
}

std::string mem_text(const Mem &m) {
    std::ostringstream os;
    os << "[";
    bool wrote = false;
    if (m.rip_relative) {
        os << "RIP";
        wrote = true;
    } else if (m.base != XReg::NONE) {
        os << sr(m.base);
        wrote = true;
    }
    if (m.index != XReg::NONE) {
        if (wrote) os << "+";
        os << sr(m.index);
        if (m.scale != 1) os << "*" << m.scale;
        wrote = true;
    }
    if (m.disp != 0 || !wrote) {
        if (wrote && m.disp > 0) os << "+";
        if (m.disp < 0) os << "-" << hex_u(static_cast<uint64_t>(-m.disp));
        else os << hex_u(static_cast<uint64_t>(m.disp));
    }
    os << "]";
    return os.str();
}

std::string op_text(const Operand &o) {
    if (o.kind == Operand::REG) return sr(o.reg);
    if (o.kind == Operand::MEM) return mem_text(o.mem);
    return "#" + hex_u(static_cast<uint64_t>(o.imm));
}

uint8_t modrm_reg(int reg, int rm) {
    return static_cast<uint8_t>(0xC0 | ((reg & 7) << 3) | (rm & 7));
}

void emit_byte(Emitted &e, uint8_t b) { e.bytes.push_back(b); }

void emit_orex_prefix(Emitted &e, int reg_field, int rm_field) {
    emit_byte(e, 0xFE);
    emit_byte(e, 0x80);
    (void)reg_field;
    (void)rm_field;
}

uint8_t orex_byte(int reg_field, int rm_field) {
    return static_cast<uint8_t>(((reg_field >> 3) & 0x3) | (((rm_field >> 3) & 0x3) << 2));
}

void encode_reg2(Emitted &e, uint8_t opcode, XReg dst, XReg src, const std::string &what) {
    int d = seabird_reg(dst), s = seabird_reg(src);
    bool orex = d > 7 || s > 7;
    if (orex) emit_orex_prefix(e, d, s);
    emit_byte(e, opcode);
    emit_byte(e, modrm_reg(d, s));
    if (orex) emit_byte(e, orex_byte(d, s));
    (void)what;
}

void encode_reg_imm64(Emitted &e, uint8_t opcode, XReg dst, int64_t imm, const std::string &what) {
    int d = seabird_reg(dst);
    bool orex = d > 7;
    if (orex) emit_orex_prefix(e, 0, d);
    emit_byte(e, opcode);
    emit_byte(e, modrm_reg(0, d));
    if (orex) emit_byte(e, orex_byte(0, d));
    const auto v = static_cast<uint64_t>(imm);
    for (int i = 0; i < 8; ++i) emit_byte(e, static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    (void)what;
}

void encode_noop(Emitted &e, uint8_t opcode) { emit_byte(e, opcode); }

struct Rex {
    bool w = false;
    bool r = false;
    bool x = false;
    bool b = false;
};

XReg reg_from_bits(int bits, bool high) {
    return static_cast<XReg>((bits & 7) | (high ? 8 : 0));
}

struct DecodedModRM {
    Operand rm;
    XReg reg = XReg::NONE;
};

DecodedModRM decode_modrm(Reader &r, const Rex &rex) {
    uint8_t b = r.u8();
    int mod = (b >> 6) & 3;
    int reg = (b >> 3) & 7;
    int rm = b & 7;
    DecodedModRM out;
    out.reg = reg_from_bits(reg, rex.r);

    if (mod == 3) {
        out.rm = Operand::regop(reg_from_bits(rm, rex.b));
        return out;
    }

    Mem m;
    m.present = true;
    if (rm == 4) {
        uint8_t sib = r.u8();
        int scale_bits = (sib >> 6) & 3;
        int index = (sib >> 3) & 7;
        int base = sib & 7;
        m.scale = 1 << scale_bits;
        if (index != 4) m.index = reg_from_bits(index, rex.x);
        if (mod == 0 && base == 5) {
            m.base = XReg::NONE;
            m.disp = r.i32();
        } else {
            m.base = reg_from_bits(base, rex.b);
        }
    } else if (mod == 0 && rm == 5) {
        m.rip_relative = true;
        m.disp = r.i32();
    } else {
        m.base = reg_from_bits(rm, rex.b);
    }

    if (mod == 1) m.disp += r.i8();
    if (mod == 2) m.disp += r.i32();
    out.rm = Operand::memop(m);
    return out;
}

std::string jcc_name(int cc) {
    static const char *names[16] = {
        "JO", "JNO", "JC", "JNC", "JE", "JNE", "JLE", "JG",
        "JS", "JNS", "JP", "JNP", "JL", "JGE", "JLE", "JG"
    };
    return names[cc & 15];
}

class Decoder {
public:
    explicit Decoder(const std::vector<uint8_t> &bytes) : bytes_(bytes), r_(bytes) {}

    bool eof() const { return r_.eof(); }

    XInst next() {
        size_t start = r_.pos();
        XInst inst;
        inst.pc = start;
        Rex rex;

        while (!r_.eof()) {
            uint8_t b = peek();
            if (b >= 0x40 && b <= 0x4F) {
                b = r_.u8();
                rex.w = b & 0x08;
                rex.r = b & 0x04;
                rex.x = b & 0x02;
                rex.b = b & 0x01;
            } else if (b == 0x66 || b == 0xF0 || b == 0xF2 || b == 0xF3) {
                r_.u8();
            } else {
                break;
            }
        }

        uint8_t op = r_.u8();
        if (op >= 0x50 && op <= 0x57) {
            inst.mnemonic = "PUSH";
            inst.ops.push_back(Operand::regop(reg_from_bits(op - 0x50, rex.b)));
        } else if (op >= 0x58 && op <= 0x5F) {
            inst.mnemonic = "POP";
            inst.ops.push_back(Operand::regop(reg_from_bits(op - 0x58, rex.b)));
        } else if (op >= 0x70 && op <= 0x7F) {
            inst.mnemonic = jcc_name(op - 0x70);
            size_t next = r_.pos() + 1;
            int8_t disp = r_.i8();
            inst.ops.push_back(Operand::immop(static_cast<int64_t>(next) + disp));
        } else if (op >= 0xB8 && op <= 0xBF) {
            inst.mnemonic = "MOV";
            inst.ops.push_back(Operand::regop(reg_from_bits(op - 0xB8, rex.b)));
            inst.ops.push_back(Operand::immop(rex.w ? static_cast<int64_t>(r_.u64()) : r_.i32()));
        } else {
            switch (op) {
            case 0x01: binary_rm_reg(inst, "ADD", rex); break;
            case 0x03: binary_reg_rm(inst, "ADD", rex); break;
            case 0x09: binary_rm_reg(inst, "OR", rex); break;
            case 0x0B: binary_reg_rm(inst, "OR", rex); break;
            case 0x21: binary_rm_reg(inst, "AND", rex); break;
            case 0x23: binary_reg_rm(inst, "AND", rex); break;
            case 0x29: binary_rm_reg(inst, "SUB", rex); break;
            case 0x2B: binary_reg_rm(inst, "SUB", rex); break;
            case 0x31: binary_rm_reg(inst, "XOR", rex); break;
            case 0x33: binary_reg_rm(inst, "XOR", rex); break;
            case 0x39: binary_rm_reg(inst, "CMP", rex); break;
            case 0x3B: binary_reg_rm(inst, "CMP", rex); break;
            case 0x89: binary_rm_reg(inst, "MOV", rex); break;
            case 0x8B: binary_reg_rm(inst, "MOV", rex); break;
            case 0x90: inst.mnemonic = "NOP"; break;
            case 0xC3: inst.mnemonic = "RET"; break;
            case 0xC7: {
                auto m = decode_modrm(r_, rex);
                inst.mnemonic = "MOV";
                inst.ops.push_back(m.rm);
                inst.ops.push_back(Operand::immop(r_.i32()));
                break;
            }
            case 0xE8:
                inst.mnemonic = "CALL";
                branch_rel32(inst);
                break;
            case 0xE9:
                inst.mnemonic = "JMP";
                branch_rel32(inst);
                break;
            case 0xEB:
                inst.mnemonic = "JMP";
                branch_rel8(inst);
                break;
            case 0x0F: {
                uint8_t op2 = r_.u8();
                if (op2 == 0x05) {
                    inst.mnemonic = "SYSCALL";
                } else if (op2 >= 0x80 && op2 <= 0x8F) {
                    inst.mnemonic = jcc_name(op2 - 0x80);
                    branch_rel32(inst);
                } else {
                    throw std::runtime_error("unsupported 0F opcode " + hex_u(op2, 2));
                }
                break;
            }
            default:
                throw std::runtime_error("unsupported opcode " + hex_u(op, 2));
            }
        }

        inst.size = r_.pos() - start;
        inst.bytes = std::vector<uint8_t>(bytes_.begin() + static_cast<long long>(start),
                                          bytes_.begin() + static_cast<long long>(r_.pos()));
        return inst;
    }

private:
    uint8_t peek() const { return bytes_.at(r_.pos()); }

    void binary_rm_reg(XInst &inst, const std::string &mn, const Rex &rex) {
        auto m = decode_modrm(r_, rex);
        inst.mnemonic = mn;
        inst.ops.push_back(m.rm);
        inst.ops.push_back(Operand::regop(m.reg));
    }

    void binary_reg_rm(XInst &inst, const std::string &mn, const Rex &rex) {
        auto m = decode_modrm(r_, rex);
        inst.mnemonic = mn;
        inst.ops.push_back(Operand::regop(m.reg));
        inst.ops.push_back(m.rm);
    }

    void branch_rel8(XInst &inst) {
        size_t next = r_.pos() + 1;
        int8_t disp = r_.i8();
        inst.ops.push_back(Operand::immop(static_cast<int64_t>(next) + disp));
    }

    void branch_rel32(XInst &inst) {
        size_t next = r_.pos() + 4;
        int32_t disp = r_.i32();
        inst.ops.push_back(Operand::immop(static_cast<int64_t>(next) + disp));
    }

    const std::vector<uint8_t> &bytes_;
    Reader r_;
};

uint8_t seabird_opcode(const std::string &m) {
    for (const auto &entry : seabird::isa::kOpcodes) {
        if (entry.length == 1 && entry.mnemonic == m) return entry.bytes[0];
    }
    return 0xFF;
}

void emit_ea(Emitted &e, const Mem &m) {
    if (m.rip_relative) {
        e.asm_lines.push_back("  ; RIP-relative x86 operand lowered as absolute target for AOT relocation review");
        e.asm_lines.push_back("  MOVI R28, #" + hex_u(static_cast<uint64_t>(m.disp)));
        e.notes.push_back("RIP-relative relocation still requires AOT relocation metadata");
        return;
    }
    if (m.base != XReg::NONE && m.index != XReg::NONE) {
        e.asm_lines.push_back("  LEAS R28, " + mem_text(m));
        e.notes.push_back("LEAS SIB byte emission is not implemented in this translator path yet");
    } else if (m.base != XReg::NONE && m.disp != 0) {
        e.asm_lines.push_back("  MOV R28, " + sr(m.base));
        e.notes.push_back("compound effective-address sequence is currently emitted textually");
        e.asm_lines.push_back("  ADDI R28, #" + hex_u(static_cast<uint64_t>(m.disp)));
        e.notes.push_back("compound effective-address sequence is currently emitted textually");
    } else if (m.base != XReg::NONE) {
        e.asm_lines.push_back("  MOV R28, " + sr(m.base));
        e.notes.push_back("effective-address helper is currently emitted textually");
    } else {
        e.asm_lines.push_back("  MOVI R28, #" + hex_u(static_cast<uint64_t>(m.disp)));
        e.notes.push_back("absolute effective-address helper is currently emitted textually");
    }
}

void emit_load_to(Emitted &e, XReg dst, const Mem &m) {
    emit_ea(e, m);
    e.asm_lines.push_back("  LDQ " + sr(dst) + ", [R28]");
    e.notes.push_back("LDQ memory-form byte emission is not implemented in this translator path yet");
}

void emit_load_scratch(Emitted &e, const Mem &m) {
    emit_ea(e, m);
    e.asm_lines.push_back("  LDQ R29, [R28]");
    e.notes.push_back("LDQ memory-form byte emission is not implemented in this translator path yet");
}

void emit_store_from(Emitted &e, const Mem &m, XReg src) {
    emit_ea(e, m);
    e.asm_lines.push_back("  STQ [R28], " + sr(src));
    e.notes.push_back("STQ memory-form byte emission is not implemented in this translator path yet");
}

void emit_store_scratch(Emitted &e, const Mem &m) {
    emit_ea(e, m);
    e.asm_lines.push_back("  STQ [R28], R29");
    e.notes.push_back("STQ memory-form byte emission is not implemented in this translator path yet");
}

void emit_branch_imm(Emitted &e, const std::string &m, int64_t target) {
    e.asm_lines.push_back("  " + m + " " + hex_u(static_cast<uint64_t>(target)));
    emit_byte(e, seabird_opcode(m));
    uint32_t v = static_cast<uint32_t>(target);
    for (int i = 0; i < 4; ++i) emit_byte(e, static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

Emitted lower(const std::vector<XInst> &insts) {
    Emitted e;
    for (const auto &x : insts) {
        std::ostringstream head;
        head << "L" << std::hex << x.pc << ": ; x86 " << x.mnemonic;
        for (size_t i = 0; i < x.ops.size(); ++i) head << (i == 0 ? " " : ", ") << op_text(x.ops[i]);
        e.asm_lines.push_back(head.str());

        const auto &m = x.mnemonic;
        if (m == "NOP") {
            e.asm_lines.push_back("  ; NOP elided (SeaBird base ISA has no architecturally inert NOP)");
        } else if (m == "RET" || m == "SYSCALL") {
            e.asm_lines.push_back("  " + m);
            encode_noop(e, seabird_opcode(m));
        } else if (m == "PUSH" || m == "POP") {
            e.asm_lines.push_back("  " + m + " " + op_text(x.ops[0]));
            if (x.ops[0].kind == Operand::REG) {
                int r = seabird_reg(x.ops[0].reg);
                bool orex = r > 7;
                if (orex) emit_orex_prefix(e, 0, r);
                emit_byte(e, seabird_opcode(m));
                emit_byte(e, modrm_reg(0, r));
                if (orex) emit_byte(e, orex_byte(0, r));
            }
        } else if (m == "JMP" || m == "CALL" || m[0] == 'J') {
            emit_branch_imm(e, m, x.ops[0].imm);
        } else if (m == "MOV" && x.ops[0].kind == Operand::REG && x.ops[1].kind == Operand::IMM) {
            e.asm_lines.push_back("  MOVI " + op_text(x.ops[0]) + ", " + op_text(x.ops[1]));
            encode_reg_imm64(e, 0x01, x.ops[0].reg, x.ops[1].imm, "MOVI");
        } else if (m == "MOV" && x.ops[0].kind == Operand::REG && x.ops[1].kind == Operand::REG) {
            e.asm_lines.push_back("  MOV " + op_text(x.ops[0]) + ", " + op_text(x.ops[1]));
            encode_reg2(e, 0x00, x.ops[0].reg, x.ops[1].reg, "MOV");
        } else if (m == "MOV" && x.ops[0].kind == Operand::REG && x.ops[1].kind == Operand::MEM) {
            emit_load_to(e, x.ops[0].reg, x.ops[1].mem);
        } else if (m == "MOV" && x.ops[0].kind == Operand::MEM && x.ops[1].kind == Operand::REG) {
            emit_store_from(e, x.ops[0].mem, x.ops[1].reg);
        } else if (m == "MOV" && x.ops[0].kind == Operand::MEM && x.ops[1].kind == Operand::IMM) {
            e.asm_lines.push_back("  MOVI R29, " + op_text(x.ops[1]));
            e.notes.push_back("memory-immediate lowering sequence is currently emitted textually");
            emit_store_scratch(e, x.ops[0].mem);
        } else if ((m == "ADD" || m == "SUB" || m == "AND" || m == "OR" || m == "XOR" || m == "CMP")
                   && x.ops[0].kind == Operand::REG && x.ops[1].kind == Operand::REG) {
            e.asm_lines.push_back("  " + m + " " + op_text(x.ops[0]) + ", " + op_text(x.ops[1]));
            encode_reg2(e, seabird_opcode(m), x.ops[0].reg, x.ops[1].reg, m);
        } else if ((m == "ADD" || m == "SUB" || m == "AND" || m == "OR" || m == "XOR" || m == "CMP")
                   && x.ops[0].kind == Operand::REG && x.ops[1].kind == Operand::MEM) {
            emit_load_scratch(e, x.ops[1].mem);
            e.asm_lines.push_back("  " + m + " " + op_text(x.ops[0]) + ", R29");
            e.notes.push_back(m + " memory lowering sequence is currently emitted textually");
        } else if ((m == "ADD" || m == "SUB" || m == "AND" || m == "OR" || m == "XOR")
                   && x.ops[0].kind == Operand::MEM && x.ops[1].kind == Operand::REG) {
            emit_load_scratch(e, x.ops[0].mem);
            e.asm_lines.push_back("  " + m + " R29, " + op_text(x.ops[1]));
            e.notes.push_back(m + " memory lowering sequence is currently emitted textually");
            emit_store_scratch(e, x.ops[0].mem);
        } else if (m == "CMP" && x.ops[0].kind == Operand::MEM && x.ops[1].kind == Operand::REG) {
            emit_load_scratch(e, x.ops[0].mem);
            e.asm_lines.push_back("  CMP R29, " + op_text(x.ops[1]));
            e.notes.push_back("CMP memory lowering sequence is currently emitted textually");
        } else {
            e.asm_lines.push_back("  ; unsupported lowering pattern");
            e.notes.push_back("unsupported lowering at x86 offset " + hex_u(x.pc) + ": " + x.mnemonic);
        }
    }
    return e;
}

std::vector<uint8_t> parse_hex(const std::string &s) {
    std::vector<uint8_t> out;
    std::string clean;
    for (char c : s) {
        if (std::isxdigit(static_cast<unsigned char>(c))) clean.push_back(c);
    }
    if (clean.size() % 2) throw std::runtime_error("hex input has odd number of digits");
    for (size_t i = 0; i < clean.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoul(clean.substr(i, 2), nullptr, 16)));
    }
    return out;
}

std::vector<uint8_t> read_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input file: " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

void write_bytes(const std::string &path, const std::vector<uint8_t> &bytes) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open output file: " + path);
    f.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void write_lines(const std::string &path, const std::vector<std::string> &lines) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("cannot open output file: " + path);
    for (const auto &line : lines) f << line << "\n";
}

std::vector<XInst> decode_all(const std::vector<uint8_t> &bytes) {
    Decoder d(bytes);
    std::vector<XInst> insts;
    while (!d.eof()) insts.push_back(d.next());
    return insts;
}

void print_usage() {
    std::cerr << "usage:\n"
              << "  pebble-xlate jit \"48 b8 01 00 00 00 00 00 00 00 c3\"\n"
              << "  pebble-xlate aot input.bin out_prefix\n";
}

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc < 3) {
            print_usage();
            return 2;
        }
        std::string mode = argv[1];
        std::vector<uint8_t> input;
        std::string out_prefix;
        if (mode == "jit") {
            input = parse_hex(argv[2]);
        } else if (mode == "aot") {
            if (argc < 4) {
                print_usage();
                return 2;
            }
            input = read_file(argv[2]);
            out_prefix = argv[3];
        } else {
            print_usage();
            return 2;
        }

        auto insts = decode_all(input);
        auto emitted = lower(insts);

        if (mode == "jit") {
            for (const auto &line : emitted.asm_lines) std::cout << line << "\n";
            std::cout << "\nSeaBird bytes:";
            for (uint8_t b : emitted.bytes) {
                std::cout << " " << std::hex << std::uppercase << std::setw(2)
                          << std::setfill('0') << static_cast<int>(b);
            }
            std::cout << "\n";
            if (!emitted.notes.empty()) {
                std::cout << "\nNotes:\n";
                for (const auto &n : emitted.notes) std::cout << "- " << n << "\n";
            }
        } else {
            write_lines(out_prefix + ".sba", emitted.asm_lines);
            write_bytes(out_prefix + ".sbc", emitted.bytes);
            std::vector<std::string> report;
            report.push_back("Decoded x86 instructions: " + std::to_string(insts.size()));
            report.push_back("Emitted SeaBird bytes: " + std::to_string(emitted.bytes.size()));
            report.push_back("");
            report.push_back("Notes:");
            if (emitted.notes.empty()) report.push_back("- none");
            for (const auto &n : emitted.notes) report.push_back("- " + n);
            write_lines(out_prefix + ".report.txt", report);
            std::cout << "wrote " << out_prefix << ".sba, " << out_prefix
                      << ".sbc, and " << out_prefix << ".report.txt\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

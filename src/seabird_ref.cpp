#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../generated/seabird_opcodes.hpp"

namespace {

struct Cpu {
    std::array<std::uint64_t, 32> r{};
    std::array<std::array<std::uint64_t, 2>, 32> v{};
    std::array<std::uint8_t, 65536> memory{};
    bool cf = false, zf = false, sf = false, of = false;

    Cpu() { r[7] = memory.size(); }
};

struct Decoded {
    std::uint8_t opcode = 0;
    std::uint8_t reg = 0;
    std::uint8_t rm = 0;
    std::uint8_t index = 4;
    std::uint8_t scale = 1;
    std::int64_t displacement = 0;
    bool memory = false;
    bool vector = false;
    bool vector_memory = false;
    bool int_to_fp = false;
    bool fp_to_int = false;
    bool scalar_fp_load = false;
    bool scalar_fp_store = false;
    unsigned scalar_fp_bytes = 8;
    std::uint8_t third = 0;
    std::uint64_t immediate = 0;
    std::uint16_t system_register = 0;
    std::size_t size = 0;
};

std::uint64_t read_le(const std::vector<std::uint8_t>& code, std::size_t& p, unsigned bytes) {
    if (bytes > 8 || p + bytes > code.size()) throw std::runtime_error("truncated immediate");
    std::uint64_t value = 0;
    for (unsigned i = 0; i < bytes; ++i) value |= std::uint64_t(code[p++]) << (i * 8);
    return value;
}

Decoded decode_at(const std::vector<std::uint8_t>& code, std::size_t start,
                  unsigned operand_bytes = 8) {
    std::size_t p = start;
    bool has_orex = false;
    bool has_vector = false;
    unsigned operand_width_code = 0;
    if (p < code.size() && code[p] == 0xFE) {
        if (p + 2 > code.size()) throw std::runtime_error("truncated primary prefix");
        const auto control = code[p + 1];
        has_orex = (control & 0x80) != 0;
        has_vector = (control & 0x40) != 0;
        operand_width_code = (control >> 3) & 7;
        p += 2;
    }
    if (p >= code.size()) throw std::runtime_error("truncated instruction");
    Decoded d;
    d.opcode = code[p++];
    d.vector_memory = has_vector && (d.opcode == 0xA1 || d.opcode == 0xA2);
    d.int_to_fp = has_vector && d.opcode == 0xAC;
    d.fp_to_int = has_vector && d.opcode == 0xAD;
    if (d.opcode == 0xFF) {
        if (p + 2 > code.size() || code[p++] != 0x05)
            throw std::runtime_error("unsupported extension opcode");
        const auto subopcode = code[p++];
        d.scalar_fp_load = subopcode == 0x10;
        d.scalar_fp_store = subopcode == 0x11;
        if (!d.scalar_fp_load && !d.scalar_fp_store)
            throw std::runtime_error("unsupported FPX opcode");
    }
    if (d.scalar_fp_load || d.scalar_fp_store) {
        if (operand_width_code < 1 || operand_width_code > 5)
            throw std::runtime_error("unsupported scalar FP memory width");
        d.scalar_fp_bytes = 1U << (operand_width_code - 1);
    }
    if (d.opcode == 0x60 || d.opcode == 0x82) {
        d.size = p - start;
        return d;
    }
    if (d.opcode == 0x5C || d.opcode == 0x5E ||
        (d.opcode >= 0x61 && d.opcode <= 0x6C)) {
        d.immediate = read_le(code, p, 4);
        d.size = p - start;
        return d;
    }
    if (p >= code.size()) throw std::runtime_error("truncated ModR/M");
    const auto modrm = code[p++];
    const auto mod = modrm >> 6;
    const bool integer_memory = d.opcode == 0x13 || d.opcode == 0x14 || d.opcode == 0x15 ||
                                d.opcode == 0x16 || d.opcode == 0x17 ||
                                d.opcode == 0x18 || d.opcode == 0x19 || d.opcode == 0x1A ||
                                d.opcode == 0x1D;
    if (mod != 3 && !integer_memory && !d.vector_memory &&
        !d.scalar_fp_load && !d.scalar_fp_store)
        throw std::runtime_error("unsupported ModR/M form");
    d.memory = mod != 3;
    d.reg = (modrm >> 3) & 7;
    d.rm = modrm & 7;
    const bool has_sib = d.memory && d.rm == 4;
    if (has_sib) {
        if (p >= code.size()) throw std::runtime_error("missing SIB byte");
        const auto sib = code[p++];
        d.scale = 1 << (sib >> 6);
        d.index = (sib >> 3) & 7;
        d.rm = sib & 7;
    }
    if (has_orex) {
        if (p >= code.size()) throw std::runtime_error("missing OREX byte");
        const auto orex = code[p++];
        d.reg |= (orex & 0x3) << 3;
        if (has_sib) {
            d.index |= ((orex >> 4) & 0x3) << 3;
            d.rm |= ((orex >> 6) & 0x3) << 3;
        } else {
            d.rm |= ((orex >> 2) & 0x3) << 3;
        }
    }
    if (d.opcode == 0x80 || d.opcode == 0x81) {
        if (mod != 3 || d.reg != 0)
            throw std::runtime_error("non-canonical system-register ModR/M");
        d.system_register = static_cast<std::uint16_t>(read_le(code, p, 2));
    }
    if (d.opcode == 0x6D || d.opcode == 0x6E)
        d.immediate = read_le(code, p, 4);
    if (has_vector) {
        if (p >= code.size())
            throw std::runtime_error("unsupported VectorCtl");
        const auto vector_control = code[p++];
        const auto lane_width = (vector_control >> 3) & 7;
        if ((vector_control & 0xC7) != 0 || lane_width > 4)
            throw std::runtime_error("unsupported VectorCtl");
        d.scalar_fp_bytes = 1U << lane_width;
        if (d.int_to_fp || d.fp_to_int) {
            // VectorCtl selects the scalar IEEE format; there is no XOP.
        } else if (d.opcode == 0xA1 || d.opcode == 0xA2) {
            d.vector_memory = true;
        } else if (d.opcode == 0x00 || d.opcode == 0xAA ||
                   d.opcode == 0xAB || d.opcode == 0xAE ||
                   d.opcode == 0xAF) {
            d.vector = true;
        } else {
            if (p >= code.size() || (code[p] >> 5) != 1)
                throw std::runtime_error("missing vector XOP");
            d.third = code[p++] & 31;
            d.vector = true;
        }
    }
    if (d.opcode == 0x01) d.immediate = read_le(code, p, operand_bytes);
    if (d.memory && mod == 1)
        d.displacement = static_cast<std::int8_t>(read_le(code, p, 1));
    else if (d.memory && mod == 2)
        d.displacement = static_cast<std::int32_t>(read_le(code, p, 4));
    d.size = p - start;
    return d;
}

Decoded decode(const std::vector<std::uint8_t>& code, unsigned operand_bytes = 8) {
    Decoded d = decode_at(code, 0, operand_bytes);
    if (d.size != code.size()) throw std::runtime_error("unexpected trailing bytes");
    return d;
}

void flags_add(Cpu& cpu, std::uint64_t a, std::uint64_t b, std::uint64_t result) {
    cpu.cf = result < a;
    cpu.zf = result == 0;
    cpu.sf = (result >> 63) != 0;
    cpu.of = ((~(a ^ b) & (a ^ result)) >> 63) != 0;
}

void flags_sub(Cpu& cpu, std::uint64_t a, std::uint64_t b, std::uint64_t result) {
    cpu.cf = a < b;
    cpu.zf = result == 0;
    cpu.sf = (result >> 63) != 0;
    cpu.of = (((a ^ b) & (a ^ result)) >> 63) != 0;
}

void execute(Cpu& cpu, const Decoded& d) {
    if (d.int_to_fp) {
        if (d.scalar_fp_bytes == 4) {
            const float value = static_cast<float>(static_cast<std::int64_t>(cpu.r[d.rm]));
            cpu.v[d.reg] = {};
            std::memcpy(&cpu.v[d.reg][0], &value, sizeof(value));
            return;
        }
        if (d.scalar_fp_bytes == 16) {
            const __float128 value = static_cast<__float128>(
                static_cast<std::int64_t>(cpu.r[d.rm]));
            std::memcpy(cpu.v[d.reg].data(), &value, sizeof(value));
            return;
        }
        const double value = static_cast<double>(static_cast<std::int64_t>(cpu.r[d.rm]));
        std::memcpy(&cpu.v[d.reg][0], &value, sizeof(value));
        cpu.v[d.reg][1] = 0;
        return;
    }
    if (d.fp_to_int) {
        if (d.scalar_fp_bytes == 4) {
            float value = 0;
            std::memcpy(&value, &cpu.v[d.rm][0], sizeof(value));
            cpu.r[d.reg] = static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
            return;
        }
        if (d.scalar_fp_bytes == 16) {
            __float128 value = 0;
            std::memcpy(&value, cpu.v[d.rm].data(), sizeof(value));
            cpu.r[d.reg] = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(value));
            return;
        }
        double value = 0;
        std::memcpy(&value, &cpu.v[d.rm][0], sizeof(value));
        cpu.r[d.reg] = static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
        return;
    }
    if (d.vector) {
        if (d.opcode == 0xAB) {
            bool unordered = false;
            bool less = false;
            bool equal = false;
            if (d.scalar_fp_bytes == 4) {
                float lhs = 0, rhs = 0;
                std::memcpy(&lhs, &cpu.v[d.reg][0], sizeof(lhs));
                std::memcpy(&rhs, &cpu.v[d.rm][0], sizeof(rhs));
                unordered = std::isnan(lhs) || std::isnan(rhs);
                less = !unordered && lhs < rhs;
                equal = !unordered && lhs == rhs;
            } else if (d.scalar_fp_bytes == 8) {
                double lhs = 0, rhs = 0;
                std::memcpy(&lhs, &cpu.v[d.reg][0], sizeof(lhs));
                std::memcpy(&rhs, &cpu.v[d.rm][0], sizeof(rhs));
                unordered = std::isnan(lhs) || std::isnan(rhs);
                less = !unordered && lhs < rhs;
                equal = !unordered && lhs == rhs;
            } else if (d.scalar_fp_bytes == 16) {
                __float128 lhs = 0, rhs = 0;
                std::memcpy(&lhs, cpu.v[d.reg].data(), sizeof(lhs));
                std::memcpy(&rhs, cpu.v[d.rm].data(), sizeof(rhs));
                unordered = __builtin_isnan(lhs) || __builtin_isnan(rhs);
                less = !unordered && lhs < rhs;
                equal = !unordered && lhs == rhs;
            } else {
                throw std::runtime_error("unsupported scalar FP compare width");
            }
            cpu.cf = unordered || less;
            cpu.zf = unordered || equal;
            cpu.sf = false;
            cpu.of = false;
            return;
        }
        if (d.opcode >= 0xA6 && d.opcode <= 0xA9) {
            if (d.scalar_fp_bytes == 4) {
                float lhs, rhs, result = 0;
                std::memcpy(&lhs, &cpu.v[d.rm][0], sizeof(lhs));
                std::memcpy(&rhs, &cpu.v[d.third][0], sizeof(rhs));
                if (d.opcode == 0xA6) result = lhs + rhs;
                if (d.opcode == 0xA7) result = lhs - rhs;
                if (d.opcode == 0xA8) result = lhs * rhs;
                if (d.opcode == 0xA9) result = lhs / rhs;
                cpu.v[d.reg] = {};
                std::memcpy(&cpu.v[d.reg][0], &result, sizeof(result));
                return;
            }
            if (d.scalar_fp_bytes == 16) {
                __float128 lhs, rhs, result = 0;
                std::memcpy(&lhs, cpu.v[d.rm].data(), sizeof(lhs));
                std::memcpy(&rhs, cpu.v[d.third].data(), sizeof(rhs));
                if (d.opcode == 0xA6) result = lhs + rhs;
                if (d.opcode == 0xA7) result = lhs - rhs;
                if (d.opcode == 0xA8) result = lhs * rhs;
                if (d.opcode == 0xA9) result = lhs / rhs;
                std::memcpy(cpu.v[d.reg].data(), &result, sizeof(result));
                return;
            }
            if (d.scalar_fp_bytes != 8)
                throw std::runtime_error("unsupported scalar FP arithmetic width");
            double lhs, rhs, result = 0;
            std::memcpy(&lhs, &cpu.v[d.rm][0], sizeof(lhs));
            std::memcpy(&rhs, &cpu.v[d.third][0], sizeof(rhs));
            if (d.opcode == 0xA6) result = lhs + rhs;
            if (d.opcode == 0xA7) result = lhs - rhs;
            if (d.opcode == 0xA8) result = lhs * rhs;
            if (d.opcode == 0xA9) result = lhs / rhs;
            std::memcpy(&cpu.v[d.reg][0], &result, sizeof(result));
            cpu.v[d.reg][1] = 0;
            return;
        }
        if (d.opcode == 0x00) {
            cpu.v[d.reg] = cpu.v[d.rm];
            return;
        }
        for (unsigned lane = 0; lane < 2; ++lane) {
            const auto lhs = cpu.v[d.rm][lane];
            const auto rhs = cpu.v[d.third][lane];
            if (d.opcode == 0x97) cpu.v[d.reg][lane] = lhs + rhs;
            else if (d.opcode == 0x98) cpu.v[d.reg][lane] = lhs - rhs;
            else if (d.opcode == 0x99) cpu.v[d.reg][lane] = lhs * rhs;
            else if (d.opcode == 0x9B) cpu.v[d.reg][lane] = lhs & rhs;
            else if (d.opcode == 0x9C) cpu.v[d.reg][lane] = lhs | rhs;
            else if (d.opcode == 0x9D) cpu.v[d.reg][lane] = lhs ^ rhs;
            else throw std::runtime_error("unsupported vector execution opcode");
        }
        return;
    }
    if (d.memory) {
        const auto address = cpu.r[d.rm] +
            (d.index == 4 ? 0 : cpu.r[d.index] * d.scale) + d.displacement;
        const unsigned access_bytes =
            (d.scalar_fp_load || d.scalar_fp_store) ? d.scalar_fp_bytes : 8;
        if (address > cpu.memory.size() - access_bytes)
            throw std::runtime_error("memory access outside reference image");
        if (d.opcode == 0x1D) {
            cpu.r[d.reg] = address;
            return;
        }
        if (d.vector_memory) {
            if (address > cpu.memory.size() - 16)
                throw std::runtime_error("vector memory access outside reference image");
            for (unsigned lane = 0; lane < 2; ++lane) {
                if (d.opcode == 0xA1) {
                    std::uint64_t value = 0;
                    for (unsigned i = 0; i < 8; ++i)
                        value |= std::uint64_t(cpu.memory[address + lane * 8 + i]) << (i * 8);
                    cpu.v[d.reg][lane] = value;
                } else {
                    for (unsigned i = 0; i < 8; ++i)
                        cpu.memory[address + lane * 8 + i] =
                            static_cast<std::uint8_t>(cpu.v[d.reg][lane] >> (i * 8));
                }
            }
            return;
        }
        if (d.opcode == 0x13 || d.opcode == 0x14 || d.opcode == 0x15 || d.opcode == 0x16 ||
            d.scalar_fp_load) {
            const unsigned bytes = d.scalar_fp_load ? d.scalar_fp_bytes :
                                   d.opcode == 0x13 ? 1 : d.opcode == 0x14 ? 2 :
                                   d.opcode == 0x15 ? 4 : 8;
            std::uint64_t value = 0;
            for (unsigned i = 0; i < std::min(bytes, 8U); ++i)
                value |= std::uint64_t(cpu.memory[static_cast<std::size_t>(address) + i]) << (i * 8);
            if (d.scalar_fp_load) {
                cpu.v[d.reg] = {};
                for (unsigned i = 0; i < bytes; ++i)
                    reinterpret_cast<std::uint8_t *>(cpu.v[d.reg].data())[i] =
                        cpu.memory[static_cast<std::size_t>(address) + i];
            } else {
                cpu.r[d.reg] = value;
            }
            return;
        }
        if (d.opcode == 0x17 || d.opcode == 0x18 || d.opcode == 0x19 || d.opcode == 0x1A ||
            d.scalar_fp_store) {
            const unsigned bytes = d.scalar_fp_store ? d.scalar_fp_bytes :
                                   d.opcode == 0x17 ? 1 : d.opcode == 0x18 ? 2 :
                                   d.opcode == 0x19 ? 4 : 8;
            for (unsigned i = 0; i < bytes; ++i) {
                const auto value = d.scalar_fp_store
                    ? reinterpret_cast<const std::uint8_t *>(cpu.v[d.reg].data())[i]
                    : static_cast<std::uint8_t>(cpu.r[d.reg] >> (i * 8));
                cpu.memory[static_cast<std::size_t>(address) + i] = value;
            }
            return;
        }
    }
    auto& dst = cpu.r[d.reg];
    const auto src = cpu.r[d.rm];
    switch (d.opcode) {
    case 0x00: dst = src; break;
    case 0x01: cpu.r[d.rm] = d.immediate; break;
    case 0x20: { auto a = dst; dst += src; flags_add(cpu, a, src, dst); break; }
    case 0x22: { auto a = dst; dst -= src; flags_sub(cpu, a, src, dst); break; }
    case 0x40: dst &= src; cpu.cf = cpu.of = false; cpu.zf = dst == 0; cpu.sf = dst >> 63; break;
    case 0x41: dst |= src; cpu.cf = cpu.of = false; cpu.zf = dst == 0; cpu.sf = dst >> 63; break;
    case 0x42: dst ^= src; cpu.cf = cpu.of = false; cpu.zf = dst == 0; cpu.sf = dst >> 63; break;
    case 0x47: dst <<= (src & 63); break;
    case 0x48: dst >>= (src & 63); break;
    case 0x49: dst = static_cast<std::uint64_t>(static_cast<std::int64_t>(dst) >> (src & 63)); break;
    case 0x52: { auto result = dst - src; flags_sub(cpu, dst, src, result); break; }
    case 0x5A:
        dst = static_cast<std::int64_t>(dst) <
                      static_cast<std::int64_t>(src)
                  ? 1
                  : 0;
        break;
    default: throw std::runtime_error("unsupported reference execution opcode");
    }
}

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void run_function(Cpu& cpu, const std::vector<std::uint8_t>& code,
                  std::size_t entry) {
    std::size_t pc = entry;
    std::vector<std::size_t> returns;
    for (unsigned steps = 0; steps < 1024; ++steps) {
        if (pc >= code.size()) throw std::runtime_error("PC outside code image");
        Decoded d = decode_at(code, pc);
        pc += d.size;
        if (d.opcode == 0x60) {
            if (returns.empty()) return;
            if (cpu.r[7] > cpu.memory.size() - 8)
                throw std::runtime_error("return stack outside reference memory");
            std::size_t address = static_cast<std::size_t>(cpu.r[7]);
            std::uint64_t target = 0;
            for (unsigned i = 0; i < 8; ++i)
                target |= std::uint64_t(cpu.memory[address + i]) << (i * 8);
            pc = static_cast<std::size_t>(target);
            cpu.r[7] += 8;
            returns.pop_back();
            continue;
        }
        if (d.opcode == 0x82) {
            if (cpu.r[0] == 2)
                cpu.r[0] = 42;
            else
                throw std::runtime_error("unsupported hosted syscall");
            continue;
        }
        if (d.opcode == 0x5E) {
            if (cpu.r[7] < 8) throw std::runtime_error("return stack overflow");
            cpu.r[7] -= 8;
            for (unsigned i = 0; i < 8; ++i)
                cpu.memory[static_cast<std::size_t>(cpu.r[7]) + i] =
                    static_cast<std::uint8_t>(pc >> (i * 8));
            returns.push_back(pc);
            pc = static_cast<std::size_t>(
                static_cast<std::int64_t>(pc) +
                static_cast<std::int32_t>(d.immediate));
            continue;
        }
        if (d.opcode == 0x5F) {
            if (cpu.r[7] < 8) throw std::runtime_error("return stack overflow");
            cpu.r[7] -= 8;
            for (unsigned i = 0; i < 8; ++i)
                cpu.memory[static_cast<std::size_t>(cpu.r[7]) + i] =
                    static_cast<std::uint8_t>(pc >> (i * 8));
            returns.push_back(pc);
            pc = static_cast<std::size_t>(cpu.r[d.rm]);
            continue;
        }
        if (d.opcode == 0x5C) {
            pc = static_cast<std::size_t>(
                static_cast<std::int64_t>(pc) +
                static_cast<std::int32_t>(d.immediate));
            continue;
        }
        if (d.opcode >= 0x61 && d.opcode <= 0x68) {
            bool take = false;
            switch (d.opcode) {
            case 0x61: take = cpu.zf; break;
            case 0x62: take = !cpu.zf; break;
            case 0x63: take = !cpu.zf && cpu.sf == cpu.of; break;
            case 0x64: take = cpu.sf == cpu.of; break;
            case 0x65: take = cpu.sf != cpu.of; break;
            case 0x66: take = cpu.zf || cpu.sf != cpu.of; break;
            case 0x67: take = cpu.cf; break;
            case 0x68: take = !cpu.cf; break;
            }
            if (take)
                pc = static_cast<std::size_t>(
                    static_cast<std::int64_t>(pc) +
                    static_cast<std::int32_t>(d.immediate));
            continue;
        }
        if (d.opcode == 0x6D || d.opcode == 0x6E) {
            const bool zero = cpu.r[d.rm] == 0;
            if ((d.opcode == 0x6D && zero) ||
                (d.opcode == 0x6E && !zero))
                pc = static_cast<std::size_t>(
                    static_cast<std::int64_t>(pc) +
                    static_cast<std::int32_t>(d.immediate));
            continue;
        }
        execute(cpu, d);
    }
    throw std::runtime_error("function did not return");
}

std::vector<std::uint8_t> read_binary(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open binary: " + path);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

void c_smoke_test(const std::string& path) {
    const auto code = read_binary(path);
    require(code.size() == 22, "unexpected C smoke binary size");

    Cpu mix;
    mix.r[0] = 5;
    mix.r[1] = 7;
    mix.r[2] = 3;
    mix.r[3] = 2;
    run_function(mix, code, 0);
    require(mix.r[0] == 13, "seabird_mix execution");

    Cpu constant;
    constant.r[0] = 0xFF;
    run_function(constant, code, 9);
    require(constant.r[0] == (0x1122334455667788ULL ^ 0xFFULL),
            "seabird_constant execution");
    std::cout << "SeaBird C-generated binary execution passed\n";
}

void c_call_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 10;
    cpu.r[1] = 9;
    run_function(cpu, code, 0);
    require(cpu.r[0] == 16, "seabird_caller execution");
    std::cout << "SeaBird C-generated direct-call execution passed\n";
}

void c_branch_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu equal;
    equal.r[0] = 5;
    equal.r[1] = 5;
    run_function(equal, code, 0);
    require(equal.r[0] == 6, "seabird_choose equal branch");

    Cpu different;
    different.r[0] = 5;
    different.r[1] = 7;
    run_function(different, code, 0);
    require(different.r[0] == 9, "seabird_choose unequal branch");
    std::cout << "SeaBird C-generated conditional-branch execution passed\n";
}

void c_memory_test(const std::string& path) {
    const auto code = read_binary(path);
    constexpr std::uint64_t address = 64;
    constexpr std::uint64_t value = 0x1122334455667788ULL;

    Cpu load;
    load.r[0] = address;
    for (unsigned i = 0; i < 8; ++i)
        load.memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8));
    run_function(load, code, 0);
    require(load.r[0] == value, "seabird_load execution");

    Cpu store;
    store.r[0] = address;
    store.r[1] = value;
    run_function(store, code, 3);
    for (unsigned i = 0; i < 8; ++i)
        require(store.memory[address + i] == static_cast<std::uint8_t>(value >> (i * 8)),
                "seabird_store execution");

    Cpu roundtrip;
    roundtrip.r[0] = address;
    roundtrip.r[1] = value;
    run_function(roundtrip, code, 6);
    require(roundtrip.r[0] == value, "seabird_roundtrip execution");
    std::cout << "SeaBird C-generated memory execution passed\n";
}

void c_stack_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 10;
    cpu.r[7] = 1024;
    run_function(cpu, code, 0);
    require(cpu.r[0] == 8, "seabird_stack_local execution");
    require(cpu.r[7] == 1024, "seabird_stack_local stack restoration");
    std::cout << "SeaBird C-generated stack-frame execution passed\n";
}

void c_linked_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 10;
    cpu.r[7] = 1024;
    run_function(cpu, code, 0);
    require(cpu.r[0] == 12, "cross-object linked call execution");
    std::cout << "SeaBird linked multi-object execution passed\n";
}

void c_indirect_test(const std::string& path) {
    auto code = read_binary(path);
    const std::size_t target = code.size();
    code.insert(code.end(), {0x20, 0xC1, 0x60}); // add r0, r1; ret
    Cpu cpu;
    cpu.r[0] = target;
    cpu.r[1] = 10;
    cpu.r[2] = 5;
    run_function(cpu, code, 0);
    require(cpu.r[0] == 12, "register-indirect call execution");
    std::cout << "SeaBird C-generated indirect-call execution passed\n";
}

void c_stack_args_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 5;
    cpu.r[7] = 2048;
    run_function(cpu, code, 0x3A);
    require(cpu.r[0] == 60, "eleven-argument call execution");
    require(cpu.r[7] == 2048, "stack-argument call stack restoration");
    std::cout << "SeaBird C-generated stack-argument execution passed\n";
}

void c_ordered_test(const std::string& path) {
    const auto code = read_binary(path);
    auto check = [&](std::size_t entry, std::uint64_t a, std::uint64_t b,
                     std::uint64_t expected, const char *message) {
        Cpu cpu;
        cpu.r[0] = a;
        cpu.r[1] = b;
        run_function(cpu, code, entry);
        require(cpu.r[0] == expected, message);
    };
    check(0x00, UINT64_MAX, 1, 0, "signed less-than taken");
    check(0x00, 2, 1, 3, "signed less-than not taken");
    check(0x38, 2, 1, 3, "signed greater-equal taken");
    check(0x56, 5, 3, 6, "unsigned greater-than taken");
    check(0x56, 3, 5, 7, "unsigned greater-than not taken");
    check(0x74, 3, 5, 4, "unsigned less-equal taken");
    check(0x74, 5, 3, 5, "unsigned less-equal not taken");
    std::cout << "SeaBird C-generated ordered-comparison execution passed\n";
}

void c_fp_vector_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu fp;
    double a = 1.5, b = 2.5, c = 3.0, result = 0;
    std::memcpy(&fp.v[0][0], &a, sizeof(a));
    std::memcpy(&fp.v[1][0], &b, sizeof(b));
    std::memcpy(&fp.v[2][0], &c, sizeof(c));
    run_function(fp, code, 0);
    std::memcpy(&result, &fp.v[0][0], sizeof(result));
    require(result == 12.0, "binary64 FP execution");

    Cpu vector;
    vector.v[0] = {2, 4};
    vector.v[1] = {3, 5};
    run_function(vector, code, 0x0D);
    require(vector.v[0][0] == ((2 + 3) ^ 2) &&
            vector.v[0][1] == ((4 + 5) ^ 4), "v2i64 vector execution");
    std::cout << "SeaBird C-generated FP/vector execution passed\n";
}

void c_fp_memory_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 128;
    const double input = 6.25;
    std::memcpy(&cpu.v[0][0], &input, sizeof(input));
    run_function(cpu, code, 0);
    double output = 0;
    std::memcpy(&output, &cpu.v[0][0], sizeof(output));
    require(output == input, "scalar FP memory roundtrip");
    std::uint64_t stored = 0;
    for (unsigned i = 0; i < 8; ++i)
        stored |= std::uint64_t(cpu.memory[128 + i]) << (i * 8);
    require(stored == cpu.v[0][0], "scalar FP store width and byte order");
    std::cout << "SeaBird C-generated scalar FP memory execution passed\n";
}

void c_vector_memory_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu cpu;
    cpu.r[0] = 256;
    cpu.v[0] = {0x1122334455667788ULL, 0xFFEEDDCCBBAA0099ULL};
    run_function(cpu, code, 0);
    require(cpu.v[0][0] == 0x1122334455667788ULL &&
            cpu.v[0][1] == 0xFFEEDDCCBBAA0099ULL,
            "128-bit vector memory roundtrip");
    std::cout << "SeaBird C-generated vector memory execution passed\n";
}

void c_fp_convert_test(const std::string& path) {
    const auto code = read_binary(path);
    Cpu to_fp;
    to_fp.r[0] = static_cast<std::uint64_t>(-17LL);
    run_function(to_fp, code, 0);
    double converted = 0;
    std::memcpy(&converted, &to_fp.v[0][0], sizeof(converted));
    require(converted == -17.0, "signed i64 to binary64 conversion");

    Cpu to_int;
    const double input = -23.75;
    std::memcpy(&to_int.v[0][0], &input, sizeof(input));
    run_function(to_int, code, 6);
    require(static_cast<std::int64_t>(to_int.r[0]) == -23,
            "binary64 to signed i64 conversion");
    std::cout << "SeaBird C-generated FP conversion execution passed\n";
}

void run_elf_test(const std::string& path) {
    const auto elf = read_binary(path);
    auto field = [&](std::size_t offset, unsigned bytes) {
        std::size_t p = offset;
        return read_le(elf, p, bytes);
    };
    require(elf.size() >= 120 && elf[0] == 0x7F && elf[1] == 'E' &&
            elf[2] == 'L' && elf[3] == 'F', "invalid ELF executable");
    require(field(16, 2) == 2 && field(18, 2) == 0x5342,
            "ELF is not a SeaBird executable");
    const auto entry = field(24, 8);
    const auto phoff = field(32, 8);
    const auto phentsize = field(54, 2);
    const auto phnum = field(56, 2);
    for (unsigned i = 0; i < phnum; ++i) {
        const auto ph = phoff + i * phentsize;
        if (field(ph, 4) != 1 || !(field(ph + 4, 4) & 1))
            continue;
        const auto offset = field(ph + 8, 8);
        const auto address = field(ph + 16, 8);
        const auto size = field(ph + 32, 8);
        require(offset + size <= elf.size() && entry >= address &&
                entry < address + size, "invalid executable load segment");
        std::vector<std::uint8_t> code(elf.begin() + offset,
                                       elf.begin() + offset + size);
        Cpu cpu;
        run_function(cpu, code, static_cast<std::size_t>(entry - address));
        require(cpu.r[0] == 42, "hosted main exit status");
        std::cout << "SeaBird hosted ELF execution passed (exit 42)\n";
        return;
    }
    throw std::runtime_error("ELF has no executable load segment");
}

void self_test() {
    Cpu cpu;
    cpu.r[9] = 4;
    cpu.r[8] = 7;
    auto add = decode({0xFE, 0x80, 0x20, 0xC8, 0x05});
    require(add.reg == 9 && add.rm == 8, "OREX register decode");
    auto rdcr = decode({0x80, 0xC1, 0x00, 0x03});
    require(rdcr.rm == 1 && rdcr.system_register == 0x0300,
            "RDCR wide system-register decode");
    auto wrcr = decode({0xFE, 0x80, 0x81, 0xC1, 0x08, 0x3F, 0x03});
    require(wrcr.rm == 17 && wrcr.system_register == 0x033F,
            "WRCR OREX and wide system-register decode");
    execute(cpu, add);
    require(cpu.r[9] == 11, "high-register ADD execution");

    auto movi = decode({0xFE, 0x80, 0x01, 0xC0, 0x04, 0x88, 0x77, 0x66, 0x55,
                        0x44, 0x33, 0x22, 0x11});
    execute(cpu, movi);
    require(cpu.r[8] == 0x1122334455667788ULL, "64-bit little-endian immediate");

    cpu.r[1] = 0;
    cpu.r[0] = 1;
    execute(cpu, decode({0x22, 0xC8}));
    require(cpu.r[1] == UINT64_MAX && cpu.cf && cpu.sf, "SUB flags");

    cpu.r[1] = 100;
    cpu.r[3] = 2;
    const std::uint64_t memory_value = 0x8877665544332211ULL;
    for (unsigned i = 0; i < 8; ++i)
        cpu.memory[124 + i] = static_cast<std::uint8_t>(memory_value >> (i * 8));
    execute(cpu, decode({0x16, 0x54, 0x99, 0x10}));
    require(cpu.r[2] == memory_value, "SIB indexed memory execution");

    const float fp32_lhs = 1.5f;
    const float fp32_rhs = 2.25f;
    std::memcpy(&cpu.v[1][0], &fp32_lhs, sizeof(fp32_lhs));
    std::memcpy(&cpu.v[2][0], &fp32_rhs, sizeof(fp32_rhs));
    execute(cpu, decode({0xFE, 0x40, 0xA6, 0xC1, 0x10, 0x22}));
    float fp32_result = 0;
    std::memcpy(&fp32_result, &cpu.v[0][0], sizeof(fp32_result));
    require(fp32_result == 3.75f, "binary32 FADD execution");
    std::cout << "SeaBird reference-model self-test passed\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 5 && std::string(argv[1]) == "--expect-result") {
            const auto code = read_binary(argv[2]);
            const auto entry =
                static_cast<std::size_t>(std::stoull(argv[3], nullptr, 0));
            const auto expected = std::stoull(argv[4], nullptr, 0);
            Cpu cpu;
            std::copy(code.begin(), code.end(), cpu.memory.begin());
            run_function(cpu, code, entry);
            require(cpu.r[0] == expected, "generated function result");
            std::cout << "SeaBird generated function returned "
                      << expected << "\n";
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "--expect-fp-result") {
            const auto code = read_binary(argv[2]);
            const auto entry =
                static_cast<std::size_t>(std::stoull(argv[3], nullptr, 0));
            const double expected = std::stod(argv[4]);
            Cpu cpu;
            std::copy(code.begin(), code.end(), cpu.memory.begin());
            run_function(cpu, code, entry);
            double result = 0;
            std::memcpy(&result, &cpu.v[0][0], sizeof(result));
            if (result != expected)
                throw std::runtime_error(
                    "generated FP function returned " +
                    std::to_string(result) + ", expected " +
                    std::to_string(expected));
            std::cout << "SeaBird generated FP function returned "
                      << expected << "\n";
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "--expect-fp32-result") {
            const auto code = read_binary(argv[2]);
            const auto entry =
                static_cast<std::size_t>(std::stoull(argv[3], nullptr, 0));
            const float expected = std::stof(argv[4]);
            Cpu cpu;
            std::copy(code.begin(), code.end(), cpu.memory.begin());
            run_function(cpu, code, entry);
            float result = 0;
            std::memcpy(&result, &cpu.v[0][0], sizeof(result));
            if (result != expected)
                throw std::runtime_error(
                    "generated binary32 function returned " +
                    std::to_string(result) + ", expected " +
                    std::to_string(expected));
            std::cout << "SeaBird generated binary32 function returned "
                      << expected << "\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--self-test") {
            self_test();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-smoke") {
            c_smoke_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-call") {
            c_call_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-branch") {
            c_branch_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-memory") {
            c_memory_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-stack") {
            c_stack_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-linked") {
            c_linked_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-indirect") {
            c_indirect_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-stack-args") {
            c_stack_args_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-ordered") {
            c_ordered_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-fp-vector") {
            c_fp_vector_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-fp-memory") {
            c_fp_memory_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-vector-memory") {
            c_vector_memory_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--c-fp-convert") {
            c_fp_convert_test(argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--elf") {
            run_elf_test(argv[2]);
            return 0;
        }
        std::cerr << "usage: seabird-ref --self-test | --expect-result FILE "
                     "OFFSET VALUE | --expect-fp-result FILE OFFSET VALUE | "
                     "--expect-fp32-result FILE OFFSET VALUE | "
                     "--c-smoke FILE | --c-call FILE | "
                     "--c-branch FILE | --c-memory FILE | --c-stack FILE | "
                     "--c-linked FILE | --c-indirect FILE | "
                     "--c-stack-args FILE | --c-ordered FILE\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "reference model failure: " << e.what() << '\n';
        return 1;
    }
}

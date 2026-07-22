#!/usr/bin/env python3
"""Static SeaBird linker for freestanding ELF64 objects."""

import argparse
import struct
from dataclasses import dataclass, field
from pathlib import Path

EM_SEABIRD = 0x5342
SECTION_ORDER = (".text", ".rodata", ".data", ".tdata", ".tbss", ".bss")
RELOC_WIDTH = {1: 2, 2: 4, 3: 8}
R_SB_PCREL32 = 7
R_SB_TLS_LE = 13
R_SB_RELATIVE = 14
R_SB_JUMP_SLOT = 15
R_SB_GLOB_DAT = 16


@dataclass
class InputSection:
    index: int
    data: bytes
    alignment: int
    base: int = 0


@dataclass
class ObjectFile:
    path: Path
    sections: dict[str, InputSection]
    symbols: list
    relocs: list
    index_locations: dict[int, tuple[str, int]] = field(default_factory=dict)


def cstring(data, offset):
    end = data.find(b"\0", offset)
    return data[offset:end].decode("ascii")


def align(value, alignment):
    return (value + alignment - 1) & -alignment


def output_section_name(name):
    for canonical in SECTION_ORDER:
        if name == canonical or name.startswith(canonical + "."):
            return canonical
    return None


def read_object(path):
    data = Path(path).read_bytes()
    if data[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", data, 18)[0] != EM_SEABIRD:
        raise ValueError(f"{path}: not a little-endian ELF64 SeaBird object")
    shoff = struct.unpack_from("<Q", data, 40)[0]
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 58)
    headers = [struct.unpack_from("<IIQQQQIIQQ", data, shoff + i * shentsize)
               for i in range(shnum)]
    shstr = headers[shstrndx]
    names = data[shstr[4]:shstr[4] + shstr[5]]
    named = {cstring(names, sh[0]): (i, sh) for i, sh in enumerate(headers)}

    merged = {name: bytearray() for name in SECTION_ORDER}
    merged_alignment = {name: 1 for name in SECTION_ORDER}
    index_locations = {}
    for name, (index, sh) in named.items():
        output_name = output_section_name(name)
        if not output_name:
            continue
        section_alignment = max(1, sh[8])
        local_base = align(len(merged[output_name]), section_alignment)
        merged[output_name].extend(bytes(local_base - len(merged[output_name])))
        contents = bytes(sh[5]) if sh[1] == 8 else data[sh[4]:sh[4] + sh[5]]
        merged[output_name].extend(contents)
        merged_alignment[output_name] = max(
            merged_alignment[output_name], section_alignment)
        index_locations[index] = (output_name, local_base)
    input_sections = {
        name: InputSection(-1, bytes(contents), merged_alignment[name])
        for name, contents in merged.items() if contents
    }

    _, sym_sh = named[".symtab"]
    str_sh = headers[sym_sh[6]]
    strings = data[str_sh[4]:str_sh[4] + str_sh[5]]
    symbols = []
    for offset in range(sym_sh[4], sym_sh[4] + sym_sh[5], sym_sh[9]):
        name, info, _, shndx, value, size = struct.unpack_from("<IBBHQQ", data, offset)
        symbols.append((cstring(strings, name), info >> 4, shndx, value, size))

    relocs = []
    for input_name, (input_index, _) in named.items():
        location = index_locations.get(input_index)
        if not location:
            continue
        section_name, local_base = location
        relocation_name = ".rela" + input_name
        if relocation_name not in named:
            continue
        _, rela_sh = named[relocation_name]
        for offset in range(rela_sh[4], rela_sh[4] + rela_sh[5], rela_sh[9]):
            place, info, addend = struct.unpack_from("<QQq", data, offset)
            relocs.append((section_name, local_base + place, info >> 32,
                           info & 0xFFFFFFFF, addend))
    return ObjectFile(
        Path(path), input_sections, symbols, relocs, index_locations)


def link(paths, load_base=0, page_separate=False):
    objects = [read_object(path) for path in paths]
    image = bytearray()
    boundaries = {}
    first_writable = next((name for name in (".data", ".tdata", ".tbss", ".bss")
                           if any(name in obj.sections for obj in objects)), None)
    for section_name in SECTION_ORDER:
        present = any(section_name in obj.sections for obj in objects)
        if page_separate and (present and section_name in (".rodata", ".data") or
                              section_name == first_writable):
            image.extend(bytes(align(len(image), 0x1000) - len(image)))
        start = len(image)
        for obj in objects:
            section = obj.sections.get(section_name)
            if not section:
                continue
            padding = align(len(image), section.alignment) - len(image)
            image.extend(bytes(padding))
            section.base = len(image)
            image.extend(section.data)
        boundaries[section_name] = (start, len(image))

    definitions = {}
    for obj in objects:
        for name, binding, shndx, value, _ in obj.symbols:
            location = obj.index_locations.get(shndx)
            if not name or binding == 0 or not location:
                continue
            section_name, local_base = location
            address = obj.sections[section_name].base + local_base + value
            if name in definitions:
                raise ValueError(f"duplicate symbol: {name}")
            definitions[name] = address

    for obj in objects:
        for section_name, offset, symbol_index, kind, addend in obj.relocs:
            place = obj.sections[section_name].base + offset
            if kind == R_SB_RELATIVE:
                image[place:place + 8] = (load_base + addend).to_bytes(
                    8, "little", signed=False)
                continue
            name, _, shndx, value, _ = obj.symbols[symbol_index]
            target_location = obj.index_locations.get(shndx)
            if target_location:
                target_section, local_base = target_location
                target = obj.sections[target_section].base + local_base + value
            elif name in definitions:
                target_section = None
                target = definitions[name]
            else:
                raise ValueError(f"undefined symbol: {name}")
            if kind == R_SB_PCREL32:
                value_to_write = target + addend - place
                if not -(1 << 31) <= value_to_write < (1 << 31):
                    raise ValueError(f"relocation overflow for {name}")
                struct.pack_into("<i", image, place, value_to_write)
            elif kind in RELOC_WIDTH:
                width = RELOC_WIDTH[kind]
                value_to_write = load_base + target + addend
                if not 0 <= value_to_write < (1 << (width * 8)):
                    raise ValueError(f"absolute relocation overflow for {name}")
                image[place:place + width] = value_to_write.to_bytes(width, "little")
            elif kind == R_SB_TLS_LE:
                if target_section not in (".tdata", ".tbss"):
                    raise ValueError(f"TLS_LE relocation targets non-TLS symbol: {name}")
                tls_base = boundaries[".tdata"][0]
                value_to_write = target - tls_base + addend
                image[place:place + 8] = value_to_write.to_bytes(8, "little", signed=True)
            elif kind in (R_SB_JUMP_SLOT, R_SB_GLOB_DAT):
                value_to_write = load_base + target + addend
                image[place:place + 8] = value_to_write.to_bytes(8, "little")
            else:
                raise ValueError(f"{obj.path}: unsupported relocation {kind}")
    return image, definitions, boundaries


def make_executable(image, definitions, boundaries, entry_name, base):
    if entry_name not in definitions:
        raise ValueError(f"entry symbol is undefined: {entry_name}")
    file_offset = 0x1000
    entry = base + definitions[entry_name]
    ident = b"\x7fELF\x02\x01\x01\x00" + bytes(8)
    groups = []
    text_start, text_end = boundaries[".text"]
    ro_start, ro_end = boundaries[".rodata"]
    data_start, data_end = boundaries[".data"]
    tdata_start, tdata_end = boundaries[".tdata"]
    _, bss_end = boundaries[".bss"]
    _, tbss_end = boundaries[".tbss"]
    if text_end > text_start:
        groups.append((text_start, text_end, text_end, 5))
    if ro_end > ro_start:
        groups.append((ro_start, ro_end, ro_end, 4))
    writable_start = min(start for start, end in
                         (boundaries[".data"], boundaries[".tdata"],
                          boundaries[".bss"], boundaries[".tbss"])
                         if end > start) if any(end > start for start, end in
                         (boundaries[".data"], boundaries[".tdata"],
                          boundaries[".bss"], boundaries[".tbss"])) else 0
    writable_file_end = max(data_end, tdata_end)
    writable_mem_end = max(bss_end, tbss_end, writable_file_end)
    if writable_mem_end > writable_start:
        groups.append((writable_start, writable_file_end, writable_mem_end, 6))
    header = ident + struct.pack(
        "<HHIQQQIHHHHHH", 2, EM_SEABIRD, 1, entry, 64, 0, 0,
        64, 56, len(groups) + (1 if tdata_end > tdata_start else 0), 0, 0, 0)
    programs = b"".join(struct.pack(
        "<IIQQQQQQ", 1, flags, file_offset + start, base + start,
        base + start, file_end - start, mem_end - start, 0x1000)
        for start, file_end, mem_end, flags in groups)
    if tdata_end > tdata_start:
        programs += struct.pack("<IIQQQQQQ", 7, 4, file_offset + tdata_start,
                                base + tdata_start, base + tdata_start,
                                tdata_end - tdata_start, tbss_end - tdata_start,
                                16)
    file_image_end = writable_file_end if writable_mem_end > writable_file_end else len(image)
    return (header + programs + bytes(file_offset - len(header) - len(programs)) +
            image[:file_image_end])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output", required=True, type=Path)
    parser.add_argument("--format", choices=("binary", "elf"), default="binary")
    parser.add_argument("--entry", default="_start")
    parser.add_argument("--base", type=lambda value: int(value, 0), default=0x10000)
    parser.add_argument("objects", nargs="+", type=Path)
    args = parser.parse_args()
    load_base = args.base if args.format == "elf" else 0
    image, definitions, boundaries = link(
        args.objects, load_base, page_separate=args.format == "elf")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    output = (make_executable(image, definitions, boundaries, args.entry, args.base)
              if args.format == "elf" else image)
    args.output.write_bytes(output)
    print(f"linked {len(args.objects)} objects into {args.output} ({len(output)} bytes, {args.format})")
    for name, address in sorted(definitions.items(), key=lambda item: item[1]):
        print(f"{address:08x} {name}")


if __name__ == "__main__":
    main()

import os
import re
import struct
import sys

INST_FORMAT = "<Bqqq"
INST_SIZE = struct.calcsize(INST_FORMAT)
REGISTER_COUNT = 16

OPCODES = {
    "nop": 0, "halt": 1, "move": 2, "set": 3, "load": 4, "store": 5,
    "print": 6, "jump": 7, "jump_if": 8,
    "add": 9, "sub": 10, "mul": 11, "div": 12, "mod": 13,
    "equ": 14, "neq": 15, "lth": 16, "gth": 17, "leq": 18, "geq": 19,
    "and": 20, "or": 21, "xor": 22, "not": 23, "shl": 24, "shr": 25,
}

SIGNATURES = {
    "nop": (), "halt": (),
    "move": ("R", "R"), "not": ("R", "R"),
    "set": ("R", "I"), "load": ("R", "I"), "store": ("R", "I"),
    "print": ("R",),
    "jump": ("L",), "jump_if": ("R", "L"),
}
for mnemonic in ("add", "sub", "mul", "div", "mod", "equ", "neq", "lth",
                 "gth", "leq", "geq", "and", "or", "xor", "shl", "shr"):
    SIGNATURES[mnemonic] = ("R", "R", "R")


class AsmError(Exception):
    pass


def parse_register(token, lineno):
    match = re.fullmatch(r"[rR](\d+)", token)
    if match is None:
        raise AsmError(f"line {lineno}: expected register, got '{token}'")
    index = int(match.group(1))
    if not 0 <= index < REGISTER_COUNT:
        raise AsmError(f"line {lineno}: bad register '{token}'")
    return index


def parse_immediate(token, lineno):
    try:
        value = int(token, 0)
    except ValueError:
        try:
            value = int(token, 10)
        except ValueError:
            raise AsmError(f"line {lineno}: expected immediate, got '{token}'") from None
    if not -(1 << 63) <= value < (1 << 63):
        raise AsmError(f"line {lineno}: immediate {value} does not fit int64")
    return value


def strip_comment(line):
    cut = len(line)
    for symbol in ";#":
        position = line.find(symbol)
        if position != -1:
            cut = min(cut, position)
    return line[:cut].strip()


def parse_lines(source):
    labels = {}
    instructions = []

    for lineno, raw in enumerate(source.splitlines(), 1):
        line = strip_comment(raw)

        while True:
            match = re.match(r"([A-Za-z_]\w*)\s*:\s*(.*)", line)
            if match is None:
                break
            name = match.group(1)
            if name in labels:
                raise AsmError(f"line {lineno}: duplicate label '{name}'")
            labels[name] = len(instructions)
            line = match.group(2).strip()

        if not line:
            continue

        tokens = []
        for token in line.split():
            token = token.rstrip(",")
            if token:
                tokens.append(token)

        instructions.append((lineno, tokens[0].lower(), tokens[1:]))

    return labels, instructions


def assemble(source):
    labels, instructions = parse_lines(source)

    code = bytearray()
    listing = []

    for index, (lineno, mnemonic, args) in enumerate(instructions):
        if mnemonic not in OPCODES:
            raise AsmError(f"line {lineno}: unknown instruction '{mnemonic}'")

        signature = SIGNATURES[mnemonic]
        if len(args) != len(signature):
            raise AsmError(f"line {lineno}: '{mnemonic}' expects "
                           f"{len(signature)} operand(s), got {len(args)}")

        operands = [0, 0, 0]
        shown = []

        for slot, (kind, token) in enumerate(zip(signature, args)):
            if kind == "R":
                value = parse_register(token, lineno)
                shown.append(f"r{value}")
            elif kind == "I":
                value = parse_immediate(token, lineno)
                shown.append(str(value))
            else:
                if token not in labels:
                    raise AsmError(f"line {lineno}: undefined label '{token}'")
                value = labels[token]
                shown.append(f"{token}({value})")
            operands[slot] = value

        code += struct.pack(INST_FORMAT, OPCODES[mnemonic], *operands)
        listing.append((index, len(code) - INST_SIZE, mnemonic, " ".join(shown)))

    return bytes(code), listing, labels


def main():
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <input.aasm> [output.avmb]", file=sys.stderr)
        return 1

    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else \
        os.path.splitext(input_path)[0] + ".avmb"

    try:
        with open(input_path, "r", encoding="utf-8") as file:
            source = file.read()
    except OSError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    try:
        code, listing, labels = assemble(source)
    except AsmError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    with open(output_path, "wb") as file:
        file.write(code)

    for name, index in labels.items():
        print(f"label {name} -> {index}")
    for index, offset, mnemonic, operands in listing:
        print(f"{index:4} @{offset:<4} {mnemonic:<8} {operands}")
    print(f"{len(code)} bytes -> {output_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

import re


class Emitter:
    def __init__(self):
        self.lines = []
        self.indent = 0

    def emit(self, line):
        self.lines.append(" " * 4 * self.indent + line)

    def begin_block(self, name):
        self.emit(name)
        self.emit("{")
        self.indent += 1

    def end_block(self, semicolon=True):
        self.indent -= 1
        self.emit("};" if semicolon else "}")

    def separator(self):
        self.emit("")
        self.emit("")

    def __str__(self):
        return "\n".join(self.lines)


class Layout:
    AOS = 0
    SOA = 1

    def __init__(self, name):
        if name == "aos":
            self.value = Layout.AOS
        elif name == "soa":
            self.value = Layout.SOA
        else:
            raise ValueError(f"Invalid layout: {name}")


class Field:
    def __init__(self, type: str, name: str):
        self.type = type
        self.name = name
    
    def get_size(self):
        if self.type.startswith("Ref<"):
            return "sizeof(uint32_t)"
        return f"sizeof({self.type})"


class Table:
    def __init__(self, name: str, layout: Layout, fields: list[Field]):
        self.name = name
        self.layout = layout
        self.fields = fields

    @staticmethod
    def calc_total_size(fields: list[Field]):
        if not fields:
            return "(0)"

        return "(" + " + ".join(field.get_size() for field in fields) + ")"

    def calc_offsets(self) -> list[tuple[Field, str]]:
        prev_fields = []
        result = []
        for field in self.fields:
            offset = Table.calc_total_size(prev_fields)
            prev_fields.append(field)
            result.append((field, offset))
        return result

    def get_size(self):
        return Table.calc_total_size(self.fields)


def parse_table(src: str) -> Table:
    header = re.search(r"table\s+(aos|soa)\s+(\w+)\s*{", src)
    if not header:
        raise ValueError("Invalid table definition.")
    layout, name = header.groups()

    body = re.search(r"{(.*)}", src, re.S)
    if not body:
        raise ValueError("Invalid table definition.")
    fields = []
    for line in body.group(1).split(";"):
        line = line.strip()
        if not line:
            continue
        t, n = line.split()[:2]
        fields.append(Field(t.strip(), n.strip()))

    return Table(name, Layout(layout), fields)


def gen_reading_hlsl(emitter: Emitter, fields: list[tuple[Field, str]]):
    started = False
    for field, offset in fields:
        if started:
            emitter.separator()
        started = True
        emitter.begin_block(f"{field.type} Load{field.name}(Ref<{table.name}> index)")
        emitter.emit(f"const uint32_t rawIndex = index.m_rawIndex;")

        if field.type.startswith("Ref<"):
            emitter.emit(f"const uint32_t value = m_buffer.Load<uint32_t>(kSize * rawIndex + {offset});")
            emitter.emit(f"return {field.type}(value);")
        else:
            emitter.emit(f"return m_buffer.Load<{field.type}>(kSize * rawIndex + {offset});")

        emitter.end_block(semicolon=False)


def gen_writing_hlsl(emitter: Emitter, fields: list[tuple[Field, str]]):
    for field, offset in fields:
        emitter.separator()
        emitter.begin_block(f"void Store{field.name}(Ref<{table.name}> index, {field.type} value)")
        emitter.emit(f"const uint32_t rawIndex = index.m_rawIndex;")
        emitter.emit(f"m_buffer.Store(kSize * rawIndex + {offset}, value);")
        emitter.end_block(semicolon=False)


def gen_hlsl(table: Table):
    emitter = Emitter()
    emitter.begin_block("namespace DB")

    emitter.begin_block(f"struct {table.name}")
    emitter.emit(f"static const uint32_t kSize = {table.get_size()};")
    emitter.emit(f"static const uint32_t kRowsPerPage = kTablePageSize / kSize;")
    emitter.emit("")

    offsets = table.calc_offsets()

    emitter.begin_block("struct Table")
    gen_reading_hlsl(emitter, offsets)
    emitter.end_block()

    emitter.separator()

    emitter.begin_block("struct RWTable")
    gen_reading_hlsl(emitter, offsets)
    gen_writing_hlsl(emitter, offsets)
    emitter.end_block()

    emitter.end_block()

    emitter.end_block(semicolon=False)

    return str(emitter)


def gen_cpp(table: Table):
    fields = table.calc_offsets()
    whole_size = "+".join(field.get_size() for field,_ in fields)

    lines = [
        f"struct {table.name}Table {{", "    festd::vector<std::byte> m_buffer;",
        f"    static constexpr uint32_t kSize = {whole_size};",
    ]
    for field, offset in fields:
        lines.append(f"    static constexpr uint32_t kOffset_{field.name} = {offset};")
    lines.append("};")
    return "\n".join(lines)


if __name__ == "__main__":
    src = """
    table soa MyObject {
        float4x4 Transform;
        Ref<MyObject> Parent;
        uint Flags;
    }
    """

    table = parse_table(src)
    print("// === HLSL ===")
    print(gen_hlsl(table))
    print()
    print("// === C++ ===")
    print(gen_cpp(table))

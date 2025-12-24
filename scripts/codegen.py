from clang import cindex
from pathlib import Path
from jinja2 import Environment, FileSystemLoader, select_autoescape
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
from enum import Flag, Enum
import subprocess
import uuid
import json

UUID_NAMESPACE = uuid.UUID("ba6b72ef-3286-4c71-9822-2519da4e156a")
USE_INTERNAL_ID = uuid.UUID("9ecf45e0-cba3-4e15-b613-ed66e3c4be5d")

PROJECT_DIR = Path.cwd().parent.absolute().as_posix()

LLVM_DIR = PROJECT_DIR + "/ThirdParty/llvm"

jinja_env = Environment(
    loader=FileSystemLoader("./templates"),
    autoescape=select_autoescape()
)

generated_file_template = jinja_env.get_template("ReflectedType.cpp")

cindex.Config.set_library_file(LLVM_DIR + "/libclang.dll")
index = cindex.Index.create()

base_compiler_args = [
    '-x', 'c++',
    '-std=c++17',
]

base_compiler_args.append(f"-DEASTL_USER_CONFIG_HEADER=\"{Path("../FerrumCore/Private/FeCore/Base/EASTLConfig.h").absolute().as_posix()}\"")


def run_clang_format(file_path: str) -> None:
    subprocess.run([LLVM_DIR + "/clang-format.exe", "-i", file_path, "-style=file:../.clang-format"])


class TypeKind(Enum):
    NORMAL = 0
    BUILTIN = 1
    EXTERNAL_CLASS = 2
    ENUM = 3


# Should be kept in sync with Reflection.h
class FieldFlags(Flag):
    NONE = 0
    STATIC = 1 << 0
    INSTANCE = 1 << 1
    PRIVATE = 1 << 2
    PROTECTED = 1 << 3
    PUBLIC = 1 << 4
    POINTER = 1 << 5

    def __str__(self) -> str:
        return ' | '.join(["Rtti::FieldFlags::k" + (x.name or "None").capitalize() for x in self])


class FieldInfo:
    def __init__(self, name: str, attributes: dict[str, str], flags: FieldFlags, type, enum_value = None, array_size = 1):
        self.name = name
        self.attributes = attributes
        self.flags = flags
        self.type = type
        self.enum_value = enum_value
        self.array_size = array_size
        self.display_name = attributes.get("DisplayName", name)


def get_internal_type_id(qualified_name: str) -> uuid.UUID:
    return uuid.uuid5(UUID_NAMESPACE, qualified_name)


def get_module_path(type_declaration_file_path: str) -> Path:
    file_path = Path(type_declaration_file_path).relative_to(PROJECT_DIR)
    for part in file_path.parents:
        if part.parent.name == "Samples":
            return Path(PROJECT_DIR) / part
        
        if part.name == "Shaders":
            return Path(PROJECT_DIR) / "Modules/Graphics/Framework"

        if part.name == "Public" or part.name == "Private":
            return Path(PROJECT_DIR) / part.parent

    raise Exception(f"Failed to find module path for {type_declaration_file_path}")


def get_header_path(type_declaration_file_path: str, module_path: Path) -> Path:
    relative_path = Path(type_declaration_file_path).relative_to(module_path)
    if relative_path.parts[0] == "Public" or relative_path.parts[0] == "Private" or relative_path.parts[0] == "Shaders":
        return Path(*relative_path.parts[1:])
    else:
        return relative_path


class ReflectedType:
    def __init__(self, kind: TypeKind, need_reflect: bool, id: uuid.UUID, namespace: str, name: str, location, attributes: dict[str, str], bases, fields: list[FieldInfo]):
        self.need_reflect = need_reflect
        self.name = name
        self.namespace = namespace
        self.qualified_name = f"{namespace}::{name}" if namespace else name

        self.internal_id = get_internal_type_id(self.qualified_name)
        self.id = self.internal_id if id == USE_INTERNAL_ID else id

        self.module_path = get_module_path(location.file.name)
        self.header_path = get_header_path(location.file.name, self.module_path)
        if self.header_path.suffix != ".h":
            raise Exception(f"Reflected types must be declared in header files, but {self.qualified_name} is declared in {location}")

        self.attributes = attributes
        self.bases = bases
        self.fields = fields
        self.is_builtin = kind == TypeKind.BUILTIN
        self.is_enum = kind == TypeKind.ENUM
        self.is_external = self.is_builtin or self.is_enum or kind == TypeKind.EXTERNAL_CLASS


def get_qualified_name(cursor: cindex.Cursor | None) -> str:
    if cursor is None or cursor.kind == cindex.CursorKind.TRANSLATION_UNIT:
        return ''

    parent_name = get_qualified_name(cursor.semantic_parent)

    if parent_name:
        return parent_name + '::' + cursor.displayname
    else:
        return cursor.displayname


def get_builtin_type_name(t: cindex.Type) -> str|None:
    if t.kind == cindex.TypeKind.CHAR_S or t.kind == cindex.TypeKind.SCHAR:
        return 'int8_t'
    if t.kind == cindex.TypeKind.CHAR_U or t.kind == cindex.TypeKind.UCHAR:
        return 'uint8_t'
    if t.kind == cindex.TypeKind.SHORT:
        return 'int16_t'
    if t.kind == cindex.TypeKind.USHORT:
        return 'uint16_t'
    if t.kind == cindex.TypeKind.INT:
        return 'int32_t'
    if t.kind == cindex.TypeKind.UINT:
        return 'uint32_t'
    if t.kind == cindex.TypeKind.LONG or t.kind == cindex.TypeKind.LONGLONG:
        return 'int64_t'
    if t.kind == cindex.TypeKind.ULONG or t.kind == cindex.TypeKind.ULONGLONG:
        return 'uint64_t'
    if t.kind == cindex.TypeKind.FLOAT:
        return 'float'
    if t.kind == cindex.TypeKind.DOUBLE:
        return 'double'
    if t.kind == cindex.TypeKind.BOOL:
        return 'bool'

    return None


def get_namespace(cursor: cindex.Cursor | None) -> str:
    if cursor is None or cursor.kind == cindex.CursorKind.TRANSLATION_UNIT:
        return ''

    return get_qualified_name(cursor.semantic_parent)


def is_class(node_kind: cindex.CursorKind) -> bool:
    return node_kind == cindex.CursorKind.STRUCT_DECL or node_kind == cindex.CursorKind.CLASS_DECL


def is_enum(node_kind: cindex.CursorKind) ->bool:
    return node_kind == cindex.CursorKind.ENUM_DECL


def get_annotation_tokens(node: cindex.Cursor) -> list[str]:
    annotations = []
    for child_node in node.get_children():
        if child_node.kind == cindex.CursorKind.ANNOTATE_ATTR:
            annotations.append(child_node.spelling or child_node.displayname or "")
    return annotations


def parse_rtti_id(id_string: str) -> uuid.UUID:
    if id_string == 'Random':
        return USE_INTERNAL_ID
    else:
        return uuid.UUID(id_string)


def parse_rtti_attribute(annotations: dict[str, str]) -> tuple[uuid.UUID | None, bool]:
    for annotation, type_id in annotations.items():
        if annotation == "ReflectFull":
            return parse_rtti_id(type_id), True
        if annotation == "ReflectBasic":
            return parse_rtti_id(type_id), False
    return None, False


def parse_attributes(node: cindex.Cursor) -> dict[str, str]:
    annotations = get_annotation_tokens(node)

    attributes = {}
    for annotation_list in annotations:
        for annotation in annotation_list.split(";"):
            key, value = annotation.split("=", 1)
            attributes[key.strip()] = value.strip()

    return attributes


def get_field_flags(node: cindex.Cursor, access_specifier: cindex.AccessSpecifier) -> FieldFlags:
    assert node.kind == cindex.CursorKind.FIELD_DECL
    flags = FieldFlags.NONE

    if access_specifier == cindex.AccessSpecifier.PUBLIC:
        flags |= FieldFlags.PUBLIC
    elif access_specifier == cindex.AccessSpecifier.PROTECTED:
        flags |= FieldFlags.PROTECTED
    elif access_specifier == cindex.AccessSpecifier.PRIVATE:
        flags |= FieldFlags.PRIVATE

    if node.storage_class == cindex.StorageClass.STATIC:
        flags |= FieldFlags.STATIC
    else:
        flags |= FieldFlags.INSTANCE

    assert flags != FieldFlags.NONE
    return flags


def resolve_type(t: cindex.Type, types: dict[uuid.UUID, ReflectedType]) -> ReflectedType | None:
    type_qualified_name = get_builtin_type_name(t)
    if type_qualified_name is None:
        if t.kind == cindex.TypeKind.CONSTANTARRAY:
            return resolve_type(t.get_array_element_type(), types)
        type_declaration = t.get_declaration()
        if type_declaration is None:
            return None

        type_qualified_name = get_qualified_name(type_declaration)
        if type_qualified_name is None:
            return None

    type_internal_id = get_internal_type_id(type_qualified_name)
    return types.get(type_internal_id)


def get_all_base_types(immediate_bases: list[cindex.Type], types: dict[uuid.UUID, ReflectedType]) -> list[ReflectedType]:
    bases: list[ReflectedType] = []

    unvisited = [resolve_type(t, types) for t in immediate_bases]
    while unvisited:
        base_type = unvisited.pop(0)
        if base_type is None or base_type in bases:
            continue

        bases.append(base_type)
        unvisited.extend(base_type.bases)

    return list(bases)


def parse_class(node: cindex.Cursor, fields: list[FieldInfo], base_types: list[cindex.Type], types: dict[uuid.UUID, ReflectedType], need_reflect: bool, public_only: bool):
    access_spec = cindex.AccessSpecifier.PUBLIC if node.kind == cindex.CursorKind.STRUCT_DECL else cindex.AccessSpecifier.PRIVATE
    for child in node.get_children():
        if child.kind == cindex.CursorKind.CXX_ACCESS_SPEC_DECL:
            access_spec = child.access_specifier
        if child.kind == cindex.CursorKind.CXX_BASE_SPECIFIER:
            base_types.append(child.type)
        if child.kind == cindex.CursorKind.FIELD_DECL and need_reflect:
            field_name = child.spelling
            canonical_type = child.type.get_canonical()
            field_type = resolve_type(canonical_type, types)
            array_size = canonical_type.get_array_size() if canonical_type.kind == cindex.TypeKind.CONSTANTARRAY else 1
            field_attributes = parse_attributes(child)
            field_flags = get_field_flags(child, access_spec)
            if not public_only or access_spec == cindex.AccessSpecifier.PUBLIC:
                fields.append(FieldInfo(field_name, field_attributes, field_flags, field_type, array_size=array_size))


def parse_enum(node: cindex.Cursor, fields: list[FieldInfo], base_types: list[cindex.Type]):
    base_types.append(node.enum_type)
    for child in node.get_children():
        if child.kind == cindex.CursorKind.ENUM_CONSTANT_DECL:
            field_name = child.spelling
            field_value = child.enum_value
            field_attributes = parse_attributes(child)
            fields.append(FieldInfo(field_name, field_attributes, FieldFlags.NONE, None, enum_value=field_value))


def visit_external_rtti_declaration(node: cindex.Cursor, attributes: dict[str, str], types: dict[uuid.UUID, ReflectedType]) -> bool:
    reflected_type_id, need_reflect = parse_rtti_attribute(attributes)
    if reflected_type_id is None:
        return False

    assert need_reflect
    assert node.get_num_template_arguments() == 1

    namespace_name = ""
    type_name = ""
    attributes = {}
    fields = []
    base_types = []

    type_kind = TypeKind.NORMAL
    reflected_type = node.get_template_argument_type(0)
    reflected_type_declaration = reflected_type.get_declaration()
    if reflected_type_declaration and is_class(reflected_type_declaration.kind):
        type_kind = TypeKind.EXTERNAL_CLASS
        namespace_name = get_namespace(reflected_type_declaration)
        type_name = reflected_type_declaration.displayname

        if reflected_type_declaration.location.file.name != node.location.file.name:
            raise Exception(f"FE_RTTI_Reflect macro must be declared in the same file as the reflected type {get_qualified_name(reflected_type_declaration)}")

        parse_class(reflected_type_declaration, fields, [], types, need_reflect, True)
    elif reflected_type_declaration and is_enum(reflected_type_declaration.kind):
        type_kind = TypeKind.ENUM
        namespace_name = get_namespace(reflected_type_declaration)
        type_name = reflected_type_declaration.displayname
        parse_enum(reflected_type_declaration, fields, base_types)
        assert len(base_types) == 1
    else:
        type_kind = TypeKind.BUILTIN
        type_name = get_builtin_type_name(reflected_type)
        assert type_name

    bases = [resolve_type(t, types) for t in base_types]
    assert all(bases)
    ref_type = ReflectedType(type_kind, need_reflect, reflected_type_id, namespace_name, type_name, node.location, attributes, bases, fields)
    types[ref_type.internal_id] = ref_type
    return True


def visit_class(node: cindex.Cursor, types: dict[uuid.UUID, ReflectedType]):
    attributes = parse_attributes(node)
    if visit_external_rtti_declaration(node, attributes, types):
        return

    # Find static Reflect method declaration. It is used as a marker to generate reflection code.
    # We need to generate the implementation for this method.
    reflected_type_id, need_reflect = None, False
    for child in node.get_children():
        if child.is_static_method() and child.spelling == "Reflect":
            annotations = parse_attributes(child)
            reflected_type_id, need_reflect = parse_rtti_attribute(annotations)

    if reflected_type_id:
        # type_description = node.brief_comment
        namespace_name = get_namespace(node)
        attributes = {}
        base_types = []
        fields = []
        parse_class(node, fields, base_types, types, need_reflect, False)

        bases = get_all_base_types(base_types, types)
        ref_type = ReflectedType(TypeKind.NORMAL, need_reflect, reflected_type_id, namespace_name, node.displayname, node.location, attributes, bases, fields)
        types[ref_type.internal_id] = ref_type


def visit(node: cindex.Cursor, types: dict[uuid.UUID, ReflectedType]):
    if is_class(node.kind):
        if node.is_definition():
            visit_class(node, types)
    for c in node.get_children():
        visit(c, types)


def parse_file(file_path: str, include_dirs: list[str], defines: list[str]) -> dict[uuid.UUID, ReflectedType] | None:
    compiler_args = base_compiler_args + [f"-I{x}" for x in include_dirs] + [f"-D{x}" for x in defines]
    tu = index.parse(file_path, args=compiler_args, options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)

    if not tu:
        print("Failed to parse:", file_path)
        return None

    if tu.diagnostics:
        for d in tu.diagnostics:
            if d.severity not in [cindex.Diagnostic.Error, cindex.Diagnostic.Fatal]:
                continue
            if d.location.file:
                if d.location.file.name.startswith("C:\\Program Files\\"):
                    continue
                if "ThirdParty" in d.location.file.name:
                    continue

            print(f"  {d.location}: {d}")

    if tu.cursor:
        types = {}
        visit(tu.cursor, types)
        return types

    return None


def should_parse_file(file_path: str) -> bool:
    # return "Downsample.cpp" in file_path
    return "ThirdParty" not in file_path and file_path.endswith(".cpp") and not file_path.endswith(".gen.cpp")


NUM_WORKERS = 16

if __name__ == "__main__":
    with open("../cmake-build/windows-codegen/compile_commands.json") as f:
        data = json.load(f)
    files = [d for d in data if should_parse_file(d["file"])]
    total_file_count = len(files)

    reflected_types_dict = {}
    with ProcessPoolExecutor(max_workers=NUM_WORKERS) as executor:
        futures = []
        with tqdm(total=total_file_count, desc="Preparing") as pbar:
            for d in files:
                file_path = d["file"]
                command = d["command"]
                command_tokens = command.split(" ")
                include_dirs = [p[2:] for p in command_tokens if p.startswith("-I")]
                include_dirs += [p[11:] for p in command_tokens if p.startswith("-external:I")]
                defines = [p[2:] for p in command_tokens if p.startswith("-D") or p.startswith("/D")]
                if NUM_WORKERS > 1:
                    futures.append(executor.submit(parse_file, file_path, include_dirs, defines))
                else:
                    result = parse_file(file_path, include_dirs, defines)
                    if result:
                        reflected_types_dict.update(result)
                pbar.update(1)

        if NUM_WORKERS > 1:
            with tqdm(total=total_file_count, desc="Processing files") as pbar:
                for future in as_completed(futures):
                    pbar.update(1)
                    result = future.result()
                    if result:
                        reflected_types_dict.update(result)

    reflected_types = reflected_types_dict.values()
    for t in reflected_types:
        same_id = [x for x in reflected_types if x.id == t.id]
        ok = len(same_id) == 1 and same_id[0] is t
        if not ok:
            raise Exception(f'These types have the same id: {', '.join(x.qualified_name for x in same_id)}')

    types_by_module = {}
    for t in reflected_types:
        module_path = t.module_path
        if module_path not in types_by_module:
            types_by_module[module_path] = []
        types_by_module[module_path].append(t)

    with open("./templates/CommonGeneratedHeader.h", "r") as f:
        common_header = f.read()

    for module_path, types in types_by_module.items():
        headers = set()
        for t in types:
            headers.add(t.header_path)

        generated_file = (module_path / "Reflection.gen.cpp").as_posix()
        with open(generated_file, "w") as f:
            f.write(common_header)
            f.write('\n')

            for h in headers:
                f.write(f'#include <{h.as_posix()}>\n')
            f.write('\n\n\n')

            for t in types:
                code = generated_file_template.render(type=t)
                f.write(code)
                f.write('\n\n\n')

        run_clang_format(generated_file)

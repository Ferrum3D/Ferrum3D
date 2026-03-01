from __future__ import annotations

from clang import cindex
from dataclasses import dataclass
from pathlib import Path
import uuid

from .model import (
    FieldFlags,
    FieldInfo,
    ReflectedType,
    TypeKind,
    USE_INTERNAL_ID,
    get_internal_type_id,
)


@dataclass(frozen=True)
class ParseConfig:
    project_dir: Path
    llvm_dir: Path
    base_compiler_args: list[str]


_INDEX = None


def _get_index(llvm_dir: Path) -> cindex.Index:
    global _INDEX
    if _INDEX is None:
        cindex.Config.set_library_file(str(llvm_dir / "libclang.dll"))
        _INDEX = cindex.Index.create()
    return _INDEX


def get_qualified_name(cursor: cindex.Cursor | None) -> str:
    if cursor is None or cursor.kind == cindex.CursorKind.TRANSLATION_UNIT:
        return ""

    parent_name = get_qualified_name(cursor.semantic_parent)

    if parent_name:
        return parent_name + "::" + cursor.displayname
    return cursor.displayname


def get_builtin_type_name(t: cindex.Type) -> str | None:
    if t.kind == cindex.TypeKind.CHAR_S or t.kind == cindex.TypeKind.SCHAR:
        return "int8_t"
    if t.kind == cindex.TypeKind.CHAR_U or t.kind == cindex.TypeKind.UCHAR:
        return "uint8_t"
    if t.kind == cindex.TypeKind.SHORT:
        return "int16_t"
    if t.kind == cindex.TypeKind.USHORT:
        return "uint16_t"
    if t.kind == cindex.TypeKind.INT:
        return "int32_t"
    if t.kind == cindex.TypeKind.UINT:
        return "uint32_t"
    if t.kind == cindex.TypeKind.LONG or t.kind == cindex.TypeKind.LONGLONG:
        return "int64_t"
    if t.kind == cindex.TypeKind.ULONG or t.kind == cindex.TypeKind.ULONGLONG:
        return "uint64_t"
    if t.kind == cindex.TypeKind.FLOAT:
        return "float"
    if t.kind == cindex.TypeKind.DOUBLE:
        return "double"
    if t.kind == cindex.TypeKind.BOOL:
        return "bool"

    return None


def get_namespace(cursor: cindex.Cursor | None) -> str:
    if cursor is None or cursor.kind == cindex.CursorKind.TRANSLATION_UNIT:
        return ""

    return get_qualified_name(cursor.semantic_parent)


def is_class(node_kind: cindex.CursorKind) -> bool:
    return node_kind == cindex.CursorKind.STRUCT_DECL or node_kind == cindex.CursorKind.CLASS_DECL


def is_enum(node_kind: cindex.CursorKind) -> bool:
    return node_kind == cindex.CursorKind.ENUM_DECL


def get_annotation_tokens(node: cindex.Cursor) -> list[str]:
    annotations = []
    for child_node in node.get_children():
        if child_node.kind == cindex.CursorKind.ANNOTATE_ATTR:
            annotations.append(child_node.spelling or child_node.displayname or "")
    return annotations


def parse_rtti_id(id_string: str) -> uuid.UUID:
    if id_string == "Random":
        return USE_INTERNAL_ID
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


def parse_class(
    node: cindex.Cursor,
    fields: list[FieldInfo],
    base_types: list[cindex.Type],
    types: dict[uuid.UUID, ReflectedType],
    need_reflect: bool,
    public_only: bool,
):
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


def visit_external_rtti_declaration(
    node: cindex.Cursor,
    attributes: dict[str, str],
    types: dict[uuid.UUID, ReflectedType],
    project_dir: Path,
) -> bool:
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

        if reflected_type_declaration.location.file.name != node.location.file.name and "EmptyStruct" not in type_name:
            raise Exception(
                f"FE_RTTI_Reflect macro must be declared in the same file as the reflected type {get_qualified_name(reflected_type_declaration)}"
            )

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
    ref_type = ReflectedType(
        type_kind,
        need_reflect,
        reflected_type_id,
        namespace_name,
        type_name,
        node.location,
        attributes,
        bases,
        fields,
        project_dir,
    )
    types[ref_type.internal_id] = ref_type
    return True


def visit_class(node: cindex.Cursor, types: dict[uuid.UUID, ReflectedType], project_dir: Path):
    attributes = parse_attributes(node)
    if visit_external_rtti_declaration(node, attributes, types, project_dir):
        return

    # Find static Reflect method declaration. It is used as a marker to generate reflection code.
    # We need to generate the implementation for this method.
    reflected_type_id, need_reflect = None, False
    for child in node.get_children():
        if child.is_static_method() and child.spelling == "Reflect":
            annotations = parse_attributes(child)
            reflected_type_id, need_reflect = parse_rtti_attribute(annotations)

    if reflected_type_id:
        namespace_name = get_namespace(node)
        attributes = {}
        base_types = []
        fields = []
        parse_class(node, fields, base_types, types, need_reflect, False)

        bases = get_all_base_types(base_types, types)
        ref_type = ReflectedType(
            TypeKind.NORMAL,
            need_reflect,
            reflected_type_id,
            namespace_name,
            node.displayname,
            node.location,
            attributes,
            bases,
            fields,
            project_dir,
        )
        types[ref_type.internal_id] = ref_type


def visit(node: cindex.Cursor, types: dict[uuid.UUID, ReflectedType], project_dir: Path):
    if is_class(node.kind):
        if node.is_definition():
            visit_class(node, types, project_dir)
    for c in node.get_children():
        visit(c, types, project_dir)


def parse_file(
    file_path: str,
    include_dirs: list[str],
    defines: list[str],
    config: ParseConfig,
) -> dict[uuid.UUID, ReflectedType] | None:
    compiler_args = config.base_compiler_args + [f"-I{x}" for x in include_dirs] + [f"-D{x}" for x in defines]
    index = _get_index(config.llvm_dir)
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
        visit(tu.cursor, types, config.project_dir)
        return types

    return None

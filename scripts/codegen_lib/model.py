from __future__ import annotations

from clang import cindex
from enum import Enum, Flag
from pathlib import Path
import uuid

UUID_NAMESPACE = uuid.UUID("ba6b72ef-3286-4c71-9822-2519da4e156a")
USE_INTERNAL_ID = uuid.UUID("9ecf45e0-cba3-4e15-b613-ed66e3c4be5d")

REF_COUNTED_OBJECT_BASE_ID = uuid.UUID("b4fa5c63-69c0-4666-8a92-726f070d769b")


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
        return " | ".join(["Rtti::FieldFlags::k" + (x.name or "None").capitalize() for x in self])


class FieldInfo:
    def __init__(self, name: str, attributes: dict[str, str], flags: FieldFlags, type: "ReflectedType|None", enum_value=None, array_size=1) -> None:
        self.name = name
        self.attributes = attributes
        self.flags = flags
        self.type = type
        self.enum_value = enum_value
        self.array_size = array_size
        self.display_name = attributes.get("DisplayName", name)


class ConstructorInfo:
    def __init__(self, args: list["ReflectedType"]) -> None:
        self.args = args
        self.is_di_compatible = all(t.pointer_level == 1 and t.is_derived_from(REF_COUNTED_OBJECT_BASE_ID) for t in args)

    @staticmethod
    def create_invalid() -> ConstructorInfo:
        info = ConstructorInfo([])
        info.is_di_compatible = False
        return info


def get_internal_type_id(qualified_name: str) -> uuid.UUID:
    return uuid.uuid5(UUID_NAMESPACE, qualified_name)


def get_module_path(type_declaration_file_path: str, project_dir: Path) -> Path:
    file_path = Path(type_declaration_file_path).relative_to(project_dir)
    for part in file_path.parents:
        if part.parent.name == "Samples":
            return project_dir / part

        if part.name == "Shaders":
            return project_dir / "Modules/Graphics/Framework"

        if part.name == "Public" or part.name == "Private":
            return project_dir / part.parent

    raise Exception(f"Failed to find module path for {type_declaration_file_path}")


def get_header_path(type_declaration_file_path: str, module_path: Path) -> Path:
    relative_path = Path(type_declaration_file_path).relative_to(module_path)
    if relative_path.parts[0] == "Public" or relative_path.parts[0] == "Private" or relative_path.parts[0] == "Shaders":
        return Path(*relative_path.parts[1:])
    return relative_path


class ReflectedType:
    def __init__(
        self,
        kind: TypeKind,
        need_reflect: bool,
        id: uuid.UUID,
        namespace: str,
        name: str,
        location: cindex.SourceLocation,
        attributes: dict[str, str],
        bases: list[ReflectedType],
        fields: list[FieldInfo],
        constructors: list[ConstructorInfo],
        project_dir: Path,
    ):
        self.need_reflect = need_reflect
        self.name = name
        self.namespace = namespace
        self.qualified_name = f"{namespace}::{name}" if namespace else name

        self.internal_id = get_internal_type_id(self.qualified_name)
        self.id = self.internal_id if id == USE_INTERNAL_ID else id

        assert(location.file is not None)
        self.module_path = get_module_path(location.file.name, project_dir)
        self.header_path = get_header_path(location.file.name, self.module_path)
        if self.header_path.suffix != ".h":
            raise Exception(
                f"Reflected types must be declared in header files, but {self.qualified_name} is declared in {location}"
            )

        self.attributes = attributes
        self.bases = bases
        self.fields = fields
        self.constructors = constructors
        self.is_builtin = kind == TypeKind.BUILTIN
        self.is_enum = kind == TypeKind.ENUM
        self.is_external = self.is_builtin or self.is_enum or kind == TypeKind.EXTERNAL_CLASS
        self.pointer_level = 0
        self.is_di_compatible = len(constructors) == 1 and constructors[0].is_di_compatible or len(constructors) == 0
        if not self.is_derived_from(REF_COUNTED_OBJECT_BASE_ID):
            self.is_di_compatible = False

    def is_derived_from(self, base_id: uuid.UUID) -> bool:
        return any(t.id == base_id for t in self.bases)

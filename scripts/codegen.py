from clang import cindex
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
import json
import sys

cindex.Config.set_library_file("./libclang.dll")
index = cindex.Index.create()

base_compiler_args = [
    '-x', 'c++',
    '-std=c++17',
]

base_compiler_args.append(f"-DEASTL_USER_CONFIG_HEADER=\"{Path("../FerrumCore/Private/FeCore/Base/EASTLConfig.h").absolute().as_posix()}\"")


def get_fully_qualified_name(cursor):
    if cursor is None or cursor.kind == cindex.CursorKind.TRANSLATION_UNIT:
        return ''

    parent_name = get_fully_qualified_name(cursor.semantic_parent)

    if parent_name:
        return parent_name + '::' + cursor.spelling
    else:
        return cursor.spelling


def get_annotation_tokens(node):
    """Look for annotate attributes among children and return their text."""
    annots = []
    for c in node.get_children():
        if c.kind == cindex.CursorKind.ANNOTATE_ATTR:
            annots.append(c.spelling or c.displayname or "")
    return annots


def visit(node):
    if node.kind == cindex.CursorKind.STRUCT_DECL:
        if node.is_definition():
            struct_annots = get_annotation_tokens(node)
            if "fe_reflect" in struct_annots:
                print(f"Struct: {get_fully_qualified_name(node)} (loc: {node.location.file}:{node.location.line})")
                for child in node.get_children():
                    if child.kind == cindex.CursorKind.FIELD_DECL:
                        f_name = child.spelling
                        f_type = child.type.spelling
                        print(f"  Field: {f_name} : {f_type}")
                        ann = get_annotation_tokens(child)
                        if ann:
                            print(f"    annotations: {ann}")
    for c in node.get_children():
        visit(c)


def parse_file(file_path: str, include_dirs: list[str], defines: list[str]):
    compiler_args = base_compiler_args + [f"-I{x}" for x in include_dirs] + [f"-D{x}" for x in defines]
    tu = index.parse(file_path, args=compiler_args)

    if not tu:
        print("Failed to parse:", file_path)
        return False

    # if tu.diagnostics:
    #     print("Translation unit diagnostics:")
    #     for d in tu.diagnostics:
    #         print(f"  {d.location}: {d}")
    #     if any(d.severity in [cindex.Diagnostic.Error, cindex.Diagnostic.Fatal] for d in tu.diagnostics):
    #         return False

    visit(tu.cursor)
    return True


def should_parse_file(file_path: str) -> bool:
    return "ThirdParty" not in file_path


if __name__ == "__main__":
    with open("../cmake-build/windows-codegen/compile_commands.json") as f:
        data = json.load(f)
        files = [d for d in data if should_parse_file(d["file"])]
        total_file_count = len(files)

        with ProcessPoolExecutor(max_workers=16) as executor:
            futures = []
            with tqdm(total=total_file_count, desc="Preparing") as pbar:
                for d in files:
                    file_path = d["file"]
                    command = d["command"]
                    command_tokens = command.split(" ")
                    include_dirs = [p[2:] for p in command_tokens if p.startswith("-I")]
                    include_dirs += [p[11:] for p in command_tokens if p.startswith("-external:I")]
                    defines = [p[2:] for p in command_tokens if p.startswith("-D") or p.startswith("/D")]
                    futures.append(executor.submit(parse_file, file_path, include_dirs, defines))
                    pbar.update(1)

            with tqdm(total=total_file_count, desc="Processing files") as pbar:
                for future in as_completed(futures):
                    pbar.update(1)

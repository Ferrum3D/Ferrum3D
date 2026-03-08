from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
from tqdm import tqdm
import json
import os
import pickle

from jinja2 import Environment, FileSystemLoader, select_autoescape

from codegen_lib.clang_parser import ParseConfig, parse_file
from codegen_lib.generators.reflection import ReflectionGenerator
from codegen_lib.gpudb_parser import generate_gpudb
import codegen_lib.compile_commands_filter as ccf

script_path = os.path.abspath(__file__)
script_directory = os.path.dirname(script_path)
os.chdir(script_directory)

PROJECT_DIR = Path.cwd().parent.absolute()
LLVM_DIR = PROJECT_DIR / "ThirdParty/llvm"

base_compiler_args = [
    '-x', 'c++-header',
    '-std=c++17',
]

base_compiler_args.append(f"-DEASTL_USER_CONFIG_HEADER=\"{Path("../FerrumCore/Private/FeCore/Base/EASTLConfig.h").absolute().as_posix()}\"")

def should_parse_file(file_path: str) -> bool:
    # return "Downsample.cpp" in file_path
    return "ThirdParty" not in file_path and file_path.endswith(".cpp") and not file_path.endswith(".gen.cpp")


NUM_WORKERS = 8
MODEL_CACHE_PATH = ".reflection_model"


def _load_model_cache(path: str) -> dict[str, dict]:
    p = Path(path)
    if p.exists():
        try:
            with p.open("rb") as f:
                return pickle.load(f)
        except Exception:
            pass
    return {}


def _save_model_cache(path: str, model: dict[str, dict]) -> None:
    with Path(path).open("wb") as f:
        pickle.dump(model, f)


def main():
    templates_dir=Path("./templates")
    clang_format_path=LLVM_DIR / "clang-format.exe"
    clang_format_style=Path("../.clang-format")

    env = Environment(loader=FileSystemLoader(str(templates_dir)), autoescape=select_autoescape())

    generate_gpudb(
        env=env,
        templates_dir=templates_dir,
        project_root=PROJECT_DIR,
        clang_format_path=clang_format_path,
        clang_format_style=clang_format_style
    )

    compile_commands_path = Path("../cmake-build/windows-codegen/compile_commands.json")
    with open(compile_commands_path, "r") as f:
        commands = json.load(f)

    filtered_commands = [d for d in commands if should_parse_file(d["file"])]

    cache = ccf.Cache.load(".reflection_cache")
    changed = ccf.filter_changed(filtered_commands, cache, PROJECT_DIR, exclude_dirs=["ThirdParty"])
    cache.save()

    total_file_count = len(changed)

    if total_file_count == 0 and cache.is_valid():
        print("Nothing to do. All headers are up to date.")

    # When the CCF cache is invalid (a file was deleted), regenerate the model
    # from scratch; otherwise load the existing per-file model and update only
    # the entries for changed files.
    per_file_model: dict[str, dict] = {} if not cache.is_valid() else _load_model_cache(MODEL_CACHE_PATH)

    parse_config = ParseConfig(project_dir=PROJECT_DIR, llvm_dir=LLVM_DIR, base_compiler_args=base_compiler_args)
    with ProcessPoolExecutor(max_workers=NUM_WORKERS) as executor:
        # Map future -> file_path so results can be stored per file.
        futures: dict = {}
        with tqdm(total=total_file_count, desc="Preparing") as pbar:
            for d in changed:
                file_path = d["file"]
                command = d["command"]
                command_tokens = command.split(" ")
                include_dirs = [p[2:] for p in command_tokens if p.startswith("-I")]
                include_dirs += [p[11:] for p in command_tokens if p.startswith("-external:I")]
                defines = [p[2:] for p in command_tokens if p.startswith("-D") or p.startswith("/D")]
                if NUM_WORKERS > 1:
                    future = executor.submit(parse_file, file_path, include_dirs, defines, parse_config)
                    futures[future] = file_path
                else:
                    result = parse_file(file_path, include_dirs, defines, parse_config)
                    per_file_model[file_path] = result or {}
                pbar.update(1)

        if NUM_WORKERS > 1:
            with tqdm(total=total_file_count, desc="Processing files") as pbar:
                for future in as_completed(futures):
                    pbar.update(1)
                    p = futures[future]
                    per_file_model[p] = future.result() or {}

    _save_model_cache(MODEL_CACHE_PATH, per_file_model)

    # Merge per-file results into a single flat dict for codegen.
    reflected_types_dict = {}
    for file_types in per_file_model.values():
        reflected_types_dict.update(file_types)

    reflected_types = reflected_types_dict.values()
    for t in reflected_types:
        same_id = [x for x in reflected_types if x.id == t.id]
        ok = len(same_id) == 1 and same_id[0] is t
        if not ok:
            raise Exception(f'These types have the same id: {', '.join(x.qualified_name for x in same_id)}')

    generator = ReflectionGenerator(
        env=env,
        templates_dir=templates_dir,
        clang_format_path=clang_format_path,
        clang_format_style=clang_format_style,
    )
    generator.generate(list(reflected_types))


if __name__ == "__main__":
    main()

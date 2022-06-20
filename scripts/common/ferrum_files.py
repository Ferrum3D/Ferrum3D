import glob
import os
from functools import reduce


def get_ferrum_files(extensions=None, engine_dirs=None):
    if engine_dirs is None:
        engine_dirs = ["FeAssetCompiler", "FeCore", "Modules", "FeTestProject", "Samples", "cmake"]
    if extensions is None:
        extensions = ["cpp", "h"]

    filenames = [glob.glob(".." + os.sep + d + '/**/*.' + e, recursive=True) for d in engine_dirs for e in extensions]
    filenames = reduce(list.__add__, filenames)
    return filenames

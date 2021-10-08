# Ferrum3D Engine
**Ferrum3D** is a 3D game engine. Ferrum3D will be multi-platform,
data-oriented, data-driven, and modular in the future.

## Current development status
For now, only the core is working. The FrameGraph and C# bindings are in development.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3** (tested with Python 3.9.5)

### Windows
Currently, the engine has been tested on Windows only. For building, you will need
Visual Studio 2019 with *Game Development for C++* installed.

### Downloading sources
This repository uses git submodules for dependencies. To clone the sources run this:
```shell
git clone https://github.com/Ferrum3D/Ferrum3D.git --recursive
cd Ferrum3D
```
If you cloned the repository without `--recursive`, run this:
```shell
git submodule update --init --recursive
```

### Build steps
After you cloned the repository, run the setup script:
```shell
python setup.py
```

If you're using Visual Studio, you can now open the solution in `BuildRelease/Ferrum3D.sln`
and build it.

When C++ projects are compiled, you can build the C# bindings. Open another
solution in `Managed/Ferrum3D/Ferrum3D.sln`. It depends on DLLs from `BuildRelease/`, so
make sure it has been built.

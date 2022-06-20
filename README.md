# Ferrum3D Engine
**Ferrum3D** is a data-oriented, data-driven, and modular 3D game engine.

## Current development status
For now, only the core and its C# bindings are working.
The higher level renderer, i.e. FrameGraph is in development yet.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3** (tested with Python 3.10)

### Windows
Currently, the engine has been tested on Windows only. For building, you will need
Visual Studio build tools installed.

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

### Build and run
After you cloned the repository, run the setup script:
```shell
./setup
```

It will set up all projects in all configurations, build them and run the triangle sample.
To use the engine, you will need to reference the managed assemblies in your project.

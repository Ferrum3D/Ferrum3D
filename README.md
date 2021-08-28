# Ferrum3D Engine
**Ferrum3D** is a 3D game engine under active development.
It is intended to be multi-platform, data-oriented, data-driven and modular.

## Current development status
Currently working modules are the engine's core and graphics hardware abstraction.
The HAL allowed to write a simple triangle application using vertex and index buffers.
The color of the triangle is stored in a constant buffer accessed through descriptors.

Next steps to do with HAL:
1. Add SPIR-V reflection to use HLSL semantics instead of `[[vk::location()]]`
1. Add support for different queues for rendering and presentation
1. Add samplers and textures with mipmaps
1. Refactor the code and write documentation

The next step will be to write a high-level renderer using FrameGraph.
This renderer must abstract away all synchronization and resource management.
It will allow custom render passes which are classes that define
the way they access resources and record command buffers abstracted with `IRenderContext`.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3** (tested with Python 3.9.5)

### Windows
Currently the engine was tested on windows only. For building you will need
Visual Studio 2019 with *Game Development for C++* installed.

### Downloading sources
This repository uses git submodules for dependencies.
```shell
git clone https://github.com/Ferrum3D/Ferrum3D.git --recursive
```
If you cloned the repository without `--recursive`, run this:
```shell
git submodule update --init --recursive
```

### Build steps
After you cloned the repository, run this commands:
```shell
cd Ferrum3D
cmake -B Build -S .
```

If you use Visual Studio, you can now open the solution in `Build/Ferrum3D.sln`
and build it.

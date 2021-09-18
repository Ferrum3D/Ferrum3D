# Ferrum3D Engine
**Ferrum3D** is a 3D game engine. Ferrum3D will be multi-platform,
data-oriented, data-driven, and modular in the future.

## Current development status
Currently, working modules are the engine's core and graphics hardware abstraction.
The HAL allowed writing a simple triangle application using vertex and index buffers.
A constant buffer stores color of the triangle accessed through descriptors.

Next steps to do with HAL:
1. Add support for different queues for rendering and presentation
1. Add samplers and textures with mipmaps
1. Refactor the code and write documentation

The engine will also have a higher-level renderer with FrameGraph.
This renderer must abstract away all synchronization and resource management.
It will allow the user to write custom render passes. A render pass is a class
that defines the way they access resources and record command buffers abstracted
with `IRenderContext`.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3** (tested with Python 3.9.5)

### Windows
Currently, the engine has been tested on windows only. For building, you will need
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
After you cloned the repository, run these commands:
```shell
cd Ferrum3D
cmake -B Build -S .
```

If you're using Visual Studio, you can now open the solution in `Build/Ferrum3D.sln`
and build it.

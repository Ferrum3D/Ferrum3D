# Ferrum3D Engine
**Ferrum3D** is a 3D game engine under active development.
It is intended to be multi-platform, data-oriented, data-driven and modular.

However, currently only the engine's core is tested since the development started only about
two months ago.

The next module to write is **FeGPU** - a graphics HAL (Hardware Abstraction Layer) on top of
Vulkan API and draw a triangle with vertex and constant buffers. This will allow to develop
a higher-level renderer with engine's frame graph implementation.

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

# Ferrum3D Engine
[![GitHub license](https://img.shields.io/github/license/Ferrum3D/Ferrum3D?style=for-the-badge)](https://github.com/Ferrum3D/Ferrum3D/blob/main/LICENSE)

**Ferrum3D** is a data-oriented, data-driven, and modular 3D graphics engine and game framework with C# support.
It can be used for 2D/3D games and other real-time graphics applications.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3**

### Windows
Currently, the engine has been tested on Windows only. For building, you will need
Visual Studio build tools with "Desktop development with C++" and ".NET desktop development" installed.

### Downloading sources
This repository uses git submodules for dependencies. To clone the sources run this command:
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

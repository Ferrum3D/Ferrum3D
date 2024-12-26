# Ferrum3D Engine
[![GitHub license](https://img.shields.io/github/license/Ferrum3D/Ferrum3D?style=for-the-badge)](https://github.com/Ferrum3D/Ferrum3D/blob/main/LICENSE)

**Ferrum3D** is a data-oriented modular 3D graphics engine and game framework.
It can be used for 2D/3D games and other real-time graphics applications.

## Modular architecture
The entire engine consists of different modules. The core is very lightweight and
doesn't even have any graphics functionality. The graphics engine is a dynamic module
that can be loaded by the user app.

All the modules have the same structure, e.g. if the module is called *MyModule*:
- `MyModule`
  - `MyModule/Public` - Public API of the module.
  - `MyModule/Private` - Private headers and sources.
  - `Tests` - Unit tests.
  - `CMakeLists.txt`.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Vulkan SDK**

### Windows
Currently, the engine has been tested on Windows only. To build Ferrum3D you will need
Visual Studio build tools with *Desktop development with C++* installed.

![Screenshot 1](images/screen1.png)

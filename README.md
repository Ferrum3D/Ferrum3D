# Ferrum3D Engine
[![GitHub license](https://img.shields.io/github/license/Ferrum3D/Ferrum3D?style=for-the-badge)](https://github.com/Ferrum3D/Ferrum3D/blob/main/LICENSE)

**Ferrum3D** is a data-oriented, data-driven, and modular 3D graphics engine and game framework.
It can be used for 2D/3D games and other real-time graphics applications.

## Modular architecture
The entire engine consists of different modules. The core is very lightweight and
doesn't even have any graphics functionality. The graphics engine is a dynamic module
that can be loaded by the user app.

All the modules have the same structure, e.g. if the module is called *MyModule*:
- `MyModule`
  - `MyModule` - The main module code.
  - `Tests` - Unit tests written with Google Test framework.
  - `CMakeLists.txt` - `add_library(MyModule SHARED ...)`, `add_subdirectory(Bindings)`, `add_subdirectory(Tests)`.

## Engine structure
- `FerrumAssetCompiler` - A command line utility to compile assets to the format usable by Ferrum3D.
- `FerrumCore` - The engine core. All the modules connect to it.
  - `Assets` - Classes for streaming and managing assets.
  - `Base` - Basic platform- and compiler-specific `#define`s, core functions etc. Used everywhere in the engine.
  - `Components` - Core ECS components.
  - `Console` - Console loggers, functions for windows-specific UTF-8 initialization, colored text printing.
  - `Containers` - Custom data structures like `ArraySlice<T>`, `SparseSet<T>`, aliases for third party containers.
  - `ECS` - Data oriented Entity Component System implementation.
  - `EventBus` - Event bus and event handler implementation and basic events.
  - `IO` - Input/Output streams, file system utilities, etc.
  - `Jobs` - Job scheduler and basic jobs implementation.
  - `Math` - Math library: vectors, matrices, etc.
  - `Memory` - Memory management, allocators and reference counting.
  - `Modules` - Global environment for communication between different modules, DLL loading utility.
  - `Parallel` - Threads, fibers, synchronization primitives.
  - `RTTI` - Lightweight RTTI system and fast `fe_dynamic_cast` implementation.
  - `SIMD` - Abstraction on top of SIMD instructions and vectors.
  - `Strings` - String implementation, Unicode support library, formatting library.
  - `Systems` - Core ECS systems.
  - `Time` - `DateTime` and `TimeSpan` implementation.
  - `Utils` - Various utilities like UUID and `Result<T>`.
- `FerrumModules` - Optional modules that can be connected to the user app.
  - `OsmiumGPU` - Osmium's hardware abstraction layer on top of Vulkan API.
  - `OsmiumAssets` - `IAssetLoader` and `AssetStorage` implementations for textures, 3D meshes, shaders, etc.
- `Samples` - Sample projects.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Vulkan SDK**

### Windows
Currently, the engine has been tested on Windows only. To build Ferrum3D you will need
Visual Studio build tools with *Desktop development with C++* installed.

![Screenshot 1](images/screen1.png)

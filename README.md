# Ferrum3D Engine
[![GitHub license](https://img.shields.io/github/license/Ferrum3D/Ferrum3D?style=for-the-badge)](https://github.com/Ferrum3D/Ferrum3D/blob/main/LICENSE)

**Ferrum3D** is a data-oriented, data-driven, and modular *3D graphics engine and game framework* with C# support.
It can be used for 2D/3D games and other real-time graphics applications.

## Modular architecture
The entire engine consists of different modules. The core is very lightweight and
doesn't even have any graphics functionality. The graphics engine is called
**Osmium** and is a dynamic module that can be connected to the user app.

All the modules have the same structure, e.g. if the module is called *MyModule*:
- `MyModule`
  - `MyModule` - The main module code.
  - `Bindings` - `FE_DLL_EXPORT extern "C"` functions that are called from the C# side.
  - `Tests` - Unit tests written with Google Test framework.
  - `CMakeLists.txt` - `add_library(MyModule SHARED ...)`, `add_subdirectory(Bindings)`, `add_subdirectory(Tests)`.

## Engine structure
- `FerrumAssetCompiler` - A command line utility to compress assets for release mode. `TODO`
- `FerrumCore` - The engine core. All the modules connect to it.
  - `Allocators` - A collection of allocators for different purposes.
  - `Assets` - Classes for streaming and managing app assets.
  - `Base` - Basic platform- and compiler-specific `#define`s, enum operators etc. Used everywhere in the engine.
  - `Console` - Console loggers, functions for windows-specific UTF-8 initialization, colored text printing.
  - `Containers` - Custom data structures: `List<T>`, `ArraySlice<T>`, `SparseSet<T>`, etc.
  - `ECS` - Data oriented Entity Component System implementation.
  - `EventBus` - Event bus and event handler implementation and basic events.
  - `Framework` - Application and module frameworks: classes for managing dependencies of engine modules.
  - `IO` - Input/Output streams, filesystem utilities, etc.
  - `Jobs` - Job scheduler and basic jobs implementation.
  - `Math` - Math library: vectors, matrices, etc. Optimized with SIMD instructions.
  - `Memory` - Basic allocators and reference counting system.
  - `Modules` - Global environment for communication between different modules, DLL loading utility.
  - `Parallel` - Synchronization primitives.
  - `RTTI` - Lightweight RTTI system and fast `fe_dynamic_cast` implementation.
  - `SIMD` - Abstraction on top of SSE instructions and `__m128`.
  - `Strings` - Custom String implementation with "small-string" optimization (up to 24 characters) and string formatting library.
  - `Time` - `DateTime` and `TimeSpan` implementation.
  - `Utils` - Various utilities: UUID, `Result<T>`, etc.
- `FerrumModules` - Optional modules that can be connected to the user app.
  - `OsmiumGPU` - Osmium's hardware abstraction layer on top of Vulkan API in C++.
  - `OsmiumAssets` - `IAssetLoader` and `AssetStorage` implementations for textures, 3D meshes, shaders, etc.
- `Managed` - C# bindings and high-level Osmium framegraph.
  - `Ferrum.Core` - FeCore bindings.
  - `Ferrum.Osmium` - Osmium bindings.
  - `Ferrum.Samples` - Sample projects in C#.
- `Samples` - Sample projects in C++.

## Getting the sources and building
If you want to build the engine, run samples and tests you will need:
 - **CMake v1.17.0** minimum
 - **Python3**

### Windows
Currently, the engine has been tested on Windows only. For building, you will need
Visual Studio build tools with *Desktop development with C++* and *.NET desktop development* installed.

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

It will set up all projects in all configurations, build them and run the *triangle* sample.
To use the engine, you will need to reference the managed assemblies in your project.

![Screenshot 1](images/screen1.png)

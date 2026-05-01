# Repository Guidelines

Scope: this file applies to the whole repository.

## Directory Structure

- `FerrumCore/` contains the core runtime library. Public API headers live under `FerrumCore/Public`, implementation details under `FerrumCore/Private`, and tests under `FerrumCore/Tests`.
- `Modules/` contains engine modules. Modules follow the same `Public` and `Private` split used by `FerrumCore`.
- `Modules/Graphics/Core/` is the low-level graphics module. Backend-independent public interfaces live in `Public/Graphics/Core`, shared private implementation lives in `Private/Graphics/Core/Common`, and Vulkan-specific implementation lives in `Private/Graphics/Core/Vulkan`.
- `Samples/` contains sample applications such as `FrameGraph` and `Renderer`.
- `Tools/` contains standalone tools such as `AssetBuilder` and `TextureCompressor`.
- `ThirdParty/` contains vendored dependencies. Avoid style-only churn or broad edits there unless the task is explicitly about that dependency.
- `cmake/` contains project CMake helpers. `cmake-build/` contains generated build trees and should not be edited as source.
- Root files such as `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, and `.editorconfig` define build and formatting behavior for the repository.

When adding C++ files, add them to the nearest `CMakeLists.txt` source list, usually `PUBLIC_HEADERS`, `COMMON_SOURCES`, backend-specific source groups such as `VULKAN_SOURCES`, or platform-specific groups such as `WINDOWS_SOURCES`.

## Code Style

- Use C++17. The root `CMakeLists.txt` sets `CMAKE_CXX_STANDARD 17`, and `.clang-format` uses `Standard: c++17`.
- Follow `.clang-format` for C++ formatting:
  - 4-space indentation, no tabs.
  - 130-column limit.
  - Allman-style braces for namespaces, classes, structs, functions, enums, control statements, and extern blocks.
  - `else`, `catch`, and `while` after `do` appear on their own lines with braces.
  - Pointer and reference stars bind to the type side, for example `Buffer*` and `BufferInstance*&`.
  - Keep short empty functions/lambdas compact only where the formatter allows it.
  - Preserve include blocks and sort includes case-sensitively when changing include lists.
- Include order in implementation files follows project headers grouped at the top, as in `Buffer.cpp`. Prefer angle-bracket project includes such as `#include <Graphics/Core/Vulkan/Buffer.h>`.
- Use namespaces in the `FE::...` hierarchy and close nontrivial namespaces with comments, for example `} // namespace FE::Graphics::Vulkan`.
- Keep anonymous helper functions in an unnamed namespace inside the implementation file when they are local to that translation unit.
- Prefer early returns for invalid or empty states, as in `Buffer::UpdateDebugNames()` and `Buffer::DecommitMemory()`.
- Use existing project diagnostics and helpers instead of ad hoc checks: `FE_Assert`, `FE_AssertDebug`, `FE_DebugBreak`, `VerifyVk`, `FE_PROFILER_ZONE`, `Rtti::AssertCast`, `NativeCast`, and `ImplCast`.
- Keep comments sparse. Use comments for namespace endings, complex intent, or non-obvious behavior rather than restating the code.

## Naming Conventions

- Types use PascalCase: `Buffer`, `ResourcePool`, `BufferInstance`.
- Functions and methods use PascalCase: `Create`, `Map`, `Unmap`, `CommitInternal`, `UpdateDebugNames`.
- Data members use an `m_` prefix with lower camel case: `m_device`, `m_name`, `m_instance`, `m_vmaAllocation`.
- Constants and enum values commonly use a `k` prefix: `kBuffer`, `kUndefined`, `kHostRandomAccess`.
- Boolean variables should read as predicates where practical, for example `isTexelBuffer`.
- Local variables use lower camel case: `bufferCI`, `allocationCI`, `bufferInstance`.
- Macros use upper snake case and the project `FE_` prefix where applicable: `FE_RTTI`, `FE_DECLARE_VULKAN_OBJECT_POOL`, `FE_ENABLE_NATIVE_CAST`.
- CMake variables in module lists use upper snake case, for example `PUBLIC_HEADERS`, `COMMON_SOURCES`, and `VULKAN_SOURCES`.

## Class and API Patterns

- Public module headers should expose stable engine-facing interfaces. Backend-specific implementation headers, such as Vulkan resources, belong under `Private`.
- Resource-like Vulkan classes commonly:
  - Derive from the matching common interface.
  - Declare RTTI with `FE_RTTI`.
  - Provide a static `Create` factory using the project allocation or pool pattern.
  - Keep constructors private when creation must go through the factory.
  - Override virtual API methods with `override`.
  - Mark leaf classes with `final`.
  - Use `[[nodiscard]]` on simple getters where ignoring the result is likely a bug.
- Keep Vulkan object ownership explicit. Creation and destruction should flow through the existing device, pool, VMA, and debug-name helpers rather than raw unmanaged calls scattered through unrelated code.

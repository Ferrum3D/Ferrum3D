#include <Core/Memory/Memory.h>
#include <cstdlib>

// Keep the upstream implementation unchanged while routing its state allocations through the engine.
#define ALLOC(type) static_cast<type*>(FE::Memory::DefaultAllocate(sizeof(type)))
#define free FE::Memory::DefaultFree
#if FE_COMPILER_MSVC
#    pragma warning(push, 0)
#endif
#include <ArHosekSkyModel.cc>
#if FE_COMPILER_MSVC
#    pragma warning(pop)
#endif
#undef free
#undef ALLOC

#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Base/Hash.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <festd/base.h>
#include <festd/span.h>
#include <intrin.h>
#include <mutex>
#include <tracy/Tracy.hpp>

#define FE_PROFILER_ZONE() ZoneScoped
#define FE_PROFILER_ZONE_NAMED(name) ZoneScopedN(name)

#define FE_PROFILER_ZONE_TEXT(...)                                                                                               \
    ZoneScoped;                                                                                                                  \
    ZoneTextF(__VA_ARGS__)


namespace FE
{
    namespace Memory
    {
        template<class T>
        void Copy(festd::span<const T> source, festd::span<T> destination)
        {
            FE_CoreAssert(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        template<class T>
        void Copy(festd::span<T> source, festd::span<T> destination)
        {
            FE_CoreAssert(source.size() == destination.size(), "Size mismatch");
            eastl::copy(source.begin(), source.end(), destination.begin());
        }


        inline void Zero(void* memory, const size_t size)
        {
            memset(memory, 0, size);
        }


        inline festd::span<const std::byte> MakeByteSpan(const char* str, uint32_t length = 0)
        {
            return { reinterpret_cast<const std::byte*>(str), length ? length : static_cast<uint32_t>(__builtin_strlen(str)) };
        }
    } // namespace Memory


//! @brief Typed alloca() wrapper.
#define FE_StackAlloc(type, arraySize) static_cast<type*>(alloca(sizeof(type) * (arraySize)))
} // namespace FE

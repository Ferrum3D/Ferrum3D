#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Base/StackTrace.h>
#include <FeCore/Base/StackTracePrivate.h>
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <festd/unordered_map.h>

#if FE_DEVELOPMENT
#    include <DbgHelp.h>
#    include <xxhash.h>
#    pragma comment(lib, "dbghelp.lib")

namespace FE::Trace
{
    namespace
    {
        //
        // We cannot use the default allocator here since the primary usage of stack traces is to
        // track down memory leaks. So we might end up with a recursive allocation.
        //
        // Instead, we allocate virtual memory directly to avoid this issue.
        // The purpose of this proxy allocator is to align allocation size to the page size,
        // so we don't trigger assertions in our AllocateVirtual() function.
        //
        struct StackCacheAllocator final : public std::pmr::memory_resource
        {
        private:
            void* do_allocate(const size_t byteSize, const size_t byteAlignment) override
            {
                const size_t granularity = Memory::GetPlatformSpec().m_granularity;
                return Memory::AllocateVirtual(AlignUp(byteSize, granularity), AlignUp(byteAlignment, granularity));
            }

            void do_deallocate(void* ptr, const size_t byteSize, const size_t byteAlignment) override
            {
                FE_Unused(byteAlignment);
                const size_t granularity = Memory::GetPlatformSpec().m_granularity;
                Memory::FreeVirtual(ptr, AlignUp(byteSize, granularity));
            }

            [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        void** AllocateFrames(const uint32_t frameCount)
        {
            // We want stack frames to always be available, so we can use linear allocator
            std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
            void* memory = allocator->allocate(sizeof(void*) * frameCount);
            return static_cast<void**>(memory);
        }


        struct StackTraceStorage final
        {
            Threading::SpinLock m_lock;
            StackCacheAllocator m_cacheAllocator;
            festd::pmr::segmented_unordered_dense_map<uint64_t, CallStack> m_callstackMap{ &m_cacheAllocator };
            SegmentedVector<void**, 64 * 1024> m_callstacks{ &m_cacheAllocator };
        };

        StackTraceStorage* GStorage = nullptr;
    } // namespace


    void Internal::InitStackTrace(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GStorage == nullptr, "Stack trace already initialized");
        GStorage = Memory::New<StackTraceStorage>(allocator);

        SymSetOptions(SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_ALLOW_ZERO_ADDRESS | SYMOPT_AUTO_PUBLICS | SYMOPT_DEBUG
                      | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_NO_PROMPTS);

        SymInitialize(GetCurrentProcess(), nullptr, TRUE);

        // Due to the way we allocate memory for this map, it's better to reserve and reduce the number of reallocations
        // as much as possible.
        GStorage->m_callstackMap.reserve(64 * 1024);
    }


    void Internal::ShutdownStackTrace()
    {
        FE_CoreAssert(GStorage != nullptr, "Stack trace not initialized");
        GStorage->~StackTraceStorage();
        GStorage = nullptr;

        FE_Verify(SymCleanup(GetCurrentProcess()));
    }


    SymbolInfo SymbolInfo::Resolve(void* ptr)
    {
        SymbolInfo symbolInfo;
        symbolInfo.m_address = reinterpret_cast<uintptr_t>(ptr);

        auto* winSymbolInfo = FE_StackAlloc(SYMBOL_INFO, sizeof(SYMBOL_INFO) + sizeof(symbolInfo.m_symbolName));
        winSymbolInfo->MaxNameLen = sizeof(symbolInfo.m_symbolName) - 1;
        winSymbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
        if (SymFromAddr(GetCurrentProcess(), reinterpret_cast<ULONGLONG>(ptr), nullptr, winSymbolInfo))
        {
            memcpy(symbolInfo.m_symbolName, winSymbolInfo->Name, winSymbolInfo->NameLen);
            symbolInfo.m_symbolName[winSymbolInfo->NameLen] = '\0';
        }

        IMAGEHLP_MODULE64 moduleInfo;
        moduleInfo.SizeOfStruct = sizeof(moduleInfo);
        if (SymGetModuleInfo64(GetCurrentProcess(), reinterpret_cast<ULONGLONG>(ptr), &moduleInfo))
            memcpy(symbolInfo.m_moduleName, moduleInfo.ModuleName, sizeof(moduleInfo.ModuleName));

        IMAGEHLP_LINE64 lineInfo;
        lineInfo.SizeOfStruct = sizeof(lineInfo);

        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(GetCurrentProcess(), reinterpret_cast<ULONGLONG>(ptr), &lineDisplacement, &lineInfo))
        {
            symbolInfo.m_lineNumber = lineInfo.LineNumber;

            const size_t fileNameLength = Math::Min(strlen(lineInfo.FileName), sizeof(symbolInfo.m_fileName) - 1);
            memcpy(symbolInfo.m_fileName, lineInfo.FileName, fileNameLength);
            symbolInfo.m_fileName[fileNameLength] = '\0';
        }

        return symbolInfo;
    }


    CallStack CallStack::Capture(const uint32_t maxFrames, const uint32_t skipFrames)
    {
        void** tempFrames = FE_StackAlloc(void*, maxFrames);

        const WORD frameCount = RtlCaptureStackBackTrace(skipFrames, maxFrames, tempFrames, nullptr);
        const uint64_t hash = XXH3_64bits(tempFrames, frameCount * sizeof(void*));

        {
            std::lock_guard lock{ GStorage->m_lock };

            const auto it = GStorage->m_callstackMap.find(hash);
            if (it != GStorage->m_callstackMap.end())
                return it->second;

            void** frames = AllocateFrames(frameCount + 1);
            memcpy(frames, tempFrames, frameCount * sizeof(void*));
            frames[frameCount] = nullptr;

            CallStack callStack;
            callStack.m_value = GStorage->m_callstacks.size();
            GStorage->m_callstacks.push_back(frames);
            GStorage->m_callstackMap[hash] = callStack;
            return callStack;
        }
    }


    void** CallStack::GetFrames() const
    {
        FE_Assert(m_value < GStorage->m_callstacks.size());
        return GStorage->m_callstacks[m_value];
    }
} // namespace FE::Trace

#else

namespace FE::Trace
{
    void Internal::InitStackTrace(std::pmr::memory_resource*) {}
    void Internal::ShutdownStackTrace() {}

    CallStack CallStack::Capture(const uint32_t, const uint32_t)
    {
        return CallStack();
    }
} // namespace FE::Trace

#endif

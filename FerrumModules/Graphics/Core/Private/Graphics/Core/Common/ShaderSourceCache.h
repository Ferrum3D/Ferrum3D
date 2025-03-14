#pragma once
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/ShaderStage.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct ShaderSourceCache;

    struct ShaderSourceFile final : public Memory::RefCountedObjectBase
    {
        ~ShaderSourceFile() override;

        [[nodiscard]] ShaderStage GetStage() const;
        [[nodiscard]] festd::string_view GetSource() const;

    private:
        friend ShaderSourceCache;

        ShaderSourceCache* m_sourceCache = nullptr;
        std::pmr::memory_resource* m_sourceAllocator = nullptr;
        char* m_source = nullptr;
        uint32_t m_sourceSize = 0;
        ShaderStage m_stage = ShaderStage::kUndefined;
    };


    struct ShaderSourceCache final
        : public Memory::RefCountedObjectBase
        , public IO::IAsyncReadCallback
    {
        FE_RTTI_Class(ShaderSourceCache, "FE08F0A8-40B4-4C17-B152-8220DC1BF5F6");

        ShaderSourceCache(IO::IAsyncStreamIO* asyncIO, Logger* logger);

        [[nodiscard]] bool IsLoading() const;

        festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> GetSource(Env::Name path);

    private:
        void AsyncIOCallback(const IO::AsyncReadResult& result) override;

        Memory::Pool<ShaderSourceFile> m_filePool;
        festd::segmented_unordered_dense_map<Env::Name, Rc<ShaderSourceFile>> m_filesMap;
        IO::IAsyncStreamIO* m_asyncIO;
        Logger* m_logger;
        Threading::SpinLock m_lock;
        std::atomic<uint32_t> m_loadingTasksCount;
    };


    inline ShaderStage ShaderSourceFile::GetStage() const
    {
        FE_Assert(!m_sourceCache->IsLoading());
        return m_stage;
    }


    inline festd::string_view ShaderSourceFile::GetSource() const
    {
        FE_Assert(!m_sourceCache->IsLoading());
        return m_source;
    }


    inline bool ShaderSourceCache::IsLoading() const
    {
        return m_loadingTasksCount.load(std::memory_order_acquire) > 0;
    }
} // namespace FE::Graphics::Core

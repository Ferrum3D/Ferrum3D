#pragma once
#include <Core/IO/Async.h>
#include <Core/Jobs/Base.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/SharedSpinLock.h>
#include <Graphics/Core/ShaderStage.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct ShaderSourceCache;

    struct ShaderSourceFile final : public Memory::RefCountedObjectBase
    {
        ShaderSourceFile(Memory::Pool<ShaderSourceFile>& pool)
            : m_pool(pool)
        {
        }

        ~ShaderSourceFile() override;

        [[nodiscard]] ShaderStage GetStage() const;
        [[nodiscard]] festd::string_view GetSource() const;

    private:
        friend ShaderSourceCache;

        void DoRelease() override;

        Memory::Pool<ShaderSourceFile>& m_pool;
        ShaderSourceCache* m_sourceCache = nullptr;
        char* m_source = nullptr;
        uint32_t m_sourceSize = 0;
        ShaderStage m_stage = ShaderStage::kUndefined;
    };


    struct ShaderSourceCache final : public Memory::RefCountedObjectBase
    {
        FE_RTTI("FE08F0A8-40B4-4C17-B152-8220DC1BF5F6");

        ShaderSourceCache();

        [[nodiscard]] bool IsLoading() const;

        festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> GetSource(Env::Name path);

    private:
        void ReadDirectory(Jobs::Graph& jobGraph, const IO::Path& path);
        void OnFileLoaded(const IO::Async::IController* controller, festd::string_view fullPath, Env::Name shaderName,
                          char* source, uint32_t sourceSize);

        void DoRelease() override;

        Memory::Pool<ShaderSourceFile> m_filePool;
        festd::segmented_unordered_dense_map<Env::Name, Rc<ShaderSourceFile>> m_filesMap;
        Threading::SharedSpinLock m_lock;
        std::atomic<uint32_t> m_loadingJobCount;
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
        return m_loadingJobCount.load(std::memory_order_acquire) > 0;
    }
} // namespace FE::Graphics::Core

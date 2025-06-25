#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>

namespace FE::Graphics::Core
{
    ShaderSourceFile::~ShaderSourceFile()
    {
        if (m_source)
            m_sourceAllocator->deallocate(m_source, m_sourceSize);

        m_source = nullptr;
    }


    ShaderSourceCache::ShaderSourceCache(IO::IAsyncStreamIO* asyncIO, Logger* logger)
        : m_filePool("Graphics/Core/ShaderSourceCache/FilePool")
        , m_asyncIO(asyncIO)
        , m_logger(logger)
    {
        FE_PROFILER_ZONE();

        ReadDirectory(IO::GetAbsolutePath("Shaders"));
        ReadDirectory(IO::GetAbsolutePath("../../Modules/Graphics/Framework/Shaders"));

        // TODO: probably we can wait later...
        while (m_loadingTasksCount.load(std::memory_order_acquire) > 0)
        {
            for (uint32_t i = 0; i < 32; ++i)
                _mm_pause();
        }
    }


    void ShaderSourceCache::ReadDirectory(const IO::Path& path)
    {
        const IO::ResultCode iterationResult =
            IO::Directory::TraverseRecursively(path, "*.hlsl;*.hlsli;*.h", [&](const IO::DirectoryEntry& entry) {
                if (!Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
                {
                    const festd::string_view pathStrView{ entry.m_path };
                    FE_Assert(pathStrView.starts_with(path));

                    const festd::string_view shaderNameView = pathStrView.substr_ascii(path.size() + 1);
                    const Env::Name shaderName{ shaderNameView };
                    m_loadingTasksCount.fetch_add(1, std::memory_order_release);

                    IO::AsyncReadRequest request;
                    request.m_callback = this;
                    request.m_path = entry.m_path;
                    request.m_userData0 = shaderName.GetHandle();
                    request.m_overallocateBytes = 1;
                    m_asyncIO->ReadAsync(request);
                }

                return true;
            });

        FE_AssertMsg(iterationResult == IO::ResultCode::kSuccess,
                     "Failed to traverse directory {}: {}",
                     path,
                     IO::GetResultDesc(iterationResult));
    }


    festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> ShaderSourceCache::GetSource(const Env::Name path)
    {
        FE_PROFILER_ZONE();

        FE_Assert(!IsLoading());

        std::shared_lock lk{ m_lock };

        const auto iter = m_filesMap.find(path);
        if (iter == m_filesMap.end())
            return festd::unexpected(IO::ResultCode::kNoFileOrDirectory);

        return iter->second;
    }


    void ShaderSourceCache::AsyncIOCallback(const IO::AsyncReadResult& result)
    {
        FE_PROFILER_ZONE();

        auto deferFree = festd::defer([&result, this] {
            result.FreeData();
            m_loadingTasksCount.fetch_sub(1, std::memory_order_release);
        });

        switch (result.m_controller->GetStatus())
        {
        case IO::AsyncOperationStatus::kFailed:
            m_logger->LogError("Failed to read shader file: {}", result.m_request->m_path);
            [[fallthrough]];

        case IO::AsyncOperationStatus::kCanceled:
            return;

        case IO::AsyncOperationStatus::kSucceeded:
            break;

        default:
            FE_Assert(false, "Unexpected");
            break;
        }

        const IO::PathView pathView{ result.m_request->m_path };
        const ShaderStage stage = GetShaderStageFromName(pathView.stem());

        if (pathView.extension() != ".hlsli" && pathView.extension() != ".h")
        {
            if (stage == ShaderStage::kUndefined)
            {
                m_logger->LogError("Couldn't determine shader stage: {}", result.m_request->m_path);
                return;
            }
        }

        deferFree.dismiss();

        const Rc file = Rc<ShaderSourceFile>::New(m_filePool.GetAllocator());
        file->m_sourceCache = this;
        file->m_source = reinterpret_cast<char*>(result.m_request->m_readBuffer);
        file->m_sourceSize = result.m_request->m_readBufferSize;
        file->m_sourceAllocator = result.m_request->m_allocator;
        file->m_stage = stage;
        file->m_source[file->m_sourceSize] = '\0';

        const auto shaderName = Env::Name::CreateFromHandle(static_cast<uint32_t>(result.m_request->m_userData0));

        const festd::string_view shaderNameStrView{ shaderName };

        std::lock_guard lk{ m_lock };
        m_filesMap[shaderName] = file;

        constexpr festd::string_view fidelityFxPrefix = "ThirdParty/FidelityFX/";
        if (shaderNameStrView.starts_with(fidelityFxPrefix))
        {
            const auto alias = shaderNameStrView.substr_ascii(fidelityFxPrefix.size());
            m_filesMap[Env::Name{ alias }] = file;
        }

        m_loadingTasksCount.fetch_sub(1, std::memory_order_release);
    }
} // namespace FE::Graphics::Core

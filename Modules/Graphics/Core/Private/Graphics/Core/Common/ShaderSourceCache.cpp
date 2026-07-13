#include <Core/IO/Async.h>
#include <Core/Jobs/JobGraph.h>
#include <Core/Logging/Trace.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>

namespace FE::Graphics::Core
{
    ShaderSourceFile::~ShaderSourceFile()
    {
        Memory::DefaultFree(m_source);
        m_source = nullptr;
    }


    void ShaderSourceFile::DoRelease()
    {
        m_pool.Delete(this);
    }


    ShaderSourceCache::ShaderSourceCache()
        : m_filePool("Graphics/ShaderSourceCache/FilePool")
    {
        FE_PROFILER_ZONE();

        Jobs::Graph jobGraph{ "Graphics/ShaderSourceCache/Load", Jobs::FiberAffinityMask::kAllBackground };
        ReadDirectory(jobGraph, IO::GetAbsolutePath("Shaders"));
        ReadDirectory(jobGraph, IO::GetAbsolutePath("../../Modules/Graphics/Framework/Shaders"));

        // TODO: probably we can wait later...
        jobGraph.Wait();
    }


    void ShaderSourceCache::ReadDirectory(Jobs::Graph& jobGraph, const IO::Path& path)
    {
        // Ignore non-existent directories.
        IO::Directory::TraverseRecursively(path, "*.hlsl;*.hlsli;*.h", [&](const IO::DirectoryEntry& entry) {
            if (!Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
            {
                const festd::string_view pathStrView{ entry.m_path };
                FE_Assert(pathStrView.starts_with(path));

                const festd::string_view shaderNameView = pathStrView.substr_ascii(path.size() + 1);
                const Env::Name shaderName{ shaderNameView };
                m_loadingJobCount.fetch_add(1, std::memory_order_release);

                const auto sourceSize = static_cast<uint32_t>(entry.m_stats.m_byteSize);
                char* source = Memory::DefaultAllocateArray<char>(sourceSize + 1);
                source[sourceSize] = 0;

                Rc completionWaitGroup = WaitGroup::Create();
                IO::Async::Batch batch{ completionWaitGroup.Get() };
                batch.SetSource(entry.m_path, entry.m_stats.m_byteSize);
                batch.Read(source, sourceSize);

                Rc controller = IO::Async::Read(std::move(batch));
                jobGraph.Dispatch(shaderName,
                                  { completionWaitGroup },
                                  [this, shaderName, source, sourceSize, controller, fullPath = IO::Path(entry.m_path)] {
                                      OnFileLoaded(controller.Get(), fullPath, shaderName, source, sourceSize);
                                  });
            }

            return true;
        });
    }


    void ShaderSourceCache::DoRelease()
    {
        Memory::DefaultDelete(this);
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


    void ShaderSourceCache::OnFileLoaded(const IO::IAsyncController* controller, const festd::string_view fullPath,
                                         const Env::Name shaderName, char* source, const uint32_t sourceSize)
    {
        auto deferFree = festd::defer([source, this] {
            Memory::DefaultFree(source);
            m_loadingJobCount.fetch_sub(1, std::memory_order_release);
        });

        switch (controller->GetStatus())
        {
        case IO::AsyncOperationStatus::kFailed:
            Logger::LogError("Failed to read shader file: {}", fullPath);
            [[fallthrough]];

        case IO::AsyncOperationStatus::kCanceled:
            return;

        case IO::AsyncOperationStatus::kSucceeded:
            break;

        default:
            FE_Assert(false, "Unexpected");
            break;
        }

        const IO::PathView pathView{ fullPath };
        const ShaderStage stage = GetShaderStageFromName(pathView.stem());

        if (pathView.extension() != ".hlsli" && pathView.extension() != ".h")
        {
            if (stage == ShaderStage::kUndefined)
            {
                Logger::LogError("Couldn't determine shader stage: {}", fullPath);
                return;
            }
        }

        deferFree.dismiss();

        const Rc file = m_filePool.New();
        file->m_sourceCache = this;
        file->m_source = source;
        file->m_sourceSize = sourceSize;
        file->m_stage = stage;
        file->m_source[file->m_sourceSize] = '\0';

        const festd::string_view shaderNameStrView{ shaderName };

        std::lock_guard lk{ m_lock };
        m_filesMap[shaderName] = file;

        constexpr festd::string_view fidelityFxPrefix = "ThirdParty/FidelityFX/";
        if (shaderNameStrView.starts_with(fidelityFxPrefix))
        {
            const auto alias = shaderNameStrView.substr_ascii(fidelityFxPrefix.size());
            m_filesMap[Env::Name{ alias }] = file;
        }

        m_loadingJobCount.fetch_sub(1, std::memory_order_release);
    }
} // namespace FE::Graphics::Core

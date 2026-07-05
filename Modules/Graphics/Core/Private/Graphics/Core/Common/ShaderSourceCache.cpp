#include <Core/IO/IAsyncStreamIO.h>
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


    ShaderSourceCache::ShaderSourceCache(IO::IAsyncStreamIO* asyncIO)
        : m_filePool("Graphics/Core/ShaderSourceCache/FilePool")
        , m_asyncIO(asyncIO)
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
        // Ignore if directory was not found.
        IO::Directory::TraverseRecursively(path, "*.hlsl;*.hlsli;*.h", [&](const IO::DirectoryEntry& entry) {
            if (!Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
            {
                const festd::string_view pathStrView{ entry.m_path };
                FE_Assert(pathStrView.starts_with(path));

                const festd::string_view shaderNameView = pathStrView.substr_ascii(path.size() + 1);
                const Env::Name shaderName{ shaderNameView };
                m_loadingTasksCount.fetch_add(1, std::memory_order_release);

                const auto sourceSize = static_cast<uint32_t>(entry.m_stats.m_byteSize);
                char* source = Memory::DefaultAllocateArray<char>(sourceSize + 1);
                source[sourceSize] = 0;

                Memory::StackTempAllocator<256> temp;
                IO::AsyncReadCommandListBuilder builder{ &temp, temp.Size() };
                builder.SetSource({ .m_filePath = entry.m_path, .m_byteOffset = 0, .m_byteSize = sourceSize });
                builder.Read(source, sourceSize);
                builder.InvokeOnCompletion(
                    [this, shaderName, source, sourceSize, fullPath = IO::Path(entry.m_path)](IO::IAsyncController* controller) {
                        OnFileLoaded(controller, fullPath, shaderName, source, sourceSize);
                    });
                m_asyncIO->ExecuteCommandList(builder.ShrinkAndBuild(std::pmr::get_default_resource()));
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


    void ShaderSourceCache::OnFileLoaded(IO::IAsyncController* controller, festd::string_view fullPath, Env::Name shaderName,
                                         char* source, uint32_t sourceSize)
    {
        FE_PROFILER_ZONE();

        auto deferFree = festd::defer([source, this] {
            Memory::DefaultFree(source);
            m_loadingTasksCount.fetch_sub(1, std::memory_order_release);
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

        const Rc file = m_filePool.New(m_filePool);
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

        m_loadingTasksCount.fetch_sub(1, std::memory_order_release);
    }
} // namespace FE::Graphics::Core

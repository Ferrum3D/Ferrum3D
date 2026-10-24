#include <Core/IO/Async.h>
#include <Core/IO/FileStream.h>
#include <Core/Logging/Trace.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderSourceDirectory.h>

namespace FE::Graphics::Core
{
    namespace
    {
        struct ShaderSourceDirectoryEntry final
        {
            IO::Path m_directory;
            festd::string m_virtualPrefix;
        };


        struct ShaderSourceDirectoryRegistry final
        {
            Threading::SharedSpinLock m_lock;
            festd::vector<ShaderSourceDirectoryEntry> m_entries;
        };


        ShaderSourceDirectoryRegistry& GetDirectoryRegistry()
        {
            static ShaderSourceDirectoryRegistry registry;
            return registry;
        }


        IO::Path FindSourceFile(const Env::Name path)
        {
            const festd::string_view pathView{ path };

            ShaderSourceDirectoryRegistry& registry = GetDirectoryRegistry();
            std::shared_lock lock{ registry.m_lock };

            IO::Path result;
            for (const ShaderSourceDirectoryEntry& entry : registry.m_entries)
            {
                const festd::string_view prefix{ entry.m_virtualPrefix };
                if (!pathView.starts_with(prefix))
                    continue;

                IO::Path candidate = entry.m_directory / pathView.substr_ascii(prefix.size());
                if (!IO::File::Exists(candidate))
                    continue;

                if (result.empty())
                    result = std::move(candidate);
                else
                    Logger::LogWarning("Shader source '{}' exists in several directories, using '{}'", path, result);
            }

            return result;
        }


        Rc<ShaderSourceFile> ReadSourceFileBlocking(const Env::Name path)
        {
            FE_PROFILER_ZONE();

            const IO::Path fullPath = FindSourceFile(path);
            if (fullPath.empty())
                return nullptr;

            const auto stream = IO::FileStream::Open(fullPath, IO::OpenMode::kReadOnly);
            if (!stream)
            {
                Logger::LogError("Failed to open shader source '{}': {}", fullPath, IO::GetResultDesc(stream.error()));
                return nullptr;
            }

            festd::pmr::vector<std::byte> source;
            source.resize(static_cast<uint32_t>(stream.value()->Length()));
            if (stream.value()->ReadToBuffer(source.data(), source.size()) != source.size())
            {
                Logger::LogError("Failed to read shader source '{}'", fullPath);
                return nullptr;
            }

            return Memory::DefaultNew<ShaderSourceFile>(path, std::move(source));
        }


        struct PendingSourceRead final
        {
            Env::Name m_path;
            IO::Path m_fullPath;
            festd::pmr::vector<std::byte> m_source;
            Rc<IO::Async::IController> m_controller;
        };
    } // namespace


    void AddShaderSourceDirectory(const festd::string_view directory, const festd::string_view virtualPrefix)
    {
        const IO::Path normalizedDirectory = IO::NormalizePath(directory);
        const IO::Path normalizedPrefix = IO::NormalizePath(virtualPrefix);
        FE_Assert(IO::PathView{ normalizedDirectory }.is_absolute(), "Shader source directory must be absolute");

        ShaderSourceDirectoryEntry entry;
        entry.m_directory = normalizedDirectory;
        entry.m_virtualPrefix = festd::string_view{ normalizedPrefix.data(), normalizedPrefix.size() };
        if (!entry.m_virtualPrefix.empty() && !entry.m_virtualPrefix.ends_with("/"))
            entry.m_virtualPrefix.push_back('/');

        ShaderSourceDirectoryRegistry& registry = GetDirectoryRegistry();
        std::lock_guard lock{ registry.m_lock };

        for (const ShaderSourceDirectoryEntry& existing : registry.m_entries)
        {
            if (existing.m_directory == entry.m_directory && existing.m_virtualPrefix == entry.m_virtualPrefix)
                return;
        }

        registry.m_entries.push_back(std::move(entry));
    }


    ShaderSourceFile::ShaderSourceFile(const Env::Name path, festd::pmr::vector<std::byte>&& source)
        : m_path(path)
        , m_source(std::move(source))
        , m_contentHash(DefaultHash(m_source.data(), m_source.size()))
    {
        m_source.push_back(std::byte{ 0 });
    }


    void ShaderSourceFile::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> ShaderSourceCache::GetSource(const Env::Name path,
                                                                                       const ShaderSourceReadMode mode)
    {
        FE_PROFILER_ZONE();

        if (const festd::optional cached = TryGetCached(path))
            return MakeResult(cached.value());

        if (mode == ShaderSourceReadMode::kAsync)
        {
            Prefetch(festd::span{ &path, 1 });
            return MakeResult(TryGetCached(path).value());
        }

        // Concurrent loads of the same file are harmless: the first inserted result wins and all callers share it.
        const Rc file = ReadSourceFileBlocking(path);

        std::lock_guard lock{ m_lock };
        const auto [iter, inserted] = m_filesMap.insert({ path, file });
        return MakeResult(iter->second);
    }


    void ShaderSourceCache::Prefetch(const festd::span<const Env::Name> paths)
    {
        FE_PROFILER_ZONE();

        // Reads append to the vectors in place, so the storage must not move once the first read starts.
        festd::vector<PendingSourceRead> reads;
        reads.reserve(paths.size());

        festd::inline_vector<Rc<WaitGroup>, 16> completions;
        festd::inline_vector<Env::Name, 16> missingPaths;
        for (const Env::Name path : paths)
        {
            if (TryGetCached(path))
                continue;

            IO::Path fullPath = FindSourceFile(path);
            if (fullPath.empty())
            {
                missingPaths.push_back(path);
                continue;
            }

            PendingSourceRead& read = reads.push_back();
            read.m_path = path;
            read.m_fullPath = std::move(fullPath);

            const Rc completion = WaitGroup::Create();
            IO::Async::Batch batch{ completion.Get() };
            batch.SetSource(IO::PathView{ read.m_fullPath });
            batch.ReadAppendToEnd(read.m_source);
            read.m_controller = IO::Async::Read(std::move(batch));
            completions.push_back(completion);
        }

        WaitGroup::WaitAll(festd::span<const Rc<WaitGroup>>{ completions.data(), completions.size() });

        std::lock_guard lock{ m_lock };

        for (const Env::Name path : missingPaths)
            m_filesMap.insert({ path, Rc<ShaderSourceFile>{} });

        for (PendingSourceRead& read : reads)
        {
            Rc<ShaderSourceFile> file;
            if (read.m_controller->GetStatus() == IO::Async::Status::kSucceeded)
            {
                file = Memory::DefaultNew<ShaderSourceFile>(read.m_path, std::move(read.m_source));
            }
            else
            {
                const festd::string_view resultDesc = IO::GetResultDesc(read.m_controller->GetLastOperationResult());
                Logger::LogError("Failed to read shader source '{}': {}", read.m_fullPath, resultDesc);
            }

            m_filesMap.insert({ read.m_path, std::move(file) });
        }
    }


    festd::optional<Rc<ShaderSourceFile>> ShaderSourceCache::TryGetCached(const Env::Name path)
    {
        std::shared_lock lock{ m_lock };

        const auto iter = m_filesMap.find(path);
        if (iter == m_filesMap.end())
            return festd::nullopt;

        return iter->second;
    }


    festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> ShaderSourceCache::MakeResult(const Rc<ShaderSourceFile>& file)
    {
        if (file.Get() == nullptr)
            return festd::unexpected(IO::ResultCode::kNoFileOrDirectory);

        return file;
    }


    void ShaderSourceCache::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics::Core

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
            uint32_t m_index = 0;
            Env::Name m_path;
            IO::Path m_fullPath;
            festd::pmr::vector<std::byte> m_source;
            Rc<IO::Async::IController> m_controller;
        };


        //! Read the paths with parallel asynchronous reads; files[i] is null when paths[i] is missing or unreadable.
        void ReadSourceFiles(const festd::span<const Env::Name> paths, festd::vector<Rc<ShaderSourceFile>>& files)
        {
            FE_PROFILER_ZONE();

            files.resize(paths.size());

            // Reads append to the vectors in place, so the storage must not move once the first read starts.
            festd::vector<PendingSourceRead> reads;
            reads.reserve(paths.size());

            festd::inline_vector<Rc<WaitGroup>, 16> completions;
            for (uint32_t index = 0; index < paths.size(); ++index)
            {
                IO::Path fullPath = FindSourceFile(paths[index]);
                if (fullPath.empty())
                    continue;

                PendingSourceRead& read = reads.push_back();
                read.m_index = index;
                read.m_path = paths[index];
                read.m_fullPath = std::move(fullPath);

                const Rc completion = WaitGroup::Create();
                IO::Async::Batch batch{ completion.Get() };
                batch.SetSource(IO::PathView{ read.m_fullPath });
                batch.ReadAppendToEnd(read.m_source);
                read.m_controller = IO::Async::Read(std::move(batch));
                completions.push_back(completion);
            }

            WaitGroup::WaitAll(festd::span<const Rc<WaitGroup>>{ completions.data(), completions.size() });

            for (PendingSourceRead& read : reads)
            {
                if (read.m_controller->GetStatus() != IO::Async::Status::kSucceeded)
                {
                    const festd::string_view resultDesc = IO::GetResultDesc(read.m_controller->GetLastOperationResult());
                    Logger::LogError("Failed to read shader source '{}': {}", read.m_fullPath, resultDesc);
                    continue;
                }

                files[read.m_index] = Memory::DefaultNew<ShaderSourceFile>(read.m_path, std::move(read.m_source));
            }
        }


        bool HasSourceChanged(const ShaderSourceFile* previous, const ShaderSourceFile* current)
        {
            if (previous == nullptr || current == nullptr)
                return previous != current;

            return previous->GetContentHash() != current->GetContentHash();
        }


        //! Append every virtual path under which a changed file is visible, skipping duplicates.
        void AppendVirtualPaths(const IO::Path& file, festd::vector<Env::Name>& paths)
        {
            const festd::string_view fileView{ file.data(), file.size() };

            ShaderSourceDirectoryRegistry& registry = GetDirectoryRegistry();
            std::shared_lock lock{ registry.m_lock };

            for (const ShaderSourceDirectoryEntry& entry : registry.m_entries)
            {
                const festd::string_view directory{ entry.m_directory.data(), entry.m_directory.size() };
                if (fileView.size() <= directory.size() || !fileView.starts_with(directory))
                    continue;

                if (fileView.data()[directory.size()] != '/')
                    continue;

                const festd::string_view relativePath = fileView.substr_ascii(directory.size() + 1);
                const Env::Name path{ Fmt::FixedFormat("{}{}", entry.m_virtualPrefix, relativePath) };
                if (festd::find_index(festd::span{ paths.data(), paths.size() }, path) == kInvalidIndex)
                    paths.push_back(path);
            }
        }
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

        festd::inline_vector<Env::Name, 16> uncachedPaths;
        for (const Env::Name path : paths)
        {
            if (!TryGetCached(path))
                uncachedPaths.push_back(path);
        }

        festd::vector<Rc<ShaderSourceFile>> files;
        ReadSourceFiles(festd::span{ uncachedPaths.data(), uncachedPaths.size() }, files);

        std::lock_guard lock{ m_lock };
        for (uint32_t index = 0; index < uncachedPaths.size(); ++index)
            m_filesMap.insert({ uncachedPaths[index], std::move(files[index]) });
    }


    void ShaderSourceCache::Refresh(const festd::span<const Env::Name> paths, festd::vector<Env::Name>& changedPaths)
    {
        FE_PROFILER_ZONE();

        festd::inline_vector<Env::Name, 16> cachedPaths;
        for (const Env::Name path : paths)
        {
            if (TryGetCached(path))
                cachedPaths.push_back(path);
        }

        festd::vector<Rc<ShaderSourceFile>> files;
        ReadSourceFiles(festd::span{ cachedPaths.data(), cachedPaths.size() }, files);

        std::lock_guard lock{ m_lock };
        for (uint32_t index = 0; index < cachedPaths.size(); ++index)
        {
            Rc<ShaderSourceFile>& cachedFile = m_filesMap[cachedPaths[index]];
            Rc<ShaderSourceFile>& file = files[index];
            if (!HasSourceChanged(cachedFile.Get(), file.Get()))
                continue;

            cachedFile = std::move(file);
            changedPaths.push_back(cachedPaths[index]);
        }
    }


    void ShaderSourceCache::PollChangedPaths(festd::vector<Env::Name>& paths)
    {
        FE_PROFILER_ZONE();

        WatchRegisteredDirectories();

        festd::vector<IO::Path> changedFiles;
        bool isComplete = true;
        {
            std::lock_guard lock{ m_watchersLock };
            for (const WatchedDirectory& directory : m_watchedDirectories)
            {
                if (directory.m_watcher)
                    isComplete &= directory.m_watcher->PollChanges(changedFiles);
            }
        }

        if (!isComplete)
        {
            std::shared_lock lock{ m_lock };
            for (const auto& [path, file] : m_filesMap)
                paths.push_back(path);

            return;
        }

        for (const IO::Path& file : changedFiles)
            AppendVirtualPaths(file, paths);
    }


    void ShaderSourceCache::WatchRegisteredDirectories()
    {
        festd::vector<IO::Path> directories;
        {
            ShaderSourceDirectoryRegistry& registry = GetDirectoryRegistry();
            std::shared_lock lock{ registry.m_lock };
            for (const ShaderSourceDirectoryEntry& entry : registry.m_entries)
                directories.push_back(entry.m_directory);
        }

        std::lock_guard lock{ m_watchersLock };
        for (const IO::Path& directory : directories)
        {
            if (IsWatched(directory))
                continue;

            // A directory that cannot be watched is still recorded, so it is not retried on every poll.
            WatchedDirectory& watched = m_watchedDirectories.push_back();
            watched.m_directory = directory;
            watched.m_watcher = IO::DirectoryWatcher::Create(directory);
            if (!watched.m_watcher)
                Logger::LogWarning("Shader hot reload cannot watch '{}'", directory);
        }
    }


    bool ShaderSourceCache::IsWatched(const IO::Path& directory) const
    {
        for (const WatchedDirectory& watched : m_watchedDirectories)
        {
            if (watched.m_directory == directory)
                return true;
        }

        return false;
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

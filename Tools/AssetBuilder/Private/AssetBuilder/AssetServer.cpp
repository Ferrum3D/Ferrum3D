#include <AssetBuilder/AssetServer.h>
#include <AssetBuilder/BuildState.h>
#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/ShaderProcessor.h>
#include <AssetBuilder/StatusPipe.h>
#include <Core/IO/DirectoryWatcher.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Logging/Logger.h>
#include <Core/Serialization/Document.h>
#include <Core/Threading/Mutex.h>
#include <Core/Time/BaseTime.h>

namespace FE::AssetBuilder
{
    using namespace Internal;

    namespace
    {
        //! Editors and version control often write a file in several steps, so wait until the depots have been quiet.
        constexpr double kChangeSettleSeconds = 0.15;


        //! Forwards warnings and errors to the status pipe. Sinks run under the logger lock, so messages are only queued here.
        struct StatusLogSink final : public Logger::SinkBase
        {
            void Log(const Logger::Severity severity, SourceLocation, const festd::string_view message) override
            {
                if (severity < Logger::Severity::kWarning)
                    return;

                std::lock_guard lock{ m_lock };
                m_messages.push_back(Fmt::Format("{}: {}", Logger::LogSeverityToString(severity), message));
            }

            void TakeMessages(festd::vector<festd::string>& messages)
            {
                std::lock_guard lock{ m_lock };
                messages = std::move(m_messages);
                m_messages.clear();
            }

        private:
            Threading::Mutex m_lock;
            festd::vector<festd::string> m_messages;
        };


        struct WatchedDirectory final
        {
            IO::Path m_directory;
            Rc<IO::DirectoryWatcher> m_watcher;
        };


        IO::Path GetServerStatePath(const IO::Path& outputDirectory)
        {
            return GetBuildDirectory(outputDirectory) / "server/state.json";
        }


        bool IsPathInside(const festd::string_view path, const festd::string_view directory)
        {
            return path.size() > directory.size() && PathsHaveEqualPrefix(path, directory)
                && path.byte_at(directory.size()) == '/';
        }
    } // namespace


    struct AssetServer::Impl final
    {
        AssetServerSettings m_settings;
        AssetDepot m_depot;
        IO::Path m_outputDirectory;
        festd::inline_vector<IO::Path, 2> m_ignoredDirectories;

        Rc<IO::ChangeJournal> m_journal;
        IO::ChangeJournalCursor m_journalCursor;
        festd::vector<IO::Path> m_journalDirectories;
        festd::vector<WatchedDirectory> m_watchedDirectories;

        festd::vector<IO::Path> m_pendingChanges;
        uint64_t m_lastChangeTicks = 0;
        bool m_needsFullBuild = false;

        StatusPipe m_statusPipe;
        StatusLogSink m_logSink;
        bool m_hasStatusPipe = false;

        uint32_t m_completedBuildCount = 0;
        festd::vector<IO::AssetID> m_lastPublishedProducts;

        bool OpenChangeJournal();
        void WatchDirectories();
        void ReadChanges();
        void AddChange(IO::Path&& path);
        void ProcessChanges();
        void BuildAll();
        void BuildChanged(festd::span<const IO::Path> changes);
        void Build(festd::vector<uint32_t>& assets);
        [[nodiscard]] bool HasPublishedShaderPackage(festd::span<const IO::AssetID> products) const;
        void PrewarmShaders();
        void SendStatus(festd::string_view line);
        void ForwardLogMessages();
        bool LoadState(AssetServerState& state) const;
        void SaveState() const;
    };


    bool AssetServer::Impl::OpenChangeJournal()
    {
        m_journal = IO::ChangeJournal::Open(m_outputDirectory);
        if (!m_journal)
        {
            Logger::LogInfo("The output volume has no readable change journal, watching directories instead");
            return false;
        }

        // The journal belongs to one volume, so every watched directory has to be on the same one.
        const uint64_t journalId = m_journal->GetEndCursor().m_journalId;
        for (const IO::Path& directory : m_depot.GetWatchedDirectories())
        {
            const Rc journal = IO::ChangeJournal::Open(directory);
            if (!journal || journal->GetEndCursor().m_journalId != journalId)
            {
                Logger::LogInfo("'{}' is on another volume, watching directories instead of reading the change journal",
                                directory);
                m_journal.Reset();
                return false;
            }
        }

        return true;
    }


    void AssetServer::Impl::WatchDirectories()
    {
        for (const IO::Path& directory : m_depot.GetWatchedDirectories())
        {
            if (m_journal)
            {
                if (festd::find_index(festd::span{ m_journalDirectories.data(), m_journalDirectories.size() }, directory)
                    != kInvalidIndex)
                {
                    continue;
                }

                if (!m_journal->AddDirectory(directory))
                    Logger::LogWarning("Cannot read changes of '{}'", directory);

                m_journalDirectories.push_back(directory);
                continue;
            }

            bool isWatched = false;
            for (const WatchedDirectory& watched : m_watchedDirectories)
                isWatched |= watched.m_directory == directory;

            if (isWatched)
                continue;

            WatchedDirectory& watched = m_watchedDirectories.push_back();
            watched.m_directory = directory;
            watched.m_watcher = IO::DirectoryWatcher::Create(directory);
            if (!watched.m_watcher)
                Logger::LogWarning("Cannot watch '{}'", directory);
        }
    }


    void AssetServer::Impl::ReadChanges()
    {
        FE_PROFILER_ZONE();

        festd::vector<IO::Path> changes;
        if (m_journal)
        {
            const IO::ChangeJournalReadResult result = m_journal->ReadChanges(m_journalCursor, changes);
            if (result != IO::ChangeJournalReadResult::kSuccess)
            {
                // Changes may have been lost, so every asset has to be checked again.
                m_needsFullBuild = true;
                m_lastChangeTicks = Platform::GetTicks();
            }
        }
        else
        {
            for (const WatchedDirectory& watched : m_watchedDirectories)
            {
                if (watched.m_watcher && !watched.m_watcher->PollChanges(changes))
                {
                    m_needsFullBuild = true;
                    m_lastChangeTicks = Platform::GetTicks();
                }
            }
        }

        for (IO::Path& change : changes)
            AddChange(std::move(change));
    }


    void AssetServer::Impl::AddChange(IO::Path&& path)
    {
        const festd::string_view pathView{ path.data(), path.size() };
        for (const IO::Path& ignored : m_ignoredDirectories)
        {
            if (IsPathInside(pathView, festd::string_view{ ignored.data(), ignored.size() }))
                return;
        }

        m_pendingChanges.push_back(std::move(path));
        m_lastChangeTicks = Platform::GetTicks();
    }


    void AssetServer::Impl::ProcessChanges()
    {
        FE_PROFILER_ZONE();

        const double secondsSinceChange =
            static_cast<double>(Platform::GetTicks() - m_lastChangeTicks) * Platform::GetSecondsPerTick();
        if (secondsSinceChange < kChangeSettleSeconds)
            return;

        festd::vector<IO::Path> changes = std::move(m_pendingChanges);
        m_pendingChanges.clear();

        bool areDefinitionsChanged = m_needsFullBuild;
        for (const IO::Path& change : changes)
            areDefinitionsChanged |= m_depot.IsDefinitionPath(festd::string_view{ change.data(), change.size() });

        if (areDefinitionsChanged)
        {
            if (!m_depot.Scan(m_settings.m_depot))
            {
                SendStatus("error: the depots cannot be scanned, waiting for further changes");
                return;
            }

            WatchDirectories();
        }

        if (m_needsFullBuild)
        {
            m_needsFullBuild = false;
            BuildAll();
        }
        else
        {
            BuildChanged(festd::span{ changes.data(), changes.size() });
        }

        SaveState();
    }


    void AssetServer::Impl::BuildAll()
    {
        festd::vector<uint32_t> assets;
        m_depot.GetAllAssets(assets);
        Build(assets);
    }


    void AssetServer::Impl::BuildChanged(const festd::span<const IO::Path> changes)
    {
        festd::vector<uint32_t> assets;
        m_depot.FindAffectedAssets(changes, assets);
        if (assets.empty())
            return;

        m_depot.AddDependents(assets);
        Build(assets);
    }


    void AssetServer::Impl::Build(festd::vector<uint32_t>& assets)
    {
        FE_PROFILER_ZONE();

        SendStatus(Fmt::FixedFormat("building: {} assets", assets.size()));

        const DepotBuildResult result = BuildDepotAssets(m_depot, festd::span{ assets.data(), assets.size() });
        ForwardLogMessages();

        const festd::fixed_string summary = Fmt::FixedFormat("built: {} succeeded, {} failed, {} skipped, {} products published",
                                                             result.m_succeededCount,
                                                             result.m_failedCount,
                                                             result.m_skippedCount,
                                                             result.m_publishedProducts.size());
        Logger::LogInfo("{}", summary);
        SendStatus(summary);

        m_lastPublishedProducts = result.m_publishedProducts;
        if (m_settings.m_prewarmShaders
            && HasPublishedShaderPackage(festd::span{ result.m_publishedProducts.data(), result.m_publishedProducts.size() }))
            PrewarmShaders();

        ++m_completedBuildCount;
    }


    bool AssetServer::Impl::HasPublishedShaderPackage(const festd::span<const IO::AssetID> products) const
    {
        for (const DepotAsset& asset : m_depot.GetAssets())
        {
            for (const AssetFileArtifact& artifact : asset.m_definition.m_artifacts)
            {
                const bool isPackage = artifact.m_assetTypeId == Rtti::GetTypeID<Graphics::ShaderPackageAsset>();
                if (isPackage && festd::find_index(products, artifact.m_assetId) != kInvalidIndex)
                    return true;
            }
        }

        return false;
    }


    void AssetServer::Impl::PrewarmShaders()
    {
        FE_PROFILER_ZONE();

        festd::vector<Graphics::Core::ShaderPermutation> permutations;
        const IO::Path manifestPath = Graphics::Core::GetShaderPermutationManifestPath(m_outputDirectory);
        if (!Graphics::Core::LoadShaderPermutationManifest(manifestPath, permutations) || permutations.empty())
            return;

        SendStatus(Fmt::FixedFormat("prewarming: {} shader permutations", permutations.size()));

        const bool isComplete = CompileShaderPermutations(m_depot,
                                                          festd::span{ permutations.data(), permutations.size() },
                                                          Graphics::Core::ShaderCompilerConfig::kDefault,
                                                          nullptr);
        ForwardLogMessages();
        SendStatus(isComplete ? "prewarmed: every shader permutation compiled" : "prewarmed: some shader permutations failed");
    }


    void AssetServer::Impl::SendStatus(const festd::string_view line)
    {
        if (m_hasStatusPipe)
            m_statusPipe.Broadcast(line);
    }


    void AssetServer::Impl::ForwardLogMessages()
    {
        festd::vector<festd::string> messages;
        m_logSink.TakeMessages(messages);
        for (const festd::string& message : messages)
            SendStatus(message);
    }


    bool AssetServer::Impl::LoadState(AssetServerState& state) const
    {
        auto stream = IO::FileStream::Open(GetServerStatePath(m_outputDirectory), IO::OpenMode::kReadOnly);
        if (!stream)
            return false;

        return Serialization::LoadDocument(*stream.value(), state) == Serialization::ResultCode::kSuccess;
    }


    void AssetServer::Impl::SaveState() const
    {
        if (!m_journal)
            return;

        AssetServerState state;
        state.m_journalCursor = m_journalCursor;

        festd::pmr::vector<std::byte> bytes;
        IO::WriteOnlyVectorStream stream{ bytes };
        Serialization::JsonFormat format;
        Serialization::SerializationContext context(&stream, format);
        if (context.Store(state) != Serialization::ResultCode::kSuccess
            || !WriteFileAtomically(GetServerStatePath(m_outputDirectory), bytes))
        {
            Logger::LogWarning("Failed to save the asset server state; the next start checks every asset");
        }
    }


    AssetServer::AssetServer(const AssetServerSettings& settings)
    {
        m_impl = Memory::DefaultNew<Impl>();
        m_impl->m_settings = settings;
        m_impl->m_outputDirectory = IO::NormalizePath(IO::GetAbsolutePath(settings.m_depot.m_outputDirectory));
        m_impl->m_ignoredDirectories.push_back(m_impl->m_outputDirectory / "artifacts");
        m_impl->m_ignoredDirectories.push_back(GetBuildDirectory(m_impl->m_outputDirectory));
    }


    AssetServer::~AssetServer()
    {
        Memory::DefaultDelete(m_impl);
    }


    bool AssetServer::Start()
    {
        FE_PROFILER_ZONE();

        Impl& impl = *m_impl;
        if (impl.m_settings.m_enableStatusPipe)
        {
            if (!impl.m_statusPipe.Open(GetAssetServerPipeName(impl.m_outputDirectory)))
            {
                Logger::LogError("Another asset server is already building into '{}'", impl.m_outputDirectory);
                return false;
            }

            impl.m_hasStatusPipe = true;
        }

        if (!impl.m_depot.Scan(impl.m_settings.m_depot))
            return false;

        if (impl.m_settings.m_useChangeJournal)
            impl.OpenChangeJournal();

        impl.WatchDirectories();

        // With a journal and a saved cursor, only the changes made since the previous run need building. The end cursor is
        // taken before a full build, so that changes made during it are built afterwards.
        bool isCaughtUp = false;
        festd::vector<IO::Path> missedChanges;
        if (impl.m_journal)
        {
            AssetServerState state;
            if (!impl.m_settings.m_fullScan && impl.LoadState(state))
            {
                impl.m_journalCursor = state.m_journalCursor;
                isCaughtUp =
                    impl.m_journal->ReadChanges(impl.m_journalCursor, missedChanges) == IO::ChangeJournalReadResult::kSuccess;
            }

            if (!isCaughtUp)
                impl.m_journalCursor = impl.m_journal->GetEndCursor();
        }

        if (isCaughtUp)
        {
            Logger::LogInfo("{} files changed since the previous run", missedChanges.size());
            for (IO::Path& change : missedChanges)
                impl.AddChange(std::move(change));

            festd::vector<IO::Path> changes = std::move(impl.m_pendingChanges);
            impl.m_pendingChanges.clear();

            // The startup build counts once, even if nothing needed building.
            const uint32_t previousBuildCount = impl.m_completedBuildCount;
            impl.BuildChanged(festd::span{ changes.data(), changes.size() });
            if (impl.m_completedBuildCount == previousBuildCount)
                ++impl.m_completedBuildCount;
        }
        else
        {
            impl.BuildAll();
        }

        impl.SaveState();
        impl.SendStatus("idle");
        return true;
    }


    void AssetServer::Poll()
    {
        FE_PROFILER_ZONE();

        Impl& impl = *m_impl;
        if (impl.m_hasStatusPipe)
            impl.m_statusPipe.AcceptClients();

        impl.ForwardLogMessages();
        impl.ReadChanges();

        if (impl.m_pendingChanges.empty() && !impl.m_needsFullBuild)
            return;

        const uint32_t previousBuildCount = impl.m_completedBuildCount;
        impl.ProcessChanges();
        if (impl.m_completedBuildCount != previousBuildCount)
            impl.SendStatus("idle");
    }


    bool AssetServer::IsUsingChangeJournal() const
    {
        return m_impl->m_journal.Get() != nullptr;
    }


    bool AssetServer::HasPendingChanges() const
    {
        return !m_impl->m_pendingChanges.empty() || m_impl->m_needsFullBuild;
    }


    uint32_t AssetServer::GetCompletedBuildCount() const
    {
        return m_impl->m_completedBuildCount;
    }


    festd::span<const IO::AssetID> AssetServer::GetLastPublishedProducts() const
    {
        return festd::span{ m_impl->m_lastPublishedProducts.data(), m_impl->m_lastPublishedProducts.size() };
    }


    const AssetDepot& AssetServer::GetDepot() const
    {
        return m_impl->m_depot;
    }
} // namespace FE::AssetBuilder

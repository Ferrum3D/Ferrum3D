#pragma once
#include <AssetBuilder/AssetDepot.h>
#include <Core/IO/ChangeJournal.h>

namespace FE::AssetBuilder
{
    //! @brief State that the asset server keeps between runs, in the output directory's build directory.
    struct AssetServerState final
    {
        FE_RTTI_Reflect("ED82B6DD-6255-4C87-879E-2E8DE5B70F76");
        FE_RTTI_Serialize();

        //! Journal position up to which every change has been built.
        IO::ChangeJournalCursor m_journalCursor;
    };


    struct AssetServerSettings final
    {
        DepotSettings m_depot;

        //! Read changes from the volume change journal when every watched directory is on one volume with a journal. Otherwise
        //! directories are watched while the server runs, and everything is checked at startup.
        bool m_useChangeJournal = true;

        //! Check every asset at startup, even if the journal could tell what changed since the previous run.
        bool m_fullScan = false;

        //! Serve build status on the pipe named by GetAssetServerPipeName.
        bool m_enableStatusPipe = true;

        //! When a shader package is rebuilt, compile the permutations in the output's shader permutation manifest into the
        //! shader cache next to the executable, so that executables in the same directory find them there.
        bool m_prewarmShaders = true;
    };


    //! @brief Keeps the artifacts of a set of depots up to date while it runs.
    //!
    //! At startup the server builds what changed since its previous run: with the change journal it reads only the changes made
    //! in between, otherwise it checks every asset. While running, it debounces file changes, rebuilds the definitions they
    //! affect and every dependent in dependency order, and runs independent builds in parallel. Builds of unchanged inputs are
    //! no-ops thanks to build keys. Builds write only to the output directory, so the server never sees its own writes.
    struct AssetServer final
    {
        explicit AssetServer(const AssetServerSettings& settings);
        ~AssetServer();

        AssetServer(const AssetServer&) = delete;
        AssetServer& operator=(const AssetServer&) = delete;

        //! @brief Scan the depots, start watching and build what changed. Returns false if the server cannot run.
        bool Start();

        //! @brief Read changes and build them once they have settled. Call regularly from the thread that called Start.
        void Poll();

        [[nodiscard]] bool IsUsingChangeJournal() const;

        //! @brief True while changes have been seen but not built yet.
        [[nodiscard]] bool HasPendingChanges() const;

        //! @brief Number of builds completed, including the one at startup.
        [[nodiscard]] uint32_t GetCompletedBuildCount() const;

        //! @brief Products published by the most recent build.
        [[nodiscard]] festd::span<const IO::AssetID> GetLastPublishedProducts() const;

        [[nodiscard]] const AssetDepot& GetDepot() const;

    private:
        struct Impl;
        Impl* m_impl = nullptr;
    };


    //! @brief Name of the status pipe of the asset server that builds into an output directory.
    //!
    //! Clients read text lines: "building: ...", "built: ...", "idle", and the warnings and errors logged by builds.
    festd::string GetAssetServerPipeName(const IO::Path& outputDirectory);
} // namespace FE::AssetBuilder

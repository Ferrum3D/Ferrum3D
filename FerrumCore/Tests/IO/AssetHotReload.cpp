#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Threading/Thread.h>
#include <IO/AssetTestTypes.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    namespace
    {
        const AssetID kLeaf("21212121-2121-4121-8121-212121212121");
        const AssetID kRoot("23232323-2323-4323-8323-232323232323");
        const AssetID kUnloaded("25252525-2525-4525-8525-252525252525");


        template<class T>
        festd::pmr::vector<std::byte> ToJson(const T& value)
        {
            festd::pmr::vector<std::byte> bytes;
            WriteOnlyVectorStream stream(bytes);
            Serialization::JsonFormat format;
            Serialization::SerializationContext context(&stream, format);
            EXPECT_EQ(context.Store(value), Serialization::ResultCode::kSuccess);
            return bytes;
        }


        void WriteBytes(const Path& path, const festd::span<const std::byte> bytes)
        {
            ASSERT_EQ(Directory::Create(PathView{ path }.parent_directory()), ResultCode::kSuccess);

            auto stream = FileStream::Open(path, OpenMode::kCreate);
            ASSERT_TRUE(stream);
            ASSERT_EQ(stream.value()->WriteFromBuffer(bytes.data(), bytes.size()), bytes.size());
            stream.value()->Close();
        }


        //! Replace a file atomically, like the asset builder publishes metadata.
        void ReplaceBytes(const Path& path, const festd::span<const std::byte> bytes)
        {
            Path temporaryPath = path;
            temporaryPath.append(".tmp");
            WriteBytes(temporaryPath, bytes);
            ASSERT_EQ(File::Replace(temporaryPath, path), ResultCode::kSuccess);
        }


        void RemoveDirectoryTree(const Path& root)
        {
            festd::vector<Path> files;
            festd::vector<Path> directories;
            Directory::TraverseRecursively(root, [&files, &directories](const DirectoryEntry& entry) {
                if (Bit::AnySet(entry.m_attributes, FileAttributeFlags::kDirectory))
                    directories.push_back(Path{ entry.m_path });
                else
                    files.push_back(Path{ entry.m_path });

                return true;
            });

            for (const Path& file : files)
                File::Delete(file);

            for (auto iter = directories.rbegin(); iter != directories.rend(); ++iter)
                Directory::Delete(*iter);

            Directory::Delete(root);
        }


        //! A temporary catalog with an asset manager that watches it.
        struct AssetHotReloadTest : testing::Test
        {
            void SetUp() override
            {
                m_root = Directory::GetExecutableDirectory() / Fmt::FixedFormat("AssetHotReloadTests/{}", Uuid::Random());
                ASSERT_EQ(Directory::Create(m_root / "artifacts"), ResultCode::kSuccess);
                ArtifactStore::SetCatalogSource(m_root);
                AssetManager::Init();
            }

            void TearDown() override
            {
                AssetManager::Shutdown();
                ArtifactStore::SetCatalogSource(Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts");
                RemoveDirectoryTree(m_root);
            }

            //! Publish a SyntheticAsset artifact the way the asset builder does: data first, then metadata.
            ArtifactID Publish(const AssetID assetId, const uint32_t value, const AssetID dependency = AssetID::kNull)
            {
                SyntheticAsset asset;
                asset.m_value = value;
                asset.m_dependency.SetAssetID(dependency);
                const festd::pmr::vector<std::byte> payload = ToJson(asset);

                const ArtifactID artifactId = Uuid::Random();
                WriteBytes(ArtifactStore::ResolveData(artifactId).m_filePath, payload);

                ArtifactChunkRecord chunk;
                chunk.m_compressedSize = payload.size();
                chunk.m_uncompressedSize = payload.size();
                chunk.m_compressionMethod = Compression::Method::kNone;
                chunk.m_checksum.m_current = Crc32::Compute(payload.data(), payload.size());

                ArtifactPayloadRecord payloadRecord;
                payloadRecord.m_resolvedDataSource.m_byteSize = payload.size();
                payloadRecord.m_chunks.push_back(chunk);

                ArtifactRecord record;
                record.m_artifactId = artifactId;
                record.m_assetId = assetId;
                record.m_assetTypeId = Rtti::GetTypeID<SyntheticAsset>();
                record.m_payloads.push_back(std::move(payloadRecord));
                if (dependency.IsValid())
                    record.m_dependencies.push_back({ dependency, Rtti::GetTypeID<SyntheticAsset>(), DependencyKind::kHard });

                ReplaceBytes(ArtifactStore::ResolveMeta(assetId).m_filePath, ToJson(record));
                return artifactId;
            }

            Path m_root;
        };


        //! Records finalization order and reload notifications with a configurable propagation policy.
        struct PolicyStreamer final : Streamer
        {
            AssetFinalizeResult FinalizeAssetLoading(AssetSlot& slot, const ArtifactRecord&, void* candidate) override
            {
                const auto* asset = static_cast<const SyntheticAsset*>(candidate);
                uint32_t dependencyValue = 0;
                if (asset->m_dependency.GetAssetID().IsValid())
                {
                    const AssetRead<SyntheticAsset> dependency = asset->m_dependency.GetAssetHandle().Read();
                    dependencyValue = dependency ? dependency->m_value : 0;
                }

                m_finalized.push_back({ slot.m_assetId, asset->m_value, dependencyValue });
                return AssetFinalizeResult::kSucceeded;
            }

            AssetFinalizeResult PollFinalize(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kSucceeded;
            }

            void CancelFinalize(AssetSlot&, void*) override {}

            bool RequiresFinalizedDependencies() const override
            {
                return true;
            }

            DependencyReloadPolicy GetDependencyReloadPolicy(Rtti::TypeID) const override
            {
                return m_policy;
            }

            void OnDependencyReplaced(AssetSlot& dependent, void*, AssetSlot& dependency) override
            {
                m_notifications.push_back({ dependent.m_assetId, dependency.m_assetId });
            }

            struct Finalized final
            {
                AssetID m_assetId;
                uint32_t m_value = 0;
                uint32_t m_dependencyValue = 0;
            };

            struct Notification final
            {
                AssetID m_dependent;
                AssetID m_dependency;
            };

            DependencyReloadPolicy m_policy = DependencyReloadPolicy::kNone;
            festd::vector<Finalized> m_finalized;
            festd::vector<Notification> m_notifications;
        };


        //! Tick like an application frame loop until the predicate holds or a timeout expires.
        template<class TPredicate>
        bool TickUntil(TPredicate&& predicate)
        {
            for (uint32_t iteration = 0; iteration < 2000; ++iteration)
            {
                AssetManager::Tick();
                if (predicate())
                    return true;

                Threading::Sleep(5);
            }

            return false;
        }


        //! Tick long enough for a change that is expected to have no effect to be picked up.
        void TickFor(const uint32_t iterationCount)
        {
            for (uint32_t iteration = 0; iteration < iterationCount; ++iteration)
            {
                AssetManager::Tick();
                Threading::Sleep(5);
            }
        }


        AssetRequest LoadAndWait(const AssetID assetId)
        {
            AssetRequest request = AssetManager::LoadAsset(assetId);
            EXPECT_TRUE(TickUntil([&request] {
                return request.IsCompleted();
            }));
            EXPECT_EQ(request.GetResult(), AssetLoadResult::kSucceeded) << request.GetError().data();
            return request;
        }


        uint32_t ReadValue(const AssetHandle<SyntheticAsset>& handle)
        {
            const AssetRead<SyntheticAsset> asset = handle.Read();
            return asset ? asset->m_value : 0;
        }
    } // namespace


    TEST_F(AssetHotReloadTest, ReplacedMetadataReloadsOnlyLoadedAssets)
    {
        Publish(kLeaf, 1);
        Publish(kUnloaded, 1);
        const AssetRequest request = LoadAndWait(kLeaf);
        const AssetHandle<SyntheticAsset> handle(request.GetAssetSlot());
        const uint32_t generation = handle.GetGeneration();
        ASSERT_EQ(ReadValue(handle), 1u);

        Publish(kLeaf, 2);
        Publish(kUnloaded, 2);
        ASSERT_TRUE(TickUntil([&handle] {
            return ReadValue(handle) == 2;
        }));
        EXPECT_EQ(handle.GetGeneration(), generation + 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kUnloaded), nullptr);
    }


    TEST_F(AssetHotReloadTest, ReplacedDataFileReloadsThePublishedGeneration)
    {
        const ArtifactID artifactId = Publish(kLeaf, 1);
        const AssetRequest request = LoadAndWait(kLeaf);
        const AssetHandle<SyntheticAsset> handle(request.GetAssetSlot());
        const uint32_t generation = handle.GetGeneration();

        // Restoring the same bytes, e.g. from version control, must still reach the runtime.
        SyntheticAsset asset;
        asset.m_value = 1;
        ReplaceBytes(ArtifactStore::ResolveData(artifactId).m_filePath, ToJson(asset));
        EXPECT_TRUE(TickUntil([&handle, generation] {
            return handle.GetGeneration() == generation + 1;
        }));
        EXPECT_EQ(ReadValue(handle), 1u);
    }


    TEST_F(AssetHotReloadTest, BrokenMetadataKeepsTheCurrentGenerationUntilFixed)
    {
        Publish(kLeaf, 1);
        const AssetRequest request = LoadAndWait(kLeaf);
        const AssetHandle<SyntheticAsset> handle(request.GetAssetSlot());
        const uint32_t generation = handle.GetGeneration();

        constexpr festd::string_view kGarbage = "not metadata";
        ReplaceBytes(ArtifactStore::ResolveMeta(kLeaf).m_filePath,
                     festd::span{ reinterpret_cast<const std::byte*>(kGarbage.data()), kGarbage.size() });
        TickFor(150);
        EXPECT_TRUE(handle.IsReady());
        EXPECT_EQ(handle.GetGeneration(), generation);
        EXPECT_EQ(ReadValue(handle), 1u);

        Publish(kLeaf, 3);
        EXPECT_TRUE(TickUntil([&handle] {
            return ReadValue(handle) == 3;
        }));
    }


    TEST_F(AssetHotReloadTest, ReloadPolicyRebuildsDependentsAfterTheirDependency)
    {
        PolicyStreamer streamer;
        streamer.m_policy = DependencyReloadPolicy::kReload;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        auto unregisterStreamer = festd::defer([&streamer] {
            AssetManager::UnregisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        });

        Publish(kLeaf, 1);
        Publish(kRoot, 10, kLeaf);
        const AssetRequest request = LoadAndWait(kRoot);
        const AssetHandle<SyntheticAsset> root(request.GetAssetSlot());
        const uint32_t rootGeneration = root.GetGeneration();
        streamer.m_finalized.clear();

        Publish(kLeaf, 2);
        ASSERT_TRUE(TickUntil([&root, rootGeneration] {
            return root.GetGeneration() == rootGeneration + 1;
        }));

        ASSERT_EQ(streamer.m_finalized.size(), 2u);
        EXPECT_EQ(streamer.m_finalized[0].m_assetId, kLeaf);
        EXPECT_EQ(streamer.m_finalized[1].m_assetId, kRoot);
        EXPECT_EQ(streamer.m_finalized[1].m_dependencyValue, 2u);
        EXPECT_EQ(ReadValue(root), 10u);
        EXPECT_TRUE(streamer.m_notifications.empty());
    }


    TEST_F(AssetHotReloadTest, DependentChangedWithItsDependencyReloadsOnceAfterIt)
    {
        PolicyStreamer streamer;
        streamer.m_policy = DependencyReloadPolicy::kReload;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        auto unregisterStreamer = festd::defer([&streamer] {
            AssetManager::UnregisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        });

        Publish(kLeaf, 1);
        Publish(kRoot, 10, kLeaf);
        const AssetRequest request = LoadAndWait(kRoot);
        const AssetHandle<SyntheticAsset> root(request.GetAssetSlot());
        const uint32_t rootGeneration = root.GetGeneration();
        streamer.m_finalized.clear();

        // The dependent is published first, so a naive reload would finalize it against the old dependency.
        Publish(kRoot, 20, kLeaf);
        Publish(kLeaf, 2);
        ASSERT_TRUE(TickUntil([&root] {
            return ReadValue(root) == 20;
        }));
        TickFor(150);

        EXPECT_EQ(root.GetGeneration(), rootGeneration + 1);
        ASSERT_EQ(streamer.m_finalized.size(), 2u);
        EXPECT_EQ(streamer.m_finalized[0].m_assetId, kLeaf);
        EXPECT_EQ(streamer.m_finalized[1].m_assetId, kRoot);
        EXPECT_EQ(streamer.m_finalized[1].m_dependencyValue, 2u);
    }


    TEST_F(AssetHotReloadTest, NotifyPolicyUpdatesDependentsInPlace)
    {
        PolicyStreamer streamer;
        streamer.m_policy = DependencyReloadPolicy::kNotify;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        auto unregisterStreamer = festd::defer([&streamer] {
            AssetManager::UnregisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        });

        Publish(kLeaf, 1);
        Publish(kRoot, 10, kLeaf);
        const AssetRequest request = LoadAndWait(kRoot);
        const AssetHandle<SyntheticAsset> root(request.GetAssetSlot());
        const uint32_t rootGeneration = root.GetGeneration();

        Publish(kLeaf, 2);
        ASSERT_TRUE(TickUntil([&streamer] {
            return !streamer.m_notifications.empty();
        }));
        TickFor(50);

        ASSERT_EQ(streamer.m_notifications.size(), 1u);
        EXPECT_EQ(streamer.m_notifications[0].m_dependent, kRoot);
        EXPECT_EQ(streamer.m_notifications[0].m_dependency, kLeaf);
        EXPECT_EQ(root.GetGeneration(), rootGeneration);
    }
} // namespace FE::IO::Tests

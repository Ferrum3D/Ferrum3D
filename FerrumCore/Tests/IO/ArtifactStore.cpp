#include <Core/IO/Artifact.h>
#include <Core/IO/Async.h>
#include <Core/IO/FileStream.h>
#include <Core/Jobs/WaitGroup.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    TEST(ArtifactStore, MetadataSchemaHashIsStable)
    {
        EXPECT_EQ(ArtifactRecord::RTTI_GetSerializationSchemaHash(), 5565015997505026501ull);
    }


    namespace
    {
        constexpr festd::string_view kTypeId = "334f0750-1b4e-4f4c-ac6f-985382d4bd11";

        Path GetFixtureRoot()
        {
            return Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts";
        }

        festd::vector<std::byte> ReadSource(const Path& path)
        {
            const auto openResult = FileStream::Open(path, OpenMode::kReadOnly);
            EXPECT_TRUE(openResult.has_value());
            if (!openResult)
                return {};
            festd::vector<std::byte> bytes;
            bytes.resize(static_cast<uint32_t>(openResult.value()->Length()));
            EXPECT_EQ(openResult.value()->ReadToBuffer(bytes.data(), bytes.size()), bytes.size());
            return bytes;
        }

        festd::vector<std::byte> ReadFixture(const festd::string_view name)
        {
            return ReadSource(GetFixtureRoot() / name);
        }

        ArtifactDecodeResult DecodeFixture(const AssetID assetId)
        {
            const ResolvedDataSource source = ArtifactStore::ResolveMeta(assetId);
            const festd::vector<std::byte> bytes = ReadSource(source.m_filePath);
            ArtifactResolutionContext context;
            context.m_assetId = assetId;
            context.m_metadataSource = source;
            return ArtifactStore::Decode(bytes, context);
        }

        ArtifactDecodeResult DecodeText(const festd::string_view text, const AssetID assetId)
        {
            ArtifactResolutionContext context;
            context.m_assetId = assetId;
            context.m_metadataSource.m_filePath = "inline.json";
            return ArtifactStore::Decode({ reinterpret_cast<const std::byte*>(text.data()), text.size() }, context);
        }

        festd::string MakeMetadata(const festd::string_view payload, const festd::string_view typeId = kTypeId,
                                   const uint32_t version = 0, const festd::string_view schema = "0x4d3ae890a4a739c5")
        {
            return Fmt::Format(
                R"({{"$type":"bfcd80e4-db85-41f2-b43d-f08ea62aa6ec","$version":{},"$schema":"{}","$value":{{"m_artifactId":"aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa","m_assetId":"11111111-1111-4111-8111-111111111111","m_assetTypeId":"{}","m_payloads":[{}],"m_dependencies":[]}}}})",
                version,
                schema,
                typeId,
                payload);
        }
    } // namespace

    TEST(ArtifactStore, DecodesHandAuthoredTypedDependencyFixtures)
    {
        ArtifactStore::SetCatalogSource(GetFixtureRoot());
        const ArtifactDecodeResult chain = DecodeFixture(AssetID("22222222-2222-4222-8222-222222222222"));
        ASSERT_TRUE(chain);
        ASSERT_EQ(chain->m_dependencies.size(), 1);
        EXPECT_EQ(chain->m_dependencies[0].m_kind, DependencyKind::kHard);
        EXPECT_EQ(chain->m_dependencies[0].m_expectedTypeId, Rtti::TypeID(festd::ascii_view{ kTypeId.data(), kTypeId.size() }));

        const ArtifactDecodeResult soft = DecodeFixture(AssetID("88888888-8888-4888-8888-888888888888"));
        ASSERT_TRUE(soft);
        ASSERT_EQ(soft->m_dependencies.size(), 1);
        EXPECT_EQ(soft->m_dependencies[0].m_kind, DependencyKind::kSoft);

        EXPECT_TRUE(DecodeFixture(AssetID("44444444-4444-4444-8444-444444444444")));
        EXPECT_TRUE(DecodeFixture(AssetID("66666666-6666-4666-8666-666666666666")));
        EXPECT_TRUE(DecodeFixture(AssetID("77777777-7777-4777-8777-777777777777")));
    }

    TEST(ArtifactStore, ResolvesDefaultAndStoreRelativePayloadSources)
    {
        const Path fixtureRoot = GetFixtureRoot();
        ArtifactStore::SetCatalogSource(fixtureRoot);
        const ArtifactDecodeResult simple = DecodeFixture(AssetID("11111111-1111-4111-8111-111111111111"));
        ASSERT_TRUE(simple);
        EXPECT_EQ(simple->m_payloads[0].m_chunks[0].m_checksum.m_current, 3019424693u);
        EXPECT_EQ(simple->m_payloads[0].m_chunks[0].m_compressedSize, 4);
        EXPECT_EQ(simple->m_payloads[0].m_resolvedDataSource.m_filePath,
                  ArtifactStore::ResolveData(ArtifactID("aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa")).m_filePath);

        const ArtifactDecodeResult diamond = DecodeFixture(AssetID("33333333-3333-4333-8333-333333333333"));
        ASSERT_TRUE(diamond);
        ASSERT_EQ(diamond->m_payloads.size(), 2);
        EXPECT_EQ(diamond->m_payloads[0].m_resolvedDataSource.m_filePath, NormalizePath(fixtureRoot / "payloads/diamond.bin"));
        EXPECT_EQ(diamond->m_payloads[0].m_resolvedDataSource.m_byteOffset, 8);
        EXPECT_EQ(diamond->m_payloads[1].m_resolvedDataSource.m_filePath,
                  NormalizePath(fixtureRoot / "payloads/diamond-lod.bin"));
    }


    TEST(ArtifactStore, UsesPlatformMetadataAndArtifactDataDirectories)
    {
        const Path fixtureRoot = GetFixtureRoot();
        ArtifactStore::SetCatalogSource(fixtureRoot);

        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        EXPECT_EQ(ArtifactStore::ResolveMeta(assetId).m_filePath,
                  NormalizePath(fixtureRoot / "artifacts/metadata/pc/11/11/11111111-1111-4111-8111-111111111111.meta"));

        const ArtifactID artifactId("aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa");
        EXPECT_EQ(ArtifactStore::ResolveData(artifactId).m_filePath,
                  NormalizePath(fixtureRoot / "artifacts/data/aa/aa/aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa.bin"));
    }

    TEST(ArtifactStore, AsynchronousMetadataReadFeedsStoreDecoder)
    {
        const Path fixtureRoot = GetFixtureRoot();
        ArtifactStore::SetCatalogSource(fixtureRoot);
        festd::pmr::vector<std::byte> bytes;
        Rc<WaitGroup> completion = WaitGroup::Create();
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        const Path source = ArtifactStore::ResolveMeta(assetId).m_filePath;
        ASSERT_TRUE(FileStream::Open(source, OpenMode::kReadOnly)) << source.data();
        Async::Batch batch(ResolvedDataSource{ source }, completion.Get());
        batch.ReadAppendToEnd(bytes);
        const Rc<Async::IController> controller = Async::Read(batch);
        completion->Wait();
        ASSERT_EQ(controller->GetStatus(), Async::Status::kSucceeded)
            << GetResultDesc(controller->GetLastOperationResult()).data();

        ArtifactResolutionContext context;
        context.m_assetId = assetId;
        context.m_metadataSource.m_filePath = source;
        const ArtifactDecodeResult decodeResult = ArtifactStore::Decode(bytes, context);
        ASSERT_TRUE(decodeResult);

        festd::pmr::vector<std::byte> payloadBytes;
        completion = WaitGroup::Create();
        const ArtifactPayloadRecord& payload = decodeResult->m_payloads[0];
        Async::Batch payloadBatch(payload.m_resolvedDataSource, completion.Get());
        const ArtifactChunkRecord& chunk = payload.m_chunks[0];
        payloadBatch.ReadAppend(payloadBytes,
                                chunk.m_compressionMethod,
                                chunk.m_compressedSize,
                                chunk.m_uncompressedSize,
                                chunk.m_offsetInPayload);
        const Rc<Async::IController> payloadController = Async::Read(payloadBatch);
        completion->Wait();
        ASSERT_EQ(payloadController->GetStatus(), Async::Status::kSucceeded);
        ASSERT_EQ(payloadBytes.size(), 4);
        EXPECT_EQ(Crc32::Compute(payloadBytes.data(), payloadBytes.size()), chunk.m_checksum.m_current);
    }

    TEST(ArtifactStore, ReportsStructuredInvalidMetadataErrors)
    {
        ArtifactStore::SetCatalogSource(GetFixtureRoot());
        ArtifactResolutionContext context;
        context.m_assetId = AssetID("11111111-1111-4111-8111-111111111111");
        context.m_metadataSource.m_filePath = GetFixtureRoot() / "malformed.json";
        const ArtifactDecodeResult malformed = ArtifactStore::Decode(ReadFixture("malformed.json"), context);
        ASSERT_FALSE(malformed);
        EXPECT_EQ(malformed.error().m_code, ArtifactMetadataErrorCode::kUnsupportedSchema);
        EXPECT_EQ(malformed.error().m_assetId, AssetID("11111111-1111-4111-8111-111111111111"));
        EXPECT_FALSE(malformed.error().m_source.empty());
    }

    TEST(ArtifactStore, MissingMetadataReadFailsWithoutBytesToDecode)
    {
        festd::pmr::vector<std::byte> bytes;
        Rc<WaitGroup> completion = WaitGroup::Create();
        Async::Batch batch(ResolvedDataSource{ GetFixtureRoot() / "missing.json" }, completion.Get());
        batch.ReadAppendToEnd(bytes);
        const Rc<Async::IController> controller = Async::Read(batch);
        completion->Wait();
        EXPECT_EQ(controller->GetStatus(), Async::Status::kFailed);
        EXPECT_TRUE(bytes.empty());
    }

    TEST(ArtifactStore, ValidatesAssetAndChunkBounds)
    {
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        ArtifactResolutionContext context;
        context.m_assetId = AssetID("22222222-2222-4222-8222-222222222222");
        context.m_metadataSource = ArtifactStore::ResolveMeta(assetId);
        const ArtifactDecodeResult assetResult = ArtifactStore::Decode(ReadSource(context.m_metadataSource.m_filePath), context);
        ASSERT_FALSE(assetResult);
        EXPECT_EQ(assetResult.error().m_code, ArtifactMetadataErrorCode::kIdentityMismatch);
    }
} // namespace FE::IO::Tests

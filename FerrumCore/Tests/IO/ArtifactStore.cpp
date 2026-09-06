#include <Core/IO/Artifact.h>
#include <Core/IO/Async.h>
#include <Core/IO/FileStream.h>
#include <Core/Jobs/WaitGroup.h>
#include <gtest/gtest.h>

namespace FE::IO::Tests
{
    namespace
    {
        constexpr festd::string_view kTypeId = "334f0750-1b4e-4f4c-ac6f-985382d4bd11";

        Path GetFixtureRoot()
        {
            return Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts";
        }

        festd::vector<std::byte> ReadFixture(const festd::string_view name)
        {
            const Path path = GetFixtureRoot() / name;
            const auto openResult = FileStream::Open(path, OpenMode::kReadOnly);
            EXPECT_TRUE(openResult.has_value());
            if (!openResult)
                return {};
            festd::vector<std::byte> bytes;
            bytes.resize(static_cast<uint32_t>(openResult.value()->Length()));
            EXPECT_EQ(openResult.value()->ReadToBuffer(bytes.data(), bytes.size()), bytes.size());
            return bytes;
        }

        ArtifactDecodeResult DecodeFixture(const festd::string_view name, const AssetID assetId)
        {
            const festd::vector<std::byte> bytes = ReadFixture(name);
            ArtifactResolutionContext context;
            context.m_assetId = assetId;
            context.m_metadataSource.m_filePath = GetFixtureRoot() / name;
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
                                   const uint32_t version = 1, const festd::string_view schema = "0x6172746966616374")
        {
            return Fmt::Format(
                R"({{"$type":"00000000-0000-0000-0000-000000000000","$version":{},"$schema":"{}","$value":{{"schema":"ferrum-artifact","platform":"windows-x64","asset":"11111111-1111-4111-8111-111111111111","artifact":"aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa","type":"{}","dependencies":[],"payloads":[{}]}}}})",
                version,
                schema,
                typeId,
                payload);
        }
    } // namespace

    TEST(ArtifactStore, DecodesHandAuthoredTypedDependencyFixtures)
    {
        ArtifactStore::SetCatalogSource(GetFixtureRoot());
        ArtifactStore::SetPlatform("windows-x64");
        const ArtifactDecodeResult chain = DecodeFixture("chain.json", AssetID("22222222-2222-4222-8222-222222222222"));
        ASSERT_TRUE(chain);
        ASSERT_EQ(chain->m_dependencies.size(), 1);
        EXPECT_EQ(chain->m_dependencies[0].m_kind, DependencyKind::kHard);
        EXPECT_EQ(chain->m_dependencies[0].m_expectedTypeId, Rtti::TypeID(festd::ascii_view{ kTypeId.data(), kTypeId.size() }));

        const ArtifactDecodeResult soft = DecodeFixture("soft.json", AssetID("88888888-8888-4888-8888-888888888888"));
        ASSERT_TRUE(soft);
        ASSERT_EQ(soft->m_dependencies.size(), 1);
        EXPECT_EQ(soft->m_dependencies[0].m_kind, DependencyKind::kSoft);
    }

    TEST(ArtifactStore, ResolvesDefaultAndStoreRelativePayloadSources)
    {
        const Path fixtureRoot = GetFixtureRoot();
        ArtifactStore::SetCatalogSource(fixtureRoot);
        const ArtifactDecodeResult simple = DecodeFixture("simple.json", AssetID("11111111-1111-4111-8111-111111111111"));
        ASSERT_TRUE(simple);
        EXPECT_EQ(simple->m_payloads[0].m_chunks[0].m_checksum.m_current, 3019424693u);
        EXPECT_EQ(simple->m_payloads[0].m_chunks[0].m_compressedSize, 4);
        EXPECT_EQ(simple->m_payloads[0].m_resolvedDataSource.m_filePath,
                  ArtifactStore::ResolveData(ArtifactID("aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa")).m_filePath);

        const ArtifactDecodeResult diamond = DecodeFixture("diamond.json", AssetID("33333333-3333-4333-8333-333333333333"));
        ASSERT_TRUE(diamond);
        ASSERT_EQ(diamond->m_payloads.size(), 2);
        EXPECT_EQ(diamond->m_payloads[0].m_resolvedDataSource.m_filePath, NormalizePath(fixtureRoot / "payloads/diamond.bin"));
        EXPECT_EQ(diamond->m_payloads[0].m_resolvedDataSource.m_byteOffset, 8);
        EXPECT_EQ(diamond->m_payloads[1].m_resolvedDataSource.m_filePath,
                  NormalizePath(fixtureRoot / "payloads/diamond-lod.bin"));
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
        const ArtifactDecodeResult malformed = DecodeFixture("malformed.json", AssetID("11111111-1111-4111-8111-111111111111"));
        ASSERT_FALSE(malformed);
        EXPECT_EQ(malformed.error().m_code, ArtifactMetadataErrorCode::kMissingField);
        EXPECT_EQ(malformed.error().m_assetId, AssetID("11111111-1111-4111-8111-111111111111"));
        EXPECT_FALSE(malformed.error().m_source.empty());
        EXPECT_FALSE(malformed.error().m_field.empty());
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

    TEST(ArtifactStore, RejectsInvalidPayloadLayouts)
    {
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        const festd::string rawSizeMismatch = MakeMetadata(
            R"({"offset":0,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":2,"compression":"none","checksum":0}]})");
        const ArtifactDecodeResult mismatchResult = DecodeText(rawSizeMismatch, assetId);
        ASSERT_FALSE(mismatchResult);
        EXPECT_EQ(mismatchResult.error().m_code, ArtifactMetadataErrorCode::kInvalidLayout);

        const festd::string oversizedOutput = MakeMetadata(
            R"({"offset":0,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":4294967296,"compression":"zstd","checksum":0}]})");
        const ArtifactDecodeResult oversizedResult = DecodeText(oversizedOutput, assetId);
        ASSERT_FALSE(oversizedResult);
        EXPECT_EQ(oversizedResult.error().m_code, ArtifactMetadataErrorCode::kLimitExceeded);

        const festd::string unsafeSource = MakeMetadata(
            R"({"source":"C:escape.bin","offset":0,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":1,"compression":"none","checksum":0}]})");
        const ArtifactDecodeResult unsafeResult = DecodeText(unsafeSource, assetId);
        ASSERT_FALSE(unsafeResult);
        EXPECT_EQ(unsafeResult.error().m_code, ArtifactMetadataErrorCode::kInvalidField);
    }

    TEST(ArtifactStore, RejectsSchemaTypeAndExpectedArtifactMismatch)
    {
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        ArtifactStore::SetPlatform("windows-x64");
        constexpr festd::string_view payload =
            R"({"offset":0,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":1,"compression":"none","checksum":0}]})";
        const ArtifactDecodeResult version = DecodeText(MakeMetadata(payload, kTypeId, 2), assetId);
        ASSERT_FALSE(version);
        EXPECT_EQ(version.error().m_code, ArtifactMetadataErrorCode::kUnsupportedSchema);
        const ArtifactDecodeResult schema = DecodeText(MakeMetadata(payload, kTypeId, 1, "0x0"), assetId);
        ASSERT_FALSE(schema);
        EXPECT_EQ(schema.error().m_code, ArtifactMetadataErrorCode::kUnsupportedSchema);
        const ArtifactDecodeResult type = DecodeText(MakeMetadata(payload, "12345678-1234-4234-8234-123456789012"), assetId);
        ASSERT_FALSE(type);
        EXPECT_EQ(type.error().m_code, ArtifactMetadataErrorCode::kUnknownType);

        ArtifactResolutionContext context;
        context.m_assetId = assetId;
        context.m_artifactId = ArtifactID("bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb");
        context.m_metadataSource.m_filePath = "expected-artifact.json";
        const ArtifactDecodeResult artifact = ArtifactStore::Decode(ReadFixture("simple.json"), context);
        ASSERT_FALSE(artifact);
        EXPECT_EQ(artifact.error().m_code, ArtifactMetadataErrorCode::kIdentityMismatch);
        EXPECT_EQ(artifact.error().m_source, context.m_metadataSource.m_filePath);
    }

    TEST(ArtifactStore, RejectsAggregateOutputAndPhysicalRangeOverflow)
    {
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        const festd::string aggregate = MakeMetadata(
            R"({"offset":0,"size":2,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":4294967295,"compression":"zstd","checksum":0},{"offset":1,"compressedSize":1,"uncompressedSize":1,"compression":"zstd","checksum":0}]})");
        const ArtifactDecodeResult aggregateResult = DecodeText(aggregate, assetId);
        ASSERT_FALSE(aggregateResult);
        EXPECT_EQ(aggregateResult.error().m_code, ArtifactMetadataErrorCode::kLimitExceeded);
        const festd::string range = MakeMetadata(
            R"({"offset":18446744073709551615,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":1,"compression":"none","checksum":0}]})");
        const ArtifactDecodeResult rangeResult = DecodeText(range, assetId);
        ASSERT_FALSE(rangeResult);
        EXPECT_EQ(rangeResult.error().m_code, ArtifactMetadataErrorCode::kInvalidLayout);
    }

    TEST(ArtifactStore, ValidatesPlatformAssetAndChunkBounds)
    {
        const AssetID assetId("11111111-1111-4111-8111-111111111111");
        ArtifactStore::SetPlatform("different-platform");
        const ArtifactDecodeResult platformResult = DecodeFixture("simple.json", assetId);
        ASSERT_FALSE(platformResult);
        EXPECT_EQ(platformResult.error().m_code, ArtifactMetadataErrorCode::kPlatformMismatch);
        ArtifactStore::SetPlatform("windows-x64");

        const ArtifactDecodeResult assetResult = DecodeFixture("simple.json", AssetID("22222222-2222-4222-8222-222222222222"));
        ASSERT_FALSE(assetResult);
        EXPECT_EQ(assetResult.error().m_code, ArtifactMetadataErrorCode::kIdentityMismatch);

        const festd::string unknownCompression = MakeMetadata(
            R"({"offset":0,"size":1,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":1,"compression":"brotli","checksum":0}]})");
        const ArtifactDecodeResult compressionResult = DecodeText(unknownCompression, assetId);
        ASSERT_FALSE(compressionResult);
        EXPECT_EQ(compressionResult.error().m_code, ArtifactMetadataErrorCode::kUnknownCompression);

        const festd::string uncoveredRange = MakeMetadata(
            R"({"offset":0,"size":2,"chunks":[{"offset":0,"compressedSize":1,"uncompressedSize":1,"compression":"none","checksum":0}]})");
        const ArtifactDecodeResult rangeResult = DecodeText(uncoveredRange, assetId);
        ASSERT_FALSE(rangeResult);
        EXPECT_EQ(rangeResult.error().m_code, ArtifactMetadataErrorCode::kInvalidLayout);
    }
} // namespace FE::IO::Tests

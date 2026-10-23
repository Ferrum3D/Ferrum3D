#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Core/ComputePipeline.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Shaders/Passes/Visibility/Contracts.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;


TEST(VisibilityContracts, PersistentInstanceAndPrimitiveBoundaries)
{
    const Visibility::VisibilityID first = Visibility::Encode(0, 0, 0);
    EXPECT_EQ(first.m_instance, 1u);
    EXPECT_EQ(first.m_primitive, 0u);

    const Visibility::VisibilityID last = Visibility::Encode(Visibility::kMaxInstanceRow,
                                                             Visibility::kMaxMeshletCount - 1,
                                                             Visibility::kMaxPrimitiveCount - 1,
                                                             15,
                                                             Visibility::kMaxLodCount - 1);
    EXPECT_EQ(last.m_instance, 0xffffffffu);
    EXPECT_EQ(last.m_primitive, 0xffffffffu);
    EXPECT_EQ(Visibility::DecodeLod(last.m_primitive), Visibility::kMaxLodCount - 1);
    EXPECT_EQ(Visibility::DecodeMeshlet(last.m_primitive), Visibility::kMaxMeshletCount - 1);
    EXPECT_EQ(Visibility::DecodePrimitive(last.m_primitive), Visibility::kMaxPrimitiveCount - 1);

    const Visibility::VisibilityID lod = Visibility::Encode(7, 5, 3, 0, 2);
    EXPECT_EQ(Visibility::DecodeLod(lod.m_primitive), 2u);
    EXPECT_EQ(Visibility::DecodeMeshlet(lod.m_primitive), 5u);
    EXPECT_EQ(Visibility::DecodePrimitive(lod.m_primitive), 3u);
}


TEST(VisibilityContracts, TypedClearPreservesIntegerBits)
{
    Core::RenderTargetLoadOperations operations;
    operations.ClearColor(0, Core::ColorClearValue::Uint(0xffffffffu, 0x81234567u));
    EXPECT_EQ(operations.m_colorClearValues[0].m_words[0], 0xffffffffu);
    EXPECT_EQ(operations.m_colorClearValues[0].m_words[1], 0x81234567u);
    EXPECT_EQ(operations.m_colorClearValues[0].m_words[2], 0u);

    operations.ClearColor(1, Core::ColorClearValue::Sint(-1, -2147483647));
    EXPECT_EQ(operations.m_colorClearValues[1].m_words[0], 0xffffffffu);
    EXPECT_EQ(operations.m_colorClearValues[1].m_words[1], 0x80000001u);

    const Core::ColorClearValue color = Core::ColorClearValue::Float(Color4F(1.0f, 0.0f, 0.5f, 1.0f));
    EXPECT_EQ(color.m_words[0], std::bit_cast<uint32_t>(1.0f));
    EXPECT_EQ(color.m_words[2], std::bit_cast<uint32_t>(0.5f));
}


TEST(VisibilityContracts, WaveLaneCountParticipatesInPipelineCacheKey)
{
    Core::ComputePipelineDesc desc;
    desc.SetComputeShader(Env::Name("VisibilityTest.cs"));
    const uint64_t defaultHash = desc.GetHash();
    desc.m_waveLaneCount = 32;
    const uint64_t wave32Hash = desc.GetHash();
    desc.m_waveLaneCount = 64;
    EXPECT_NE(defaultHash, wave32Hash);
    EXPECT_NE(defaultHash, desc.GetHash());
    EXPECT_NE(wave32Hash, desc.GetHash());
}

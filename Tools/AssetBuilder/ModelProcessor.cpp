#include "ModelProcessor.h"
#include "MeshOptimization.h"
#include "ModelImporter.h"
#include "Utils.h"

#include <FeCore/Compression/Compression.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Math/Packing.h>
#include <Graphics/Assets/ModelAssetFormat.h>
#include <Graphics/Core/InputLayoutBuilder.h>

namespace FE
{
    using namespace Graphics;

    namespace
    {
        festd::vector<std::byte> CompressVertices(const Core::InputStreamLayout& layout,
                                                  const festd::span<const AssetBuilder::IntermediateVertex> vertices)
        {
            using namespace AssetBuilder;

            FE_Assert(layout.CalculateActiveStreamMask() == 0x1);

            const uint32_t vertexCount = vertices.size();
            const uint32_t totalBytes = vertexCount * layout.CalculateStreamStride(0);

            festd::vector<std::byte> compressedData(totalBytes);
            Memory::BlockWriter writer{ compressedData };
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                const IntermediateVertex& vertex = vertices[vertexIndex];

                const PackedVector3F packedPosition{ vertex.m_position };
                const uint32_t packedUV = Math::Pack::RG32FloatToRG16Float(vertex.m_uv[0]);
                const uint32_t packedColor = Math::Pack::RGBA32FloatToRGBA8Unorm(Vector4(vertex.m_color[0]));
                const uint32_t packedNormal = Math::Pack::RGB32FloatToA2R10G10B10(vertex.m_normal * 0.5f + Vector3(0.5f));

                uint32_t packedTangent = Math::Pack::RGB32FloatToA2R10G10B10(Vector3{ vertex.m_tangentWithSign.m_simdVector });
                if (vertex.m_tangentWithSign.w < 0)
                    packedTangent |= (1 << 30);

                uint32_t boneSortIndices[Core::Limits::Vertex::kMaxInfluenceBones];
                festd::iota(boneSortIndices, boneSortIndices + festd::size(boneSortIndices), 0);
                festd::sort(boneSortIndices,
                            boneSortIndices + festd::size(boneSortIndices),
                            [&vertex](const uint32_t lhs, const uint32_t rhs) {
                                return vertex.m_influenceWeights[lhs] > vertex.m_influenceWeights[rhs];
                            });

                uint16_t sortedInfluenceBones[Core::Limits::Vertex::kMaxInfluenceBones];
                float sortedInfluenceWeights[Core::Limits::Vertex::kMaxInfluenceBones];
                for (uint32_t i = 0; i < festd::size(sortedInfluenceBones); ++i)
                {
                    sortedInfluenceBones[i] = vertex.m_influenceBones[boneSortIndices[i]];
                    sortedInfluenceWeights[i] = vertex.m_influenceWeights[boneSortIndices[i]];
                }

                // Check that weights sum to 1, so that we can throw away the last weight
                float weightsSum = 0.0f;
                for (const float weight : sortedInfluenceWeights)
                    weightsSum += weight;
                FE_Assert(Math::EqualEstimate(weightsSum, 1.0f, 0.001f) || weightsSum == 0.0f);

                const uint32_t packedInfluenceWeights =
                    Math::Pack::RGB32FloatToA2R10G10B10(Vector3::LoadUnaligned(sortedInfluenceWeights));

                writer.Write(packedPosition);
                writer.Write(packedUV);
                writer.Write(packedColor);
                writer.Write(packedNormal);
                writer.Write(packedTangent);
                writer.Write(packedInfluenceWeights);
                FE_Verify(writer.WriteBytes(sortedInfluenceBones, festd::size_bytes(sortedInfluenceBones)));
            }

            const uint32_t bytesWritten = static_cast<uint32_t>(writer.m_ptr - compressedData.data());
            FE_Assert(bytesWritten == totalBytes);

            return compressedData;
        }

        bool SaveModel(Logger* logger, const AssetBuilder::IntermediateModel& model,
                       const AssetBuilder::ModelProcessSettings& settings)
        {
            using namespace AssetBuilder;

            auto fileResult = settings.m_streamFactory->OpenFileStream(settings.m_outputFile, IO::OpenMode::kCreate);
            if (!fileResult)
            {
                settings.m_logger->LogError(
                    "Failed to open file '{}' for writing: {}", settings.m_outputFile, IO::GetResultDesc(fileResult.error()));
                return false;
            }

            const auto compressor = Compression::Compressor::Create(Compression::Method::kGDeflate);

            IO::IStream* out = fileResult->Get();

            CompressedBlockWriter writer{ out, &compressor };

            // Write all headers to the first block

            const uint32_t lodCount = model.m_meshes[0]->m_lods.size();

            Data::ModelHeader header;
            header.m_magic = Data::kModelMagic;
            header.m_meshCount = model.m_meshes.size();
            header.m_lodCount = lodCount;
            writer.Write(header);

            Core::InputLayoutBuilder inputLayoutBuilder;
            inputLayoutBuilder.AddStream(Core::InputStreamRate::kPerVertex)
                .AddChannel(Core::VertexChannelFormat::kR32G32B32_SFLOAT, Core::ShaderSemantic::kPosition)
                .AddChannel(Core::VertexChannelFormat::kR16G16_SFLOAT, Core::ShaderSemantic::kTexCoord)
                .AddChannel(Core::VertexChannelFormat::kR8G8B8A8_UNORM, Core::ShaderSemantic::kColor)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kNormal)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kTangent)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kBlendWeight)
                .AddChannel(Core::VertexChannelFormat::kR16G16B16A16_UINT, Core::ShaderSemantic::kBlendIndices);

            const Core::InputStreamLayout layout = inputLayoutBuilder.Build();
            FE_Assert(layout.CalculateTotalStride() % sizeof(uint32_t) == 0);

            logger->LogInfo("Compressing model '{}'", model.m_name);

            for (const IntermediateMesh* mesh : model.m_meshes)
            {
                Core::MeshInfo meshHeader;
                meshHeader.m_layout = layout;
                writer.Write(meshHeader);

                FE_Assert(mesh->m_lods.size() == lodCount, "All meshes must have the same number of LODs");

                for (const IntermediateMeshLod& lod : mesh->m_lods)
                {
                    Core::MeshLodInfo lodHeader;
                    lodHeader.m_vertexCount = lod.m_vertices.size();
                    lodHeader.m_indexCount = lod.m_indices.size();
                    lodHeader.m_meshletCount = lod.m_meshlets.size();
                    lodHeader.m_primitiveCount = lod.m_primitives.size();
                    writer.Write(lodHeader);
                }
            }

            FE_Assert(model.m_lodErrors.size() == header.m_lodCount - 1);
            writer.WriteBytes(model.m_lodErrors.data(), festd::size_bytes(model.m_lodErrors));
            writer.Flush();

            for (uint32_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                logger->LogInfo("Compressing data for LOD {}", lodCount - lodIndex - 1);

                for (uint32_t meshIndex = 0; meshIndex < model.m_meshes.size(); ++meshIndex)
                {
                    IntermediateMesh* mesh = model.m_meshes[meshIndex];
                    logger->LogInfo("  Mesh [{}/{}]", meshIndex + 1, model.m_meshes.size());

                    IntermediateMeshLod& lod = mesh->m_lods[lodIndex];

                    const auto vertexData = CompressVertices(layout, lod.m_vertices);
                    writer.WriteBytes(vertexData.data(), vertexData.size());
                    writer.WriteBytes(lod.m_indices.data(), festd::size_bytes(lod.m_indices));
                    writer.WriteBytes(lod.m_meshlets.data(), festd::size_bytes(lod.m_meshlets));
                    writer.WriteBytes(lod.m_primitives.data(), festd::size_bytes(lod.m_primitives));
                }

                // We need the LODs to be stored in separate blocks
                writer.Flush();
            }

            logger->LogInfo("Finished compressing '{}'", model.m_name);
            return true;
        }
    } // namespace


    bool AssetBuilder::ProcessModel(const ModelProcessSettings& settings)
    {
        auto fileResult = settings.m_streamFactory->OpenFileStream(settings.m_inputFile, IO::OpenMode::kReadOnly);
        if (!fileResult)
        {
            settings.m_logger->LogError(
                "Failed to open file {}: {}", settings.m_inputFile, IO::GetResultDesc(fileResult.error()));
            return false;
        }

        settings.m_logger->LogInfo("Processing model '{}'", settings.m_inputFile);

        IO::IStream* file = fileResult->Get();

        const size_t rawSize = file->Length();
        void* rawData = Memory::DefaultAllocate(rawSize);
        if (file->ReadToBuffer(rawData, rawSize) != rawSize)
        {
            Memory::DefaultFree(rawData);
            settings.m_logger->LogError("Failed to read file '{}'", settings.m_inputFile);
            return false;
        }

        fileResult->Reset();
        file = nullptr;

        settings.m_logger->LogInfo("Importing model '{}'", settings.m_inputFile);

        auto importer = ModelImporter::Create(settings.m_logger, rawData, static_cast<uint32_t>(rawSize));
        Memory::DefaultFree(rawData);
        if (!importer)
            return false;

        IntermediateScene* scene = importer.ParseScene();

        settings.m_logger->LogInfo("Optimizing meshes");
        scene->ForEachMesh(MeshOptimizationPasses::Remap);
        scene->ForEachMesh(MeshOptimizationPasses::OptimizeVertexCache);
        scene->ForEachMesh(MeshOptimizationPasses::OptimizeOverdraw);
        //scene->ForEachMesh(MeshOptimizationPasses::OptimizeVertexFetch);

        settings.m_logger->LogInfo("Generating LODs");
        scene->ForEachModel(MeshOptimizationPasses::GenerateLods);
        scene->ForEachMesh(MeshOptimizationPasses::GenerateMeshlets);

        return SaveModel(settings.m_logger, scene->m_models[0], settings);
    }
} // namespace FE

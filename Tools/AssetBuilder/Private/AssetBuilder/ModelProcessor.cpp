#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/MeshOptimization.h>
#include <AssetBuilder/ModelImporter.h>
#include <AssetBuilder/ModelProcessor.h>

#include <Core/Math/Packing.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Core/InputLayoutBuilder.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;

    namespace
    {
        Core::InputStreamLayout BuildInputLayout()
        {
            Core::InputLayoutBuilder builder;
            builder.AddStream(Core::InputStreamRate::kPerVertex)
                .AddChannel(Core::VertexChannelFormat::kR32G32B32_SFLOAT, Core::ShaderSemantic::kPosition)
                .AddChannel(Core::VertexChannelFormat::kR16G16_SFLOAT, Core::ShaderSemantic::kTexCoord)
                .AddChannel(Core::VertexChannelFormat::kR8G8B8A8_UNORM, Core::ShaderSemantic::kColor)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kNormal)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kTangent)
                .AddChannel(Core::VertexChannelFormat::kA2R10G10B10_UNORM, Core::ShaderSemantic::kBlendWeight)
                .AddChannel(Core::VertexChannelFormat::kR16G16B16A16_UINT, Core::ShaderSemantic::kBlendIndices);
            return builder.Build();
        }


        festd::vector<std::byte> PackVertices(const Core::InputStreamLayout& layout,
                                              const festd::span<const IntermediateVertex> vertices)
        {
            const uint32_t totalBytes = vertices.size() * layout.CalculateStreamStride(0);
            festd::vector<std::byte> result(totalBytes);
            Memory::BlockWriter writer(result);
            for (const IntermediateVertex& vertex : vertices)
            {
                const PackedVector3F packedPosition(vertex.m_position);
                const uint32_t packedUv = Math::Pack::RG32FloatToRG16Float(vertex.m_uv[0]);
                const uint32_t packedColor = Math::Pack::RGBA32FloatToRGBA8Unorm(Vector4(vertex.m_color[0]));
                const uint32_t packedNormal =
                    Math::Pack::RGBA32FloatToA2R10G10B10Unorm(Vector4(vertex.m_normal * 0.5f + Vector3(0.5f), 0.0f));

                Vector4 tangentWithSign = vertex.m_tangentWithSign;
                tangentWithSign.w = tangentWithSign.w < 0 ? 1.0f : 0.0f;
                const uint32_t packedTangent = Math::Pack::RGBA32FloatToA2R10G10B10Unorm(tangentWithSign);

                uint32_t boneSortIndices[Core::Limits::Vertex::kMaxInfluenceBones];
                festd::iota(boneSortIndices, boneSortIndices + festd::size(boneSortIndices), 0);
                festd::sort(boneSortIndices,
                            boneSortIndices + festd::size(boneSortIndices),
                            [&vertex](const uint32_t lhs, const uint32_t rhs) {
                                return vertex.m_influenceWeights[lhs] > vertex.m_influenceWeights[rhs];
                            });

                uint16_t sortedInfluenceBones[Core::Limits::Vertex::kMaxInfluenceBones];
                float sortedInfluenceWeights[Core::Limits::Vertex::kMaxInfluenceBones];
                for (uint32_t index = 0; index < festd::size(sortedInfluenceBones); ++index)
                {
                    sortedInfluenceBones[index] = vertex.m_influenceBones[boneSortIndices[index]];
                    sortedInfluenceWeights[index] = vertex.m_influenceWeights[boneSortIndices[index]];
                }

                const uint32_t packedInfluenceWeights =
                    Math::Pack::RGBA32FloatToA2R10G10B10Unorm(Vector4(Vector3::LoadUnaligned(sortedInfluenceWeights), 0.0f));

                writer.Write(packedPosition);
                writer.Write(packedUv);
                writer.Write(packedColor);
                writer.Write(packedNormal);
                writer.Write(packedTangent);
                writer.Write(packedInfluenceWeights);
                FE_Verify(writer.WriteBytes(sortedInfluenceBones, festd::size_bytes(sortedInfluenceBones)));
            }
            FE_Assert(writer.m_ptr == result.data() + result.size());
            return result;
        }


        void AppendBytes(festd::vector<std::byte>& destination, const void* data, const size_t size)
        {
            const uint32_t oldSize = destination.size();
            destination.resize(oldSize + static_cast<uint32_t>(size));
            memcpy(destination.data() + oldSize, data, size);
        }


        bool WriteMesh(const IntermediateModel& model, const Core::InputStreamLayout& layout, const MeshProcessSettings& settings)
        {
            MeshAsset header;
            header.m_vertexStride = layout.CalculateStreamStride(0);
            header.m_lodErrors.assign(model.m_lodErrors.begin(), model.m_lodErrors.end());
            for (const IntermediateMesh* submesh : model.m_meshes)
            {
                MeshSubmeshAssetInfo& submeshInfo = header.m_submeshes.emplace_back();
                for (const IntermediateMeshLod& lod : submesh->m_lods)
                {
                    MeshLodAssetInfo& lodInfo = submeshInfo.m_lods.emplace_back();
                    lodInfo.m_vertexCount = lod.m_vertices.size();
                    lodInfo.m_indexCount = lod.m_indices.size();
                    lodInfo.m_meshletCount = lod.m_meshlets.size();
                    lodInfo.m_primitiveCount = lod.m_primitives.size();
                }
            }

            ArtifactWriter writer(settings.m_outputDirectory,
                                  settings.m_assetId,
                                  settings.m_artifactId,
                                  Rtti::GetTypeID<MeshAsset>());
            if (!writer.WriteHeader(header))
                return false;

            const uint32_t lodCount = model.m_meshes.front()->m_lods.size();
            for (uint32_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                festd::vector<std::byte> payload;
                for (const IntermediateMesh* submesh : model.m_meshes)
                {
                    const IntermediateMeshLod& lod = submesh->m_lods[lodIndex];
                    const festd::vector<std::byte> vertices = PackVertices(layout, lod.m_vertices);
                    AppendBytes(payload, vertices.data(), vertices.size());
                    AppendBytes(payload, lod.m_indices.data(), festd::size_bytes(lod.m_indices));
                    AppendBytes(payload, lod.m_meshlets.data(), festd::size_bytes(lod.m_meshlets));
                    AppendBytes(payload, lod.m_primitives.data(), festd::size_bytes(lod.m_primitives));
                }

                if (!writer.WritePayload(payload))
                    return false;
            }
            return writer.Finish();
        }
    } // namespace


    bool ProcessModel(const ModelProcessSettings& settings)
    {
        ModelAsset header;
        ArtifactWriter writer(settings.m_outputDirectory,
                              settings.m_assetId,
                              settings.m_artifactId,
                              Rtti::GetTypeID<ModelAsset>());
        for (const AssetFileDependency& dependency : settings.m_dependencies)
        {
            if (dependency.m_expectedTypeId == Rtti::GetTypeID<MeshAsset>())
                header.m_meshes.emplace_back(dependency.m_assetId);
            else if (dependency.m_expectedTypeId == Rtti::GetTypeID<TextureAsset>())
                header.m_textures.emplace_back(dependency.m_assetId);

            writer.AddDependency(dependency.m_assetId, dependency.m_expectedTypeId, dependency.m_kind);
        }

        return writer.WriteHeader(header) && writer.Finish();
    }


    bool ProcessMesh(const MeshProcessSettings& settings)
    {
        ModelImporter importer =
            ModelImporter::Create(settings.m_sourceData.data(), settings.m_sourceData.size(), settings.m_inputFile);
        if (!importer)
            return false;

        IntermediateScene* scene = importer.ParseScene();
        const auto deferDeleteScene = festd::defer([scene] {
            Memory::DefaultDelete(scene);
        });
        if (settings.m_sourceObjectIndex >= scene->m_models.size())
        {
            Logger::LogError("Model '{}' does not contain mesh product {}", settings.m_inputFile, settings.m_sourceObjectIndex);
            return false;
        }

        IntermediateModel* model = &scene->m_models[settings.m_sourceObjectIndex];
        if (model->m_meshes.empty())
        {
            Logger::LogError("Model '{}' mesh product {} contains no meshes", settings.m_inputFile, settings.m_sourceObjectIndex);
            return false;
        }

        for (IntermediateMesh* mesh : model->m_meshes)
        {
            MeshOptimizationPasses::Remap(mesh);
            MeshOptimizationPasses::OptimizeVertexCache(mesh);
            MeshOptimizationPasses::OptimizeOverdraw(mesh);
        }
        if (settings.m_generateLods)
            MeshOptimizationPasses::GenerateLods(model);
        for (IntermediateMesh* mesh : model->m_meshes)
            MeshOptimizationPasses::GenerateMeshlets(mesh);

        const Core::InputStreamLayout layout = BuildInputLayout();
        return WriteMesh(*model, layout, settings);
    }
} // namespace FE::AssetBuilder

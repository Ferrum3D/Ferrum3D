#include "MeshOptimization.h"
#include "ModelImporter.h"

#include <meshoptimizer.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;

    namespace
    {
        uint32_t RemapImpl(festd::vector<uint32_t>& remap, const festd::span<const uint32_t> indices,
                           const festd::span<const IntermediateVertex> vertices)
        {
            remap.resize(vertices.size());
            const size_t newVertexCount =
                meshopt_generateVertexRemapCustom(remap.data(),
                                                  indices.data(),
                                                  indices.size(),
                                                  vertices[0].m_position.Data(),
                                                  vertices.size(),
                                                  sizeof(IntermediateVertex),
                                                  [vertices](const uint32_t leftIndex, const uint32_t rightIndex) {
                                                      return vertices[leftIndex] == vertices[rightIndex];
                                                  });

            return static_cast<uint32_t>(newVertexCount);
        }
    } // namespace


    void MeshOptimizationPasses::Remap(IntermediateMesh* mesh)
    {
        auto& lod = festd::single(mesh->m_lods);
        festd::vector<uint32_t> remap;

        const uint32_t newVertexCount = RemapImpl(remap, lod.m_indices, lod.m_vertices);

        festd::vector<uint32_t> newIndices(lod.m_indices.size());
        meshopt_remapIndexBuffer(newIndices.data(), lod.m_indices.data(), lod.m_indices.size(), remap.data());
        lod.m_indices = std::move(newIndices);

        festd::vector<IntermediateVertex> newVertices(newVertexCount);
        meshopt_remapVertexBuffer(
            newVertices.data(), lod.m_vertices.data(), lod.m_vertices.size(), sizeof(IntermediateVertex), remap.data());
        lod.m_vertices = std::move(newVertices);
    }


    void MeshOptimizationPasses::OptimizeVertexCache(IntermediateMesh* mesh)
    {
        auto& lod = festd::single(mesh->m_lods);
        meshopt_optimizeVertexCache(lod.m_indices.data(), lod.m_indices.data(), lod.m_indices.size(), lod.m_vertices.size());
    }


    void MeshOptimizationPasses::OptimizeOverdraw(IntermediateMesh* mesh)
    {
        auto& lod = festd::single(mesh->m_lods);
        meshopt_optimizeOverdraw(lod.m_indices.data(),
                                 lod.m_indices.data(),
                                 lod.m_indices.size(),
                                 lod.m_vertices[0].m_position.Data(),
                                 lod.m_vertices.size(),
                                 sizeof(IntermediateVertex),
                                 1.05f);
    }


    void MeshOptimizationPasses::OptimizeVertexFetch(IntermediateMesh* mesh)
    {
        auto& lod = festd::single(mesh->m_lods);
        meshopt_optimizeVertexFetch(lod.m_indices.data(),
                                    lod.m_indices.data(),
                                    lod.m_indices.size(),
                                    lod.m_vertices.data(),
                                    lod.m_vertices.size(),
                                    sizeof(IntermediateVertex));
    }


    void MeshOptimizationPasses::GenerateLods(IntermediateModel* model)
    {
        festd::inline_vector<float> lodErrors;
        SegmentedVector<festd::vector<uint32_t>> lods;

        // Merge all meshes into a single mesh before generating LODs.

        uint32_t totalVertexCount = 0;
        uint32_t totalIndexCount = 0;
        for (IntermediateMesh* mesh : model->m_meshes)
        {
            auto& lod0 = festd::single(mesh->m_lods);
            totalVertexCount += lod0.m_vertices.size();
            totalIndexCount += lod0.m_indices.size();
        }

        uint32_t vertexCopyOffset = 0;
        uint32_t indexCopyOffset = 0;
        festd::vector<IntermediateVertex> vertices(totalVertexCount);
        festd::vector<uint32_t> indices(totalIndexCount);
        festd::inline_vector<Vector2UInt> indexRanges;
        indexRanges.reserve(model->m_meshes.size());
        for (IntermediateMesh* mesh : model->m_meshes)
        {
            auto& lod0 = festd::single(mesh->m_lods);
            memcpy(vertices.data() + vertexCopyOffset, lod0.m_vertices.data(), festd::size_bytes(lod0.m_vertices));
            for (uint32_t i = indexCopyOffset; i < indexCopyOffset + lod0.m_indices.size(); ++i)
                indices[i] = lod0.m_indices[i] + vertexCopyOffset;

            indexRanges.push_back({ vertexCopyOffset, vertexCopyOffset + lod0.m_vertices.size() });

            vertexCopyOffset += lod0.m_vertices.size();
            indexCopyOffset += lod0.m_indices.size();
        }

        const uint32_t initialIndexCount = indices.size() / 2;
        for (uint32_t targetIndexCount = initialIndexCount; targetIndexCount > 1000; targetIndexCount /= 2)
        {
            festd::vector<uint32_t>& lodIndices = lods.push_back();
            lodIndices.resize(indices.size());

            float lodError = 0.0f;
            const size_t lodIndexCount = meshopt_simplify(lodIndices.data(),
                                                          indices.data(),
                                                          indices.size(),
                                                          vertices[0].m_position.Data(),
                                                          vertices.size(),
                                                          sizeof(IntermediateVertex),
                                                          targetIndexCount,
                                                          Constants::kMaxFloat,
                                                          meshopt_SimplifyLockBorder,
                                                          &lodError);

            lodErrors.push_back(lodError);
            lodIndices.resize(static_cast<uint32_t>(lodIndexCount));
        }

        std::vector<IntermediateMeshLod> allLod0(model->m_meshes.size());
        for (uint32_t meshIndex = 0; meshIndex < model->m_meshes.size(); ++meshIndex)
        {
            auto& meshLods = model->m_meshes[meshIndex]->m_lods;
            std::swap(allLod0[meshIndex], meshLods[0]);
            meshLods.clear();
        }

        // We need to store the least detailed LODs first, because that is the order in which they will be loaded.
        model->m_lodErrors.reserve(lodErrors.size());
        for (int32_t i = static_cast<int32_t>(lodErrors.size()) - 1; i >= 0; --i)
        {
            model->m_lodErrors.push_back(lodErrors[i]);

            // Split each LOD back into the original meshes.
            for (uint32_t meshIndex = 0; meshIndex < model->m_meshes.size(); ++meshIndex)
            {
                IntermediateMesh* mesh = model->m_meshes[meshIndex];
                const Vector2UInt range = indexRanges[meshIndex];

                festd::vector<uint32_t> meshLodIndices;
                for (const uint32_t index : lods[i])
                {
                    if (index >= range.x && index < range.y)
                        meshLodIndices.push_back(index - range.x);
                }

                const auto& lod0Vertices = allLod0[meshIndex].m_vertices;

                festd::vector<uint32_t> remap;
                const uint32_t newVertexCount = RemapImpl(remap, meshLodIndices, lod0Vertices);

                festd::vector<uint32_t> newIndices(meshLodIndices.size());
                meshopt_remapIndexBuffer(newIndices.data(), meshLodIndices.data(), meshLodIndices.size(), remap.data());

                festd::vector<IntermediateVertex> newVertices(newVertexCount);
                meshopt_remapVertexBuffer(
                    newVertices.data(), lod0Vertices.data(), lod0Vertices.size(), sizeof(IntermediateVertex), remap.data());

                IntermediateMeshLod& lod = mesh->m_lods.push_back();
                lod.m_indices = std::move(newIndices);
                lod.m_vertices = std::move(newVertices);
            }
        }

        for (uint32_t meshIndex = 0; meshIndex < model->m_meshes.size(); ++meshIndex)
        {
            IntermediateMesh* mesh = model->m_meshes[meshIndex];
            mesh->m_lods.push_back(std::move(allLod0[meshIndex]));
        }
    }


    namespace
    {
        void GenerateMeshletsImpl(festd::vector<uint32_t>& indices, festd::vector<IntermediateVertex>& vertices,
                                  festd::vector<Core::MeshletHeader>& meshlets, festd::vector<Core::PackedTriangle>& primitives)
        {
            namespace Limits = Core::Limits::Mesh;

            const size_t maxMeshlets =
                meshopt_buildMeshletsBound(indices.size(), Limits::kMaxMeshletVertexCount, Limits::kMaxMeshletPrimitiveCount);

            festd::vector<meshopt_Meshlet> tempMeshlets(static_cast<uint32_t>(maxMeshlets));
            festd::vector<uint32_t> meshletIndices(indices.size());
            festd::vector<uint8_t> meshletTriangles(indices.size());

            const size_t meshletCount = meshopt_buildMeshlets(tempMeshlets.data(),
                                                              meshletIndices.data(),
                                                              meshletTriangles.data(),
                                                              indices.data(),
                                                              indices.size(),
                                                              vertices[0].m_position.Data(),
                                                              vertices.size(),
                                                              sizeof(IntermediateVertex),
                                                              Limits::kMaxMeshletVertexCount,
                                                              Limits::kMaxMeshletPrimitiveCount,
                                                              0.3f);

            const meshopt_Meshlet& lastMeshlet = tempMeshlets[static_cast<uint32_t>(meshletCount - 1)];
            meshletIndices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
            meshletTriangles.resize(lastMeshlet.triangle_offset + lastMeshlet.triangle_count * 3);
            tempMeshlets.resize(static_cast<uint32_t>(meshletCount));

            meshlets.reserve(tempMeshlets.size());
            for (const meshopt_Meshlet& meshlet : tempMeshlets)
            {
                const Core::MeshletHeader meshletHeader = Core::MeshletHeader::Pack(
                    meshlet.vertex_count, meshlet.vertex_offset, meshlet.triangle_count, meshlet.triangle_offset / 3);
                meshlets.push_back(meshletHeader);
            }

            FE_Assert(meshletTriangles.size() % 3 == 0);
            primitives.reserve(meshletTriangles.size() / 3);
            for (uint32_t i = 0; i < meshletTriangles.size(); i += 3)
            {
                const Core::PackedTriangle triangle =
                    Core::PackedTriangle::Pack(meshletTriangles[i], meshletTriangles[i + 1], meshletTriangles[i + 2]);
                primitives.push_back(triangle);
            }

            indices = std::move(meshletIndices);
        }
    } // namespace


    void MeshOptimizationPasses::GenerateMeshlets(IntermediateMesh* mesh)
    {
        for (IntermediateMeshLod& lod : mesh->m_lods)
            GenerateMeshletsImpl(lod.m_indices, lod.m_vertices, lod.m_meshlets, lod.m_primitives);
    }
} // namespace FE::AssetBuilder

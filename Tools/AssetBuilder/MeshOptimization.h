#pragma once
#include "ModelImporter.h"

namespace FE::AssetBuilder
{
    struct IntermediateMesh;

    namespace MeshOptimizationPasses
    {
        void Remap(IntermediateMesh* mesh);
        void OptimizeVertexCache(IntermediateMesh* mesh);
        void OptimizeOverdraw(IntermediateMesh* mesh);
        void OptimizeVertexFetch(IntermediateMesh* mesh);
        void GenerateLods(IntermediateModel* model);
        void GenerateMeshlets(IntermediateMesh* mesh);
    } // namespace MeshOptimizationPasses
} // namespace FE::AssetBuilder

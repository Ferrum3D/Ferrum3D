#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Math/Color.h>
#include <FeCore/Math/Quaternion.h>
#include <FeCore/Math/Transform.h>
#include <FeCore/Math/Vector2.h>
#include <FeCore/Math/Vector3.h>
#include <FeCore/Math/Vector4.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Base/Limits.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Meshlet.h>
#include <festd/vector.h>

namespace FE::AssetBuilder
{
    struct IntermediateScene;


    struct IntermediateVertex final
    {
        Vector3 m_position;
        Vector3 m_normal;
        Vector2 m_uv[Graphics::Core::Limits::Vertex::kMaxTexCoords];
        Color4F m_color[Graphics::Core::Limits::Vertex::kMaxColors];
        Vector4 m_tangentWithSign;
        uint16_t m_influenceBones[Graphics::Core::Limits::Vertex::kMaxInfluenceBones];
        float m_influenceWeights[Graphics::Core::Limits::Vertex::kMaxInfluenceBones];

        FE_FORCE_INLINE friend bool operator==(const IntermediateVertex& lhs, const IntermediateVertex& rhs)
        {
            if (lhs.m_position != rhs.m_position)
                return false;

            if (lhs.m_normal != rhs.m_normal)
                return false;

            return memcmp(&lhs.m_uv[0], &rhs.m_uv[0], sizeof(IntermediateVertex) - offsetof(IntermediateVertex, m_uv)) == 0;
        }

        FE_FORCE_INLINE friend bool operator!=(const IntermediateVertex& lhs, const IntermediateVertex& rhs)
        {
            return !(lhs == rhs);
        }
    };


    struct IntermediateMaterial final
    {
    };


    struct IntermediateMeshLod final
    {
        festd::vector<IntermediateVertex> m_vertices;
        festd::vector<uint32_t> m_indices;
        festd::vector<Graphics::Core::MeshletHeader> m_meshlets;
        festd::vector<Graphics::Core::PackedTriangle> m_primitives;
    };


    struct IntermediateMesh final
    {
        IntermediateMaterial* m_material = nullptr;
        SegmentedVector<IntermediateMeshLod> m_lods;
    };


    struct IntermediateModel final
    {
        Env::Name m_name;
        festd::inline_vector<IntermediateMesh*> m_meshes;
        festd::inline_vector<float> m_lodErrors;
    };


    struct IntermediateSceneNode final
    {
        Env::Name m_name;
        IntermediateSceneNode* m_parent = nullptr;
        festd::inline_vector<IntermediateSceneNode*> m_children;
        IntermediateModel* m_model = nullptr;
        Transform m_transform;
        Vector3 m_nonUniformScale;

        void Invalidate(IntermediateScene& scene);
    };


    struct IntermediateScene final
    {
        festd::vector<IntermediateSceneNode*> m_immediateNodes;
        SegmentedVector<IntermediateMaterial> m_materials;
        SegmentedVector<IntermediateMesh> m_meshes;
        SegmentedVector<IntermediateModel> m_models;

        Memory::Pool<IntermediateSceneNode> m_nodePool{ "IntermediateSceneNodePool" };

        IntermediateScene() = default;
        ~IntermediateScene();

        template<class TFunctor>
        void ForEachMesh(TFunctor&& functor)
        {
            for (IntermediateModel& model : m_models)
            {
                for (IntermediateMesh* mesh : model.m_meshes)
                    functor(mesh);
            }
        }

        template<class TFunctor>
        void ForEachModel(TFunctor&& functor)
        {
            for (IntermediateModel& model : m_models)
                functor(&model);
        }
    };


    struct ModelImporter final
    {
        ~ModelImporter();

        ModelImporter(const ModelImporter&) = delete;
        ModelImporter& operator=(const ModelImporter&) = delete;

        ModelImporter(ModelImporter&& other) noexcept;
        ModelImporter& operator=(ModelImporter&& other) noexcept;

        [[nodiscard]] static ModelImporter Create(Logger* logger, const void* data, uint32_t byteSize);

        [[nodiscard]] IntermediateScene* ParseScene();

        explicit operator bool() const
        {
            return m_impl != nullptr;
        }

    private:
        ModelImporter() = default;

        struct Implementation;
        Implementation* m_impl = nullptr;
    };
} // namespace FE::AssetBuilder

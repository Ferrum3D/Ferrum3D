#include "ModelImporter.h"

#include <FeCore/IO/Path.h>
#include <FeCore/Strings/Parser.h>

#define STBIW_MALLOC(size) FE::Memory::DefaultAllocate(size)
#define STBIW_REALLOC(p, newSize) FE::Memory::DefaultReallocate(p, newSize)
#define STBIW_FREE(p) FE::Memory::DefaultFree(p)

#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYGLTF_NO_INCLUDE_RAPIDJSON
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>


using namespace FE::Graphics;

namespace FE::AssetBuilder
{
    namespace
    {
        uint32_t GetVertexAttributeOffset(const festd::string_view attributeName)
        {
            if (attributeName == "POSITION")
                return offsetof(IntermediateVertex, m_position);

            if (attributeName == "NORMAL")
                return offsetof(IntermediateVertex, m_normal);

            if (attributeName.starts_with("TEXCOORD_"))
            {
                const uint32_t index = Parser::Parse<uint32_t>(attributeName.substr_ascii(sizeof("TEXCOORD_") - 1));
                FE_Assert(index < Graphics::Core::Limits::Vertex::kMaxTexCoords);
                return offsetof(IntermediateVertex, m_uv[index]);
            }

            if (attributeName.starts_with("COLOR_"))
            {
                const uint32_t index = Parser::Parse<uint32_t>(attributeName.substr_ascii(sizeof("COLOR_") - 1));
                FE_Assert(index < Graphics::Core::Limits::Vertex::kMaxColors);
                return offsetof(IntermediateVertex, m_color[index]);
            }

            return kInvalidIndex;
        }


        IntermediateModel* ParseModel(IntermediateScene* intermediateScene, const tinygltf::Model& model, const int32_t meshIndex)
        {
            const tinygltf::Mesh& mesh = model.meshes[meshIndex];

            IntermediateModel& intermediateModel = intermediateScene->m_models.push_back();
            intermediateModel.m_name = Env::Name(mesh.name);
            intermediateModel.m_meshes.reserve(static_cast<uint32_t>(mesh.primitives.size()));

            for (const tinygltf::Primitive& primitive : mesh.primitives)
            {
                const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];

                IntermediateMesh& intermediateMesh = intermediateScene->m_meshes.push_back();
                intermediateModel.m_meshes.push_back(&intermediateMesh);

                IntermediateMeshLod& lod0 = intermediateMesh.m_lods.push_back();
                lod0.m_vertices.resize(static_cast<uint32_t>(positionAccessor.count));

                for (const auto& [attributeName, accessorIndex] : primitive.attributes)
                {
                    const uint32_t attributeOffset = GetVertexAttributeOffset(festd::string_view(attributeName));
                    if (attributeOffset == kInvalidIndex)
                        continue;

                    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
                    FE_Assert(accessor.count == positionAccessor.count);

                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    const uint8_t* bufferData = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                    const uint32_t attributeByteSize = accessor.ByteStride(bufferView);
                    FE_Assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    for (uint32_t vertexIndex = 0; vertexIndex < lod0.m_vertices.size(); ++vertexIndex)
                    {
                        void* vertexData = &lod0.m_vertices[vertexIndex];
                        memcpy(static_cast<std::byte*>(vertexData) + attributeOffset,
                               bufferData + static_cast<size_t>(vertexIndex) * attributeByteSize,
                               attributeByteSize);
                    }
                }

                if (primitive.indices >= 0)
                {
                    const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferView = model.bufferViews[indicesAccessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    const void* bufferData = &buffer.data[indicesAccessor.byteOffset + bufferView.byteOffset];
                    lod0.m_indices.resize(static_cast<uint32_t>(indicesAccessor.count));

                    switch (indicesAccessor.componentType)
                    {
                    default:
                        FE_DebugBreak();
                        [[fallthrough]];

                    case TINYGLTF_COMPONENT_TYPE_BYTE:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = static_cast<uint32_t>(static_cast<const uint8_t*>(bufferData)[i]);
                        break;

                    case TINYGLTF_COMPONENT_TYPE_SHORT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = static_cast<uint32_t>(static_cast<const uint16_t*>(bufferData)[i]);
                        break;

                    case TINYGLTF_COMPONENT_TYPE_INT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = static_cast<const uint32_t*>(bufferData)[i];
                        break;
                    }
                }
                else
                {
                    lod0.m_indices.resize(lod0.m_vertices.size());
                    festd::iota(lod0.m_indices, 0);
                }
            }

            return &intermediateModel;
        }


        IntermediateSceneNode* ParseNode(IntermediateScene* intermediateScene, const tinygltf::Model& model,
                                         const int32_t nodeIndex, IntermediateSceneNode* parent)
        {
            const tinygltf::Node& node = model.nodes[nodeIndex];

            IntermediateSceneNode* intermediateNode = intermediateScene->m_nodePool.New();
            intermediateNode->m_name = Env::Name(node.name);

            if (parent == nullptr)
            {
                intermediateScene->m_immediateNodes.push_back(intermediateNode);
            }
            else
            {
                intermediateNode->m_parent = parent;
                parent->m_children.push_back(intermediateNode);
            }

            Vector3 translation = Vector3::Zero();
            if (node.translation.size() == 3)
            {
                translation = Vector3::LoadUnaligned(node.translation.data());
            }

            Quaternion rotation = Quaternion::Identity();
            if (node.rotation.size() == 4)
            {
                rotation = Quaternion::LoadUnaligned(node.rotation.data());
            }

            Vector3 scale = Vector3{ 1.0f };
            if (node.scale.size() == 3)
            {
                scale = Vector3::LoadUnaligned(node.scale.data());
            }

            if (node.matrix.size() == 16)
            {
                Matrix4x4 matrix;
                for (uint32_t i = 0; i < 16; ++i)
                    matrix.m_values[i] = static_cast<float>(node.matrix[i]);

                Vector3 shear;
                FE_Verify(Math::DecomposeTransform(matrix, translation, rotation, scale, shear));
                FE_Assert(Math::EqualEstimate(shear, Vector3::Zero()));
            }

            // Correct the coordinate system
            translation.x = -translation.x;
            rotation.x = -rotation.x;
            rotation.w = -rotation.w;

            if (Math::EqualEstimate(scale.x, scale.y) && Math::EqualEstimate(scale.y, scale.z))
            {
                intermediateNode->m_transform = Transform::Create(translation, rotation, scale.x);
                intermediateNode->m_nonUniformScale = Vector3::Zero();
            }
            else
            {
                intermediateNode->m_transform = Transform::Create(translation, rotation, 1.0f);
                intermediateNode->m_nonUniformScale = scale;
            }

            for (const int32_t childIndex : node.children)
            {
                IntermediateSceneNode* childNode = ParseNode(intermediateScene, model, childIndex, intermediateNode);
                intermediateNode->m_children.push_back(childNode);
            }

            if (node.mesh >= 0)
            {
                IntermediateModel* intermediateModel = ParseModel(intermediateScene, model, node.mesh);
                intermediateNode->m_model = intermediateModel;
            }

            return intermediateNode;
        }
    } // namespace


    struct ModelImporter::Implementation final
    {
        bool Load(const void* data, const uint32_t byteSize)
        {
            // We don't need to load images
            m_loader.SetImageLoader(
                [](tinygltf::Image*, const int, std::string*, std::string*, int, int, const uint8_t*, int, void*) {
                    return true;
                },
                nullptr);

            return m_loader.LoadBinaryFromMemory(&m_model, &m_error, &m_warn, static_cast<const uint8_t*>(data), byteSize);
        }

        IntermediateScene* ParseScene()
        {
            auto* intermediateScene = Memory::DefaultNew<IntermediateScene>();

            const int32_t sceneIndex = m_model.defaultScene >= 0 ? m_model.defaultScene : 0;
            tinygltf::Scene& scene = m_model.scenes[sceneIndex];

            for (const int32_t nodeIndex : scene.nodes)
                ParseNode(intermediateScene, m_model, nodeIndex, nullptr);

            return intermediateScene;
        }

        tinygltf::TinyGLTF m_loader;
        tinygltf::Model m_model;

        std::string m_error;
        std::string m_warn;
    };


    void IntermediateSceneNode::Invalidate(IntermediateScene& scene)
    {
        struct ChildRemover
        {
            void ClearChildren(IntermediateSceneNode* node) const
            {
                for (IntermediateSceneNode* child : node->m_children)
                {
                    ClearChildren(child);
                    m_scene->m_nodePool.Delete(child);
                }

                node->m_children.clear();
            }

            IntermediateScene* m_scene;
        };

        const ChildRemover childRemover{ &scene };
        childRemover.ClearChildren(this);
    }


    IntermediateScene::~IntermediateScene()
    {
        for (IntermediateSceneNode* node : m_immediateNodes)
            node->Invalidate(*this);
    }


    ModelImporter::~ModelImporter()
    {
        Memory::DefaultDelete(m_impl);
    }


    ModelImporter::ModelImporter(ModelImporter&& other) noexcept
    {
        m_impl = other.m_impl;
        other.m_impl = nullptr;
    }


    ModelImporter& ModelImporter::operator=(ModelImporter&& other) noexcept
    {
        festd::swap(m_impl, other.m_impl);
        return *this;
    }


    ModelImporter ModelImporter::Create(Logger* logger, const void* data, const uint32_t byteSize)
    {
        ModelImporter importer;
        importer.m_impl = Memory::DefaultNew<Implementation>();

        const bool success = importer.m_impl->Load(data, byteSize);

        if (!importer.m_impl->m_error.empty())
            logger->LogError("GLTF Error: {}", festd::string_view(importer.m_impl->m_error));

        if (!importer.m_impl->m_warn.empty())
            logger->LogWarning("GLTF Warning: {}", festd::string_view(importer.m_impl->m_warn));

        if (!success)
        {
            Memory::DefaultDelete(importer.m_impl);
            importer.m_impl = nullptr;
        }

        return importer;
    }


    IntermediateScene* ModelImporter::ParseScene()
    {
        return m_impl->ParseScene();
    }
} // namespace FE::AssetBuilder

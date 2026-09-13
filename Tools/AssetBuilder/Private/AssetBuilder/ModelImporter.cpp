#include <AssetBuilder/ModelImporter.h>

#include <Core/IO/Path.h>
#include <Core/Strings/Parser.h>

#define TINYGLTF_NO_INCLUDE_RAPIDJSON
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
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
        struct VertexAttribute final
        {
            uint32_t m_offset = kInvalidIndex;
            uint32_t m_size = 0;
        };


        template<class T>
        T ReadUnaligned(const void* data, const size_t byteOffset)
        {
            T result;
            memcpy(&result, static_cast<const std::byte*>(data) + byteOffset, sizeof(T));
            return result;
        }


        float ReadVertexComponent(const std::byte* data, const uint32_t componentType, const bool isNormalized)
        {
            switch (componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                {
                    const int8_t value = ReadUnaligned<int8_t>(data, 0);
                    return isNormalized ? Math::Max(static_cast<float>(value) / 127.0f, -1.0f) : value;
                }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    const uint8_t value = ReadUnaligned<uint8_t>(data, 0);
                    return isNormalized ? static_cast<float>(value) / 255.0f : value;
                }
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                {
                    const int16_t value = ReadUnaligned<int16_t>(data, 0);
                    return isNormalized ? Math::Max(static_cast<float>(value) / 32767.0f, -1.0f) : value;
                }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    const uint16_t value = ReadUnaligned<uint16_t>(data, 0);
                    return isNormalized ? static_cast<float>(value) / 65535.0f : value;
                }
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                return ReadUnaligned<float>(data, 0);
            default:
                FE_DebugBreak();
                return 0.0f;
            }
        }


        VertexAttribute GetVertexAttribute(const festd::string_view attributeName)
        {
            if (attributeName == "POSITION")
                return { offsetof(IntermediateVertex, m_position), sizeof(Vector3) };

            if (attributeName == "NORMAL")
                return { offsetof(IntermediateVertex, m_normal), sizeof(Vector3) };

            if (attributeName == "TANGENT")
                return { offsetof(IntermediateVertex, m_tangentWithSign), sizeof(Vector4) };

            if (attributeName.starts_with("TEXCOORD_"))
            {
                const uint32_t index = Parser::Parse<uint32_t>(attributeName.substr_ascii(sizeof("TEXCOORD_") - 1));
                FE_Assert(index < Graphics::Core::Limits::Vertex::kMaxTexCoords);
                return { offsetof(IntermediateVertex, m_uv[index]), sizeof(Vector2) };
            }

            if (attributeName.starts_with("COLOR_"))
            {
                const uint32_t index = Parser::Parse<uint32_t>(attributeName.substr_ascii(sizeof("COLOR_") - 1));
                FE_Assert(index < Graphics::Core::Limits::Vertex::kMaxColors);
                return { offsetof(IntermediateVertex, m_color[index]), sizeof(Color4F) };
            }

            return {};
        }


        IntermediateModel* ParseModel(IntermediateScene* intermediateScene, const tinygltf::Model& model, const int32_t meshIndex,
                                      const Matrix4x4& worldTransform)
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
                for (IntermediateVertex& vertex : lod0.m_vertices)
                {
                    vertex.m_position = Vector3::kZero;
                    vertex.m_normal = Vector3::kZero;
                    for (Vector2& uv : vertex.m_uv)
                        uv = Vector2::kZero;
                    for (Color4F& color : vertex.m_color)
                        color = Color4F(1.0f);
                    vertex.m_tangentWithSign = Vector4::kZero;
                    memset(vertex.m_influenceBones, 0, sizeof(vertex.m_influenceBones));
                    memset(vertex.m_influenceWeights, 0, sizeof(vertex.m_influenceWeights));
                }

                for (const auto& [attributeName, accessorIndex] : primitive.attributes)
                {
                    const VertexAttribute attribute = GetVertexAttribute(festd::string_view(attributeName));
                    if (attribute.m_offset == kInvalidIndex)
                        continue;

                    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
                    FE_Assert(accessor.count == positionAccessor.count);

                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    const uint8_t* bufferData = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                    const uint32_t componentByteSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
                    const uint32_t componentCount = tinygltf::GetNumComponentsInType(accessor.type);
                    const uint32_t attributeStride = accessor.ByteStride(bufferView);
                    FE_Assert(componentCount * sizeof(float) <= attribute.m_size);

                    for (uint32_t vertexIndex = 0; vertexIndex < lod0.m_vertices.size(); ++vertexIndex)
                    {
                        float* vertexData = reinterpret_cast<float*>(reinterpret_cast<std::byte*>(&lod0.m_vertices[vertexIndex])
                                                                     + attribute.m_offset);
                        const std::byte* sourceData =
                            reinterpret_cast<const std::byte*>(bufferData) + static_cast<size_t>(vertexIndex) * attributeStride;
                        for (uint32_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
                        {
                            vertexData[componentIndex] = ReadVertexComponent(sourceData + componentIndex * componentByteSize,
                                                                             accessor.componentType,
                                                                             accessor.normalized);
                        }
                    }
                }

                if (primitive.indices >= 0)
                {
                    const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferView = model.bufferViews[indicesAccessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    const void* bufferData = &buffer.data[indicesAccessor.byteOffset + bufferView.byteOffset];
                    lod0.m_indices.resize(static_cast<uint32_t>(indicesAccessor.count));
                    const uint32_t indexStride = indicesAccessor.ByteStride(bufferView);

                    switch (indicesAccessor.componentType)
                    {
                    default:
                        FE_DebugBreak();
                        [[fallthrough]];

                    case TINYGLTF_COMPONENT_TYPE_BYTE:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = ReadUnaligned<uint8_t>(bufferData, i * indexStride);
                        break;

                    case TINYGLTF_COMPONENT_TYPE_SHORT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = ReadUnaligned<uint16_t>(bufferData, i * indexStride);
                        break;

                    case TINYGLTF_COMPONENT_TYPE_INT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        for (uint32_t i = 0; i < lod0.m_indices.size(); ++i)
                            lod0.m_indices[i] = ReadUnaligned<uint32_t>(bufferData, i * indexStride);
                        break;
                    }
                }
                else
                {
                    lod0.m_indices.resize(lod0.m_vertices.size());
                    festd::iota(lod0.m_indices, 0);
                }

                const Matrix4x4 normalTransform = Math::Transpose(Math::Invert(worldTransform));
                for (IntermediateVertex& vertex : lod0.m_vertices)
                {
                    vertex.m_position.x = -vertex.m_position.x;
                    vertex.m_normal.x = -vertex.m_normal.x;
                    vertex.m_tangentWithSign.x = -vertex.m_tangentWithSign.x;

                    vertex.m_position = Vector4::GetXYZ(Vector4(vertex.m_position, 1.0f) * worldTransform);
                    const Vector3 normal = Vector4::GetXYZ(Vector4(vertex.m_normal, 0.0f) * normalTransform);
                    vertex.m_normal =
                        Math::LengthSquared(normal) > Constants::kEpsilon ? Math::Normalize(normal) : Vector3::kZero;
                    const float tangentSign = vertex.m_tangentWithSign.w;
                    const Vector3 tangent =
                        Vector4::GetXYZ(Vector4(Vector4::GetXYZ(vertex.m_tangentWithSign), 0.0f) * normalTransform);
                    const Vector3 normalizedTangent =
                        Math::LengthSquared(tangent) > Constants::kEpsilon ? Math::Normalize(tangent) : Vector3::kZero;
                    vertex.m_tangentWithSign = Vector4(normalizedTangent, -tangentSign);
                }

                for (uint32_t index = 0; index + 2 < lod0.m_indices.size(); index += 3)
                    festd::swap(lod0.m_indices[index + 1], lod0.m_indices[index + 2]);
            }

            return &intermediateModel;
        }


        IntermediateSceneNode* ParseNode(IntermediateScene* intermediateScene, const tinygltf::Model& model,
                                         const int32_t nodeIndex, IntermediateSceneNode* parent, const Matrix4x4& parentTransform)
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

            Vector3 translation = Vector3::kZero;
            if (node.translation.size() == 3)
            {
                translation = Vector3::LoadUnaligned(node.translation.data());
            }

            Quaternion rotation = Quaternion::kIdentity;
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
                FE_Assert(Math::CmpEqual(shear, Vector3::kZero));
            }

            // Correct the coordinate system
            translation.x = -translation.x;
            rotation.x = -rotation.x;
            rotation.w = -rotation.w;

            if (Math::CmpEqual(scale.x, scale.y) && Math::CmpEqual(scale.y, scale.z))
            {
                intermediateNode->m_transform = Transform::Create(translation, rotation, scale.x);
                intermediateNode->m_nonUniformScale = Vector3::kZero;
            }
            else
            {
                intermediateNode->m_transform = Transform::Create(translation, rotation, 1.0f);
                intermediateNode->m_nonUniformScale = scale;
            }

            Matrix4x4 localTransform = Transform::ToMatrix(intermediateNode->m_transform);
            if (intermediateNode->m_nonUniformScale != Vector3::kZero)
                localTransform = Matrix4x4::Scale(intermediateNode->m_nonUniformScale) * localTransform;
            const Matrix4x4 worldTransform = localTransform * parentTransform;

            for (const int32_t childIndex : node.children)
                ParseNode(intermediateScene, model, childIndex, intermediateNode, worldTransform);

            if (node.mesh >= 0)
            {
                IntermediateModel* intermediateModel = ParseModel(intermediateScene, model, node.mesh, worldTransform);
                intermediateNode->m_model = intermediateModel;
            }

            return intermediateNode;
        }
    } // namespace


    struct ModelImporter::Implementation final
    {
        bool Load(const void* data, const uint32_t byteSize, const IO::Path& sourcePath)
        {
            // We don't need to load images
            m_loader.SetImageLoader(
                [](tinygltf::Image*, const int, std::string*, std::string*, int, int, const uint8_t*, int, void*) {
                    return true;
                },
                nullptr);

            m_sourceDirectory = IO::PathView(sourcePath).parent_directory();
            if (IO::PathView(sourcePath).extension() == ".gltf")
            {
                return m_loader.LoadASCIIFromString(&m_model,
                                                    &m_error,
                                                    &m_warn,
                                                    static_cast<const char*>(data),
                                                    byteSize,
                                                    m_sourceDirectory.data());
            }

            return m_loader.LoadBinaryFromMemory(&m_model,
                                                 &m_error,
                                                 &m_warn,
                                                 static_cast<const uint8_t*>(data),
                                                 byteSize,
                                                 m_sourceDirectory.data());
        }

        IntermediateScene* ParseScene()
        {
            auto* intermediateScene = Memory::DefaultNew<IntermediateScene>();

            const int32_t sceneIndex = m_model.defaultScene >= 0 ? m_model.defaultScene : 0;
            tinygltf::Scene& scene = m_model.scenes[sceneIndex];

            for (const int32_t nodeIndex : scene.nodes)
                ParseNode(intermediateScene, m_model, nodeIndex, nullptr, Matrix4x4::kIdentity);

            for (const tinygltf::Texture& texture : m_model.textures)
            {
                if (texture.source < 0)
                    continue;

                const tinygltf::Image& image = m_model.images[texture.source];
                if (image.uri.empty() || IO::PathView(image.uri.c_str()).extension() != ".dds")
                    continue;

                const IO::Path texturePath = IO::GetAbsolutePath(m_sourceDirectory / festd::string_view(image.uri.c_str()));
                if (festd::find(intermediateScene->m_texturePaths.begin(), intermediateScene->m_texturePaths.end(), texturePath)
                    == intermediateScene->m_texturePaths.end())
                {
                    intermediateScene->m_texturePaths.push_back(texturePath);
                }
            }

            return intermediateScene;
        }

        tinygltf::TinyGLTF m_loader;
        tinygltf::Model m_model;
        IO::Path m_sourceDirectory;

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
        {
            node->Invalidate(*this);
            m_nodePool.Delete(node);
        }
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


    ModelImporter ModelImporter::Create(const void* data, const uint32_t byteSize, const IO::Path& sourcePath)
    {
        ModelImporter importer;
        importer.m_impl = Memory::DefaultNew<Implementation>();

        const bool success = importer.m_impl->Load(data, byteSize, sourcePath);

        if (!importer.m_impl->m_error.empty())
            Logger::LogError("GLTF Error: {}", festd::string_view(importer.m_impl->m_error));

        if (!importer.m_impl->m_warn.empty())
            Logger::LogWarning("GLTF Warning: {}", festd::string_view(importer.m_impl->m_warn));

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

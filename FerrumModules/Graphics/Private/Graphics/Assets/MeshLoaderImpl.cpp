﻿#include <FeCore/Logging/Trace.h>
#include <FeCore/Math/VectorMath.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace FE::Osmium
{
    inline constexpr int g_PostProcessFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices
        | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_GenSmoothNormals;

    inline uint32_t ComponentSize(MeshVertexComponent component)
    {
        switch (component)
        {
        case MeshVertexComponent::Dummy1F:
            return 1;
        case MeshVertexComponent::TextureCoordinate2F:
        case MeshVertexComponent::Dummy2F:
            return 2;
        case MeshVertexComponent::Position3F:
        case MeshVertexComponent::Normal3F:
        case MeshVertexComponent::Tangent3F:
        case MeshVertexComponent::Bitangent3F:
        case MeshVertexComponent::Color3F:
        case MeshVertexComponent::Dummy3F:
            return 3;
        case MeshVertexComponent::Color4F:
        case MeshVertexComponent::Dummy4F:
            return 4;
        default:
            return 0;
        }
    }

    bool LoadMeshFromMemory(const eastl::vector<int8_t>& fileData, const eastl::vector<MeshVertexComponent>& components,
                            eastl::vector<float>& vertexBuffer, eastl::vector<uint32_t>& indexBuffer, uint32_t& vertexCount)
    {
        Assimp::Importer Importer;
        const aiScene* pScene = Importer.ReadFileFromMemory(fileData.data(), fileData.size(), g_PostProcessFlags, ".fbx");
        if (!pScene)
        {
            const StringSlice error = Importer.GetErrorString();
            FE_LOG_ERROR("Assimp Error: {}", error);
            return false;
        }

        vertexCount = 0;
        uint32_t indexCount = 0;

        uint32_t vertexBufferSize = 0;
        for (auto component : components)
        {
            vertexBufferSize += ComponentSize(component);
        }

        for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
        {
            vertexCount += pScene->mMeshes[i]->mNumVertices;
        }

        vertexBufferSize *= vertexCount;
        vertexBuffer.resize(vertexBufferSize);

        Vector3F scale{ 0.01f, 0.01f, 0.01f };
        uint32_t vbIndex = 0;

        for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = pScene->mMeshes[i];

            aiColor3D color(0.f, 0.f, 0.f);
            pScene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, color);

            const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

            for (uint32_t j = 0; j < mesh->mNumVertices; ++j)
            {
                const aiVector3D* pPos = &mesh->mVertices[j];
                const aiVector3D* pNormal = &mesh->mNormals[j];
                const aiVector3D* pTexCoord = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][j] : &Zero3D;
                const aiVector3D* pTangent = mesh->HasTangentsAndBitangents() ? &mesh->mTangents[j] : &Zero3D;
                const aiVector3D* pBiTangent = mesh->HasTangentsAndBitangents() ? &mesh->mBitangents[j] : &Zero3D;

                for (auto& component : components)
                {
                    switch (component)
                    {
                    case MeshVertexComponent::None:
                        FE_AssertMsg(false, "None component in mesh loading");
                        break;
                    case MeshVertexComponent::Position3F:
                        vertexBuffer[vbIndex++] = pPos->x * scale.X();
                        vertexBuffer[vbIndex++] = -pPos->y * scale.Y();
                        vertexBuffer[vbIndex++] = pPos->z * scale.Z();
                        break;
                    case MeshVertexComponent::Normal3F:
                        vertexBuffer[vbIndex++] = pNormal->x;
                        vertexBuffer[vbIndex++] = -pNormal->y;
                        vertexBuffer[vbIndex++] = pNormal->z;
                        break;
                    case MeshVertexComponent::Tangent3F:
                        vertexBuffer[vbIndex++] = pTangent->x;
                        vertexBuffer[vbIndex++] = pTangent->y;
                        vertexBuffer[vbIndex++] = pTangent->z;
                        break;
                    case MeshVertexComponent::Bitangent3F:
                        vertexBuffer[vbIndex++] = pBiTangent->x;
                        vertexBuffer[vbIndex++] = pBiTangent->y;
                        vertexBuffer[vbIndex++] = pBiTangent->z;
                        break;
                    case MeshVertexComponent::TextureCoordinate2F:
                        vertexBuffer[vbIndex++] = pTexCoord->x;
                        vertexBuffer[vbIndex++] = -pTexCoord->y;
                        break;
                    case MeshVertexComponent::Color3F:
                        vertexBuffer[vbIndex++] = color.r;
                        vertexBuffer[vbIndex++] = color.g;
                        vertexBuffer[vbIndex++] = color.b;
                        break;
                    case MeshVertexComponent::Color4F:
                        vertexBuffer[vbIndex++] = color.r;
                        vertexBuffer[vbIndex++] = color.g;
                        vertexBuffer[vbIndex++] = color.b;
                        vertexBuffer[vbIndex++] = 1.0f;
                        break;
                    case MeshVertexComponent::Dummy4F:
                        vertexBuffer[vbIndex++] = 0.0f;
                        [[fallthrough]];
                    case MeshVertexComponent::Dummy3F:
                        vertexBuffer[vbIndex++] = 0.0f;
                        [[fallthrough]];
                    case MeshVertexComponent::Dummy2F:
                        vertexBuffer[vbIndex++] = 0.0f;
                        [[fallthrough]];
                    case MeshVertexComponent::Dummy1F:
                        vertexBuffer[vbIndex++] = 0.0f;
                        break;
                    }
                }
            }

            auto indexBase = static_cast<uint32_t>(indexBuffer.size());
            for (uint32_t j = 0; j < mesh->mNumFaces; ++j)
            {
                const aiFace& face = mesh->mFaces[j];
                if (face.mNumIndices != 3)
                {
                    continue;
                }

                indexBuffer.push_back(indexBase + face.mIndices[0]);
                indexBuffer.push_back(indexBase + face.mIndices[1]);
                indexBuffer.push_back(indexBase + face.mIndices[2]);
                indexCount += 3;
            }
        }

        return true;
    }
} // namespace FE::Osmium

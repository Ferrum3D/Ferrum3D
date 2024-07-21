#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Osmium
{
    enum class MeshVertexComponent
    {
        None,
        Position3F,
        Normal3F,
        Tangent3F,
        Bitangent3F,
        TextureCoordinate2F,
        Color3F,
        Color4F,
        Dummy1F,
        Dummy2F,
        Dummy3F,
        Dummy4F
    };

    //! \brief Load a mesh from memory.
    //!
    //! The function reads mesh data from a preloaded memory buffer and returns the contents of vertex and index buffers.
    //!
    //! \param [in] fileData      - The file data to load from.
    //! \param [in] components    - A list of components to use while loading the mesh.
    //! \param [out] vertexBuffer - An empty list that will be filled with vertex data.
    //! \param [out] indexBuffer  - An empty list that will be filled with index data.
    //! \param [out] vertexCount  - The number of vertices in the loaded mesh.
    //!
    //! \return True if the mesh was loaded successfully.
    bool LoadMeshFromMemory(const eastl::vector<Int8>& fileData, const eastl::vector<MeshVertexComponent>& components,
                            eastl::vector<float>& vertexBuffer, eastl::vector<UInt32>& indexBuffer, UInt32& vertexCount);
} // namespace FE::Osmium

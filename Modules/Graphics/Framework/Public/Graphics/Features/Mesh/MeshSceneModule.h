#pragma once
#include <FeCore/Math/Sphere.h>
#include <Graphics/Assets/IModelAssetManager.h>
#include <Graphics/Database/Base.h>
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics
{
    struct MeshInstanceTableDecl final
    {
        Sphere m_boundingSphere;
    };


    struct MeshSceneModule;

    struct MeshInstanceGroup final
    {
    private:
        friend MeshSceneModule;

        Rc<ModelAsset> m_asset;
    };


    struct MeshSceneModule final : public ISceneModule
    {
        FE_RTTI("1784843C-5085-4289-AA82-04C480E423EE");

    private:
    };
} // namespace FE::Graphics

#include <GameFramework/MeshSceneBridge.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Passes/DrawTags.h>

namespace FE::GameFramework
{
    MeshSceneBridge::MeshSceneBridge(Graphics::MeshSceneModule* module, Graphics::MeshStreamer* meshes,
                                     Graphics::TextureStreamer* textures)
        : m_module(module)
        , m_meshes(meshes)
        , m_textures(textures)
    {
    }


    Framework::LifecycleResult MeshSceneBridge::Create(const MeshComponent& component, MeshRuntimeComponent& runtime)
    {
        FE_PROFILER_ZONE();
        FE_Assert(m_module && runtime.m_handles.empty());
        const auto model = component.m_model.GetAssetHandle().Read();
        const auto material = component.m_material.GetAssetHandle().Read();
        if (!model || !material || !material->m_runtime || model->m_meshes.empty()
            || model->m_meshes.size() > Graphics::MeshBatch::kMaxInstanceCount)
        {
            return Framework::LifecycleResult::kFailed;
        }

        Aabb bounds = Aabb::kInvalid;
        // Validate the whole model before publishing any scene membership.
        for (const auto& link : model->m_meshes)
        {
            const auto mesh = link.GetAssetHandle().Read();
            if (!mesh || mesh->m_submeshes.empty() || mesh->m_lodErrors.empty())
                return Framework::LifecycleResult::kFailed;

            for (const auto& submesh : mesh->m_submeshes)
            {
                if (!submesh.m_bounds.IsValid())
                    return Framework::LifecycleResult::kFailed;

                bounds = Math::Union(bounds, submesh.m_bounds);
            }
        }

        if (m_textures)
        {
            for (const auto& parameter : material->m_parameters)
            {
                const auto texture = parameter.m_texture.GetAssetHandle().Read();
                if (texture)
                    m_textures->SetResidentMip(*texture.Get(), 0);
            }
        }

        Graphics::MeshBatchDesc batch;
        batch.m_bounds = bounds;
        batch.m_drawTagMask =
            Graphics::DrawTagMask(Graphics::DrawTags::DepthPrepass) | Graphics::DrawTagMask(Graphics::DrawTags::Opaque);
        runtime.m_batch = m_module->CreateBatch(batch);
        for (const auto& link : model->m_meshes)
        {
            const auto handle = link.GetAssetHandle();
            if (m_meshes)
                m_meshes->SetResidentLod(*handle.Read().Get(), 0);

            Graphics::MeshInstanceDesc desc;
            desc.m_asset = IO::AssetLease<Graphics::MeshAsset>(handle.GetAssetSlot());
            desc.m_material =
                IO::AssetLease<Graphics::MaterialInstanceAsset>(component.m_material.GetAssetHandle().GetAssetSlot());
            desc.m_batch = runtime.m_batch;
            desc.m_transform = Matrix4x4::kIdentity;
            runtime.m_handles.push_back(m_module->CreateInstance(desc));
        }

        runtime.m_model = component.m_model.GetAssetID();
        runtime.m_material = component.m_material.GetAssetID();
        return Framework::LifecycleResult::kSucceeded;
    }


    void MeshSceneBridge::Destroy(MeshRuntimeComponent& runtime)
    {
        FE_PROFILER_ZONE();
        FE_Assert(m_module);
        if (runtime.m_batch)
            m_module->DestroyBatch(runtime.m_batch);

        runtime.m_handles.clear();
        runtime.m_batch = nullptr;
        runtime.m_model = runtime.m_material = IO::AssetID::kNull;
    }


    void MeshSceneBridge::Update(const MeshComponent& component, const Matrix4x4& world, MeshRuntimeComponent& runtime)
    {
        if (runtime.m_model != component.m_model.GetAssetID())
        {
            // Build replacement membership first; an invalid model leaves the previous scene state intact.
            MeshRuntimeComponent replacement;
            if (Create(component, replacement) != Framework::LifecycleResult::kSucceeded)
                return;

            Destroy(runtime);
            runtime.m_batch = replacement.m_batch;
            runtime.m_handles = std::move(replacement.m_handles);
            runtime.m_model = replacement.m_model;
            runtime.m_material = replacement.m_material;
        }

        if (runtime.m_material != component.m_material.GetAssetID())
        {
            const auto handle = component.m_material.GetAssetHandle();
            const auto material = handle.Read();
            if (!material || !material->m_runtime)
                return;

            const IO::AssetLease<Graphics::MaterialInstanceAsset> lease(handle.GetAssetSlot());
            for (const auto instance : runtime.m_handles)
                m_module->UpdateMaterial(instance, lease);

            runtime.m_material = component.m_material.GetAssetID();
        }

        for (const auto instance : runtime.m_handles)
            m_module->UpdateTransform(instance, world);
    }
} // namespace FE::GameFramework

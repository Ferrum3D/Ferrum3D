#include <Graphics/Scene/View.h>

namespace FE::Graphics
{
    void View::SetCameraTransform(const Transform& transform)
    {
        m_cameraTransform = transform;
        m_inverseCameraTransform = Math::Invert(transform);
        m_viewMatrix = Transform::ToMatrix(m_inverseCameraTransform);
        m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
        m_inverseViewProjectionMatrix = Math::Invert(m_viewProjectionMatrix);
    }


    void View::SetProjection(const float fovY, const float aspectRatio, const float near, const float far)
    {
        m_fovY = fovY;
        m_aspectRatio = aspectRatio;
        m_nearPlane = near;
        m_farPlane = far;

        m_projectionMatrix = Matrix4x4::Projection(fovY, aspectRatio, near, far);
        m_viewProjectionMatrix = m_viewMatrix * m_projectionMatrix;
        m_inverseViewProjectionMatrix = Math::Invert(m_viewProjectionMatrix);
    }


    bool View::SetDebugMode(const Env::Name name)
    {
        bool found = !name.IsValid();
        m_moduleList.ForEachActive([&](ViewModuleBase& module) {
            const festd::span<const Env::Name> modes = module.GetDebugModes();
            if (modes.empty())
                return;

            uint32_t selected = kInvalidIndex;
            for (uint32_t modeIndex = 0; modeIndex < modes.size(); ++modeIndex)
            {
                if (!found && modes[modeIndex] == name)
                    selected = modeIndex;
            }

            found |= selected != kInvalidIndex;
            module.SetDebugMode(selected);
        });

        return found;
    }


    Env::Name View::GetDebugMode() const
    {
        Env::Name result;
        m_moduleList.ForEachActive([&result](const ViewModuleBase& module) {
            const uint32_t modeIndex = module.GetDebugMode();
            if (modeIndex != kInvalidIndex)
                result = module.GetDebugModes()[modeIndex];
        });

        return result;
    }
} // namespace FE::Graphics

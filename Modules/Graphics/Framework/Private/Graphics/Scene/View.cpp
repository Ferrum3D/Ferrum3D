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
} // namespace FE::Graphics

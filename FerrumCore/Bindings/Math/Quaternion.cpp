#include <FeCore/Math/Quaternion.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void Quaternion_CreateRotationX(Float32 angle, Quaternion* result)
        {
            *result = Quaternion::CreateRotationX(angle);
        }

        FE_DLL_EXPORT void Quaternion_CreateRotationY(Float32 angle, Quaternion* result)
        {
            *result = Quaternion::CreateRotationY(angle);
        }

        FE_DLL_EXPORT void Quaternion_CreateRotationZ(Float32 angle, Quaternion* result)
        {
            *result = Quaternion::CreateRotationZ(angle);
        }

        FE_DLL_EXPORT void Quaternion_FromAxisAngle(Float32 x, Float32 y, Float32 z, Float32 angle, Quaternion* result)
        {
            *result = Quaternion::FromAxisAngle(Vector3F(x, y, z), angle);
        }

        FE_DLL_EXPORT void Quaternion_FromEulerAngles(Float32 x, Float32 y, Float32 z, Quaternion* result)
        {
            *result = Quaternion::FromEulerAngles(x, y, z);
        }

        FE_DLL_EXPORT void Quaternion_GetAxisAngle(Quaternion* self, Float32* axis, Float32* angle)
        {
            Vector3F a;
            Quaternion q = *self;
            q.GetAxisAngle(a, *angle);
            memcpy(axis, a.Data(), 3 * sizeof(Float32));
        }

        FE_DLL_EXPORT void Quaternion_GetEulerAngles(Quaternion* self, Float32* result)
        {
            Quaternion q = *self;
            auto angles  = q.GetEulerAngles();
            memcpy(result, angles.Data(), 3 * sizeof(Float32));
        }

        FE_DLL_EXPORT void Quaternion_Lerp(Quaternion* src, Quaternion* dst, Float32 f, Quaternion* result)
        {
            Quaternion q1 = *src;
            Quaternion q2 = *dst;
            *result       = q1.Lerp(q2, f);
        }

        FE_DLL_EXPORT void Quaternion_SLerp(Quaternion* src, Quaternion* dst, Float32 f, Quaternion* result)
        {
            Quaternion q1 = *src;
            Quaternion q2 = *dst;
            *result       = q1.SLerp(q2, f);
        }

        FE_DLL_EXPORT void Quaternion_Multiply(Quaternion* lhs, Quaternion* rhs, Quaternion* result)
        {
            Quaternion q1 = *lhs;
            Quaternion q2 = *rhs;
            *result       = q1 * q2;
        }
    }
} // namespace FE

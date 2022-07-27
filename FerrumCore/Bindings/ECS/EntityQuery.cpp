#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/ECS/EntityQuery.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT EntityQuery* EntityQuery_Construct(EntityRegistry* registry)
        {
            return MakeShared<EntityQuery>(registry).Detach();
        }

        FE_DLL_EXPORT void EntityQuery_NoneOf(EntityQuery* self, ComponentType* componentTypes, UInt32 componentTypeCount)
        {
            self->NoneOf(ArraySlice(componentTypes, componentTypeCount));
        }

        FE_DLL_EXPORT void EntityQuery_AllOf(EntityQuery* self, ComponentType* componentTypes, UInt32 componentTypeCount)
        {
            self->AllOf(ArraySlice(componentTypes, componentTypeCount));
        }

        FE_DLL_EXPORT void EntityQuery_AnyOf(EntityQuery* self, ComponentType* componentTypes, UInt32 componentTypeCount)
        {
            self->AnyOf(ArraySlice(componentTypes, componentTypeCount));
        }

        FE_DLL_EXPORT void EntityQuery_Update(EntityQuery* self)
        {
            self->Update();
        }

        FE_DLL_EXPORT void EntityQuery_GetChunks(EntityQuery* self, ByteBuffer* result)
        {
            *result = ByteBuffer::MoveList(self->GetChunks());
        }

        FE_DLL_EXPORT void EntityQuery_Destruct(EntityQuery* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::ECS

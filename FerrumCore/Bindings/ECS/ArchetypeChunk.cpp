#include <FeCore/ECS/ArchetypeChunk.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT void* ArchetypeChunk_OffsetOf(ArchetypeChunk* self, const UUID* typeID)
        {
            return self->GetComponentStorage(*typeID)->Data();
        }

        FE_DLL_EXPORT USize ArchetypeChunk_Count(ArchetypeChunk* self)
        {
            return self->Count();
        }
    }
} // namespace FE::ECS

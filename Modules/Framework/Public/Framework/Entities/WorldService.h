#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    //! @brief World integration object with main-thread lifetime and updates outside traversal collection.
    struct WorldService
    {
        FE_RTTI("BB0BDFFE-F26A-4E94-8A9C-B8588475D196");

        virtual ~WorldService() = default;
        virtual void Init(EntityWorld&) {}
        virtual void Shutdown(EntityWorld&) {}
        //! @brief Apply queued requests before structural commit, outside the update schedule.
        virtual void Update(EntityWorld&) {}

    private:
        friend struct EntityWorld;
    };
} // namespace FE::Framework

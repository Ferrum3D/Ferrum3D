#pragma once
#include <Core/IO/Assets.h>
#include <Core/RTTI/Any.h>
#include <Framework/Entities/Base.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Framework
{
    //! @brief Authored source identity, parent, name, and component values; contains no runtime handles.
    struct EntityRecord final
    {
        //! @brief Authored or referenced UUID; never a runtime entity handle.
        Uuid m_uuid = Uuid::kNull;
        //! @brief Source UUID of the authored parent; null identifies a collection root.
        Uuid m_parentUuid = Uuid::kNull;
        //! @brief Display name; not an identity key.
        festd::string m_name;
        //! @brief Reflected component values in authored order.
        festd::inline_vector<Rtti::Any> m_components;
        //! @brief Saved activation intent; inactive records remain allocated and excluded from queries.
        bool m_active = true;
        //! @brief Saved dependency deduplication scope.
        ResidencyScope m_residency = ResidencyScope::kEntity;

        //! @brief Add an authored component value; reject duplicate or unsupported component types.
        template<class T>
        bool AddComponent(const T& value)
        {
            const Rtti::Type& type = Rtti::GetType<T>();
            if (!type.m_serialize || !type.m_deserialize || !type.m_defaultConstructor || !type.m_copyConstructor)
                return false;

            for (const Rtti::Any& component : m_components)
            {
                if (component.GetType() == &type)
                    return false;
            }

            m_components.emplace_back().Emplace<T>(value);
            return true;
        }

        FE_RTTI_Reflect("93D577F3-980E-43E3-A04F-7124E5AF2EFC");
        FE_RTTI_Serialize();
    };


    //! @brief Map one collection source UUID to one concrete placement/runtime UUID.
    struct EntityUuidBinding final
    {
        //! @brief UUID used by the source collection.
        Uuid m_sourceUuid = Uuid::kNull;
        //! @brief Concrete UUID allocated for this placement or spawn.
        Uuid m_entityUuid = Uuid::kNull;

        FE_RTTI_Reflect("252D17D9-4D81-42B0-ACCD-059E148A0529");
        FE_RTTI_Serialize();
    };


    //! @brief Engine-independent entity hierarchy with reflected component values.
    struct EntityCollection;
    //! @brief Authored placement root and concrete identity bindings for a referenced collection.
    struct EntityCollectionInstanceAsset;
    struct EntityWorldAsset;
    struct EntityWorldSnapshotAsset;
} // namespace FE::Framework

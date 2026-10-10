#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Framework/Entities/EntityCollection.h>
#include <Framework/Entities/EntityReference.h>
#include <Framework/Entities/EntitySerialization.h>

namespace FE::Framework
{
    void EntityReference::AfterDeserialize(Serialization::DeserializationContext& context)
    {
        const Uuid remapped = context.RemapObjectReference(m_uuid);
        if (remapped != m_uuid)
            m_placementAsset = IO::AssetID::kNull;
        m_uuid = remapped;
    }


    bool CopyComponent(const Rtti::Type& type, const void* source, void* destination,
                       festd::span<const EntityUuidBinding> bindings, void* referenceUserData,
                       const Serialization::SerializationContext::AssetReferenceCallback referenceCallback)
    {
        FE_PROFILER_ZONE();
        FE_Assert(type.m_serialize && type.m_deserialize && source && destination);

        IO::WriteOnlyMemoryStream stream;
        Serialization::PackedBinaryFormat format;
        Serialization::SerializationContext writer(&stream, format, referenceUserData, referenceCallback);
        if (writer.Store(type, source) != Serialization::ResultCode::kSuccess)
            return false;

        festd::pmr::vector<std::byte> bytes;
        stream.DumpAll(bytes);

        IO::ReadOnlyMemoryStream input(bytes);
        Serialization::DeserializationContext reader(&input, format);
        reader.SetObjectReferenceRemapper(&bindings, [](void* user, const Uuid uuid) {
            for (const EntityUuidBinding& binding : *static_cast<const festd::span<const EntityUuidBinding>*>(user))
            {
                if (binding.m_sourceUuid == uuid)
                    return binding.m_entityUuid;
            }

            return uuid;
        });

        return reader.Load(type, destination) == Serialization::ResultCode::kSuccess;
    }


    bool EntityCollection::Validate() const
    {
        FE_PROFILER_ZONE();

        for (uint32_t index = 0; index < m_entities.size(); ++index)
        {
            const auto& entity = m_entities[index];
            if (!entity.m_uuid.IsValid())
                return false;

            for (uint32_t other = 0; other < index; ++other)
            {
                if (m_entities[other].m_uuid == entity.m_uuid)
                    return false;
            }

            Uuid parent = entity.m_parentUuid;
            uint32_t depth = 0;
            while (parent.IsValid())
            {
                if (++depth > m_entities.size())
                    return false;

                auto found = festd::find_if(m_entities.begin(), m_entities.end(), [&](const EntityRecord& record) {
                    return record.m_uuid == parent;
                });
                if (found == m_entities.end())
                    return false;

                parent = found->m_parentUuid;
            }

            for (uint32_t column = 0; column < entity.m_components.size(); ++column)
            {
                const auto& component = entity.m_components[column];
                const Rtti::Type* type = component.GetType();
                if (!type || !type->m_serialize || !type->m_deserialize || !type->m_defaultConstructor
                    || !type->m_copyConstructor)
                    return false;

                for (uint32_t other = 0; other < column; ++other)
                {
                    if (entity.m_components[other].GetType() == type)
                        return false;
                }
            }
        }
        return true;
    }


    bool EntityCollectionInstanceAsset::UpdateBindings(const EntityCollection& collection)
    {
        if (!collection.Validate())
            return false;

        if (!m_rootUuid.IsValid())
            m_rootUuid = Uuid::Random();

        festd::vector<EntityUuidBinding> bindings;
        for (const auto& entity : collection.m_entities)
        {
            auto found = festd::find_if(m_bindings.begin(), m_bindings.end(), [&](const EntityUuidBinding& binding) {
                return binding.m_sourceUuid == entity.m_uuid;
            });

            bindings.push_back({ entity.m_uuid, found == m_bindings.end() ? Uuid::Random() : found->m_entityUuid });
        }

        m_bindings = std::move(bindings);
        if (m_root.m_entities.empty())
            m_root.m_entities.push_back({});

        m_root.m_entities.front().m_uuid = m_rootUuid;
        return Validate(collection);
    }


    bool EntityCollectionInstanceAsset::MakeIndependentCopy()
    {
        if (!Validate())
            return false;

        EntityCollectionInstanceAsset copy = *this;
        festd::vector<EntityUuidBinding> remapping;
        copy.m_rootUuid = Uuid::Random();
        remapping.push_back({ m_rootUuid, copy.m_rootUuid });

        for (uint32_t index = 0; index < copy.m_bindings.size(); ++index)
        {
            copy.m_bindings[index].m_entityUuid = Uuid::Random();
            remapping.push_back({ m_bindings[index].m_entityUuid, copy.m_bindings[index].m_entityUuid });
        }

        EntityRecord& root = copy.m_root.m_entities.front();
        root.m_uuid = copy.m_rootUuid;
        for (Rtti::Any& component : root.m_components)
        {
            if (!CopyComponent(*component.GetType(), component.GetValue(), component.GetValue(), remapping))
                return false;
        }

        *this = std::move(copy);
        return true;
    }


    bool EntityCollectionInstanceAsset::Validate() const
    {
        if (!m_rootUuid.IsValid() || !m_collection.GetAssetID().IsValid() || !m_root.Validate())
            return false;

        if (m_root.m_entities.size() != 1 || m_root.m_entities.front().m_uuid != m_rootUuid
            || m_root.m_entities.front().m_parentUuid.IsValid())
        {
            return false;
        }

        for (uint32_t index = 0; index < m_bindings.size(); ++index)
        {
            const auto& binding = m_bindings[index];
            if (!binding.m_sourceUuid.IsValid() || !binding.m_entityUuid.IsValid() || binding.m_entityUuid == m_rootUuid)
                return false;

            for (uint32_t other = 0; other < index; ++other)
            {
                if (m_bindings[other].m_sourceUuid == binding.m_sourceUuid
                    || m_bindings[other].m_entityUuid == binding.m_entityUuid)
                {
                    return false;
                }
            }
        }

        return true;
    }


    bool EntityCollectionInstanceAsset::Validate(const EntityCollection& collection) const
    {
        if (!Validate() || !collection.Validate() || m_bindings.size() != collection.m_entities.size())
            return false;

        for (const auto& binding : m_bindings)
        {
            const auto found =
                festd::find_if(collection.m_entities.begin(), collection.m_entities.end(), [&](const EntityRecord& entity) {
                    return entity.m_uuid == binding.m_sourceUuid;
                });

            if (found == collection.m_entities.end())
                return false;
        }

        return true;
    }
} // namespace FE::Framework

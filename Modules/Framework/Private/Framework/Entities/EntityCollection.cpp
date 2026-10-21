#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Framework/Entities/EntityCollection.h>
#include <Framework/Entities/EntityReference.h>
#include <Framework/Entities/EntitySerialization.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    namespace
    {
        enum class Visit : uint8_t
        {
            kUnvisited,
            kVisiting,
            kComplete
        };


        bool ValidateComponents(const EntityRecord& entity)
        {
            for (uint32_t column = 0; column < entity.m_components.size(); ++column)
            {
                const Rtti::Type* type = entity.m_components[column].GetType();
                if (!type || !type->m_serialize || !type->m_deserialize || !type->m_defaultConstructor
                    || !type->m_copyConstructor)
                    return false;

                for (uint32_t other = 0; other < column; ++other)
                {
                    if (entity.m_components[other].GetType() == type)
                        return false;
                }
            }

            return true;
        }
    } // namespace


    Serialization::ResultCode EntityReference::Serialize(Serialization::SerializationContext& context) const
    {
        return RTTI_SerializeFields(context);
    }


    Serialization::ResultCode EntityReference::Deserialize(Serialization::DeserializationContext& context)
    {
        RTTI_DeserializeFields(context);
        if (!context.IsValid())
            return context.GetResultCode();

        const Uuid remapped = context.RemapObjectReference(m_uuid);
        if (remapped != m_uuid)
            m_placementAsset = IO::AssetID::kNull;
        m_uuid = remapped;
        return context.GetResultCode();
    }


    bool ComponentCopier::Copy(const Rtti::Type& type, const void* source, void* destination,
                               festd::span<const EntityUuidBinding> bindings, void* referenceUserData,
                               const Serialization::SerializationContext::AssetReferenceCallback referenceCallback)
    {
        FE_PROFILER_ZONE();
        FE_Assert(type.m_serialize && type.m_deserialize && source && destination);

        m_buffer.clear();
        IO::WriteOnlyVectorStream stream(m_buffer);
        Serialization::PackedBinaryFormat format;
        Serialization::SerializationContext writer(&stream, format, referenceUserData, referenceCallback);
        if (writer.Store(type, source) != Serialization::ResultCode::kSuccess)
            return false;

        IO::ReadOnlyMemoryStream input(m_buffer);
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


    bool CopyComponent(const Rtti::Type& type, const void* source, void* destination,
                       const festd::span<const EntityUuidBinding> bindings, void* referenceUserData,
                       const Serialization::SerializationContext::AssetReferenceCallback referenceCallback)
    {
        ComponentCopier copier;
        return copier.Copy(type, source, destination, bindings, referenceUserData, referenceCallback);
    }


    bool EntityCollection::Validate() const
    {
        FE_PROFILER_ZONE();

        festd::unordered_dense_map<Uuid, uint32_t> indices;
        indices.reserve(m_entities.size());
        for (uint32_t index = 0; index < m_entities.size(); ++index)
        {
            const auto& entity = m_entities[index];
            if (!entity.m_uuid.IsValid() || !indices.emplace(entity.m_uuid, index).second)
                return false;

            if (!ValidateComponents(entity))
                return false;
        }

        // Walk every parent chain once; reaching a node on the current path is a cycle.
        festd::vector<Visit> visits(m_entities.size(), Visit::kUnvisited);
        festd::inline_vector<uint32_t> path;
        for (uint32_t index = 0; index < m_entities.size(); ++index)
        {
            uint32_t current = index;
            while (current != kInvalidIndex && visits[current] == Visit::kUnvisited)
            {
                visits[current] = Visit::kVisiting;
                path.push_back(current);

                const Uuid parent = m_entities[current].m_parentUuid;
                if (!parent.IsValid())
                {
                    current = kInvalidIndex;
                    break;
                }

                const auto found = indices.find(parent);
                if (found == indices.end())
                    return false;

                current = found->second;
            }

            if (current != kInvalidIndex && visits[current] == Visit::kVisiting)
                return false;

            for (const uint32_t node : path)
                visits[node] = Visit::kComplete;

            path.clear();
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
        return Validate() && Matches(collection);
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

        festd::unordered_dense_set<Uuid> sources;
        festd::unordered_dense_set<Uuid> targets;
        sources.reserve(m_bindings.size());
        targets.reserve(m_bindings.size());
        for (const auto& binding : m_bindings)
        {
            if (!binding.m_sourceUuid.IsValid() || !binding.m_entityUuid.IsValid() || binding.m_entityUuid == m_rootUuid)
                return false;

            if (!sources.insert(binding.m_sourceUuid).second || !targets.insert(binding.m_entityUuid).second)
                return false;
        }

        return true;
    }


    bool EntityCollectionInstanceAsset::Validate(const EntityCollection& collection) const
    {
        return Validate() && collection.Validate() && Matches(collection);
    }


    bool EntityCollectionInstanceAsset::Matches(const EntityCollection& collection) const
    {
        if (m_bindings.size() != collection.m_entities.size())
            return false;

        // Sources and collection UUIDs are each unique, so equal counts plus containment is a bijection.
        festd::unordered_dense_set<Uuid> sources;
        sources.reserve(m_bindings.size());
        for (const auto& binding : m_bindings)
            sources.insert(binding.m_sourceUuid);

        return std::all_of(collection.m_entities.begin(), collection.m_entities.end(), [&sources](const EntityRecord& entity) {
            return sources.contains(entity.m_uuid);
        });
    }
} // namespace FE::Framework

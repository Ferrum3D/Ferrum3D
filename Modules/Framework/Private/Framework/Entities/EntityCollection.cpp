#include <Core/IO/AssetManager.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Framework/Entities/EntityCollection.h>
#include <Framework/Entities/EntityReference.h>

namespace FE::Framework
{
    namespace
    {
        IO::DefaultStreamer GEntityAssetStreamer;
    } // namespace


    void RegisterEntityAssetStreamers()
    {
        IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<EntityCollection>(), &GEntityAssetStreamer);
        IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<EntityCollectionInstanceAsset>(), &GEntityAssetStreamer);
    }


    void UnregisterEntityAssetStreamers()
    {
        IO::AssetManager::UnregisterStreamer(Rtti::GetTypeID<EntityCollection>(), &GEntityAssetStreamer);
        IO::AssetManager::UnregisterStreamer(Rtti::GetTypeID<EntityCollectionInstanceAsset>(), &GEntityAssetStreamer);
    }


    void EntityDependencyRecord::BeforeSerialize(Serialization::SerializationContext& context) const
    {
        context.VisitAssetReference(m_asset, m_expectedType, festd::to_underlying(m_kind));
    }


    void EntityReference::AfterDeserialize(Serialization::DeserializationContext& context)
    {
        const Uuid remapped = context.RemapObjectReference(m_uuid);
        if (remapped != m_uuid)
            m_placementAsset = IO::AssetID::kNull;
        m_uuid = remapped;
    }


    bool EntityCollection::CookComponent(EntityRecord& entity, const Rtti::Type& type, const void* value)
    {
        if (!value || !type.m_serialize || !type.m_deserialize || !type.m_defaultConstructor)
            return false;

        for (const auto& component : entity.m_components)
        {
            if (component.m_type == type.m_id)
                return false;
        }

        EntityComponentRecord record;
        record.m_type = type.m_id;
        record.m_version = type.m_serializationVersion;
        record.m_schemaHash = type.m_serializationSchemaHash;

        IO::WriteOnlyMemoryStream stream;
        Serialization::PackedBinaryFormat format;
        Serialization::SerializationContext context(
            &stream,
            format,
            &record,
            [](void* data, Uuid asset, Rtti::TypeID expected, uint32_t kind) {
                auto& dependencies = static_cast<EntityComponentRecord*>(data)->m_dependencies;
                for (const auto& dependency : dependencies)
                {
                    if (dependency.m_asset == asset && dependency.m_expectedType == expected
                        && festd::to_underlying(dependency.m_kind) == kind)
                    {
                        return;
                    }
                }
                dependencies.push_back({ asset, expected, static_cast<IO::DependencyKind>(kind) });
            });

        if (context.Store(type, value) != Serialization::ResultCode::kSuccess)
            return false;

        festd::pmr::vector<std::byte> bytes;
        stream.DumpAll(bytes);
        if (bytes.size() > UINT32_MAX || m_payload.size() > UINT32_MAX - bytes.size())
            return false;

        record.m_payloadOffset = m_payload.size();
        record.m_payloadSize = bytes.size();
        for (const auto byte : bytes)
            m_payload.push_back(static_cast<uint8_t>(byte));

        entity.m_components.push_back(std::move(record));
        return true;
    }


    bool EntityCollection::Validate() const
    {
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

                auto found = festd::find_if(m_entities.begin(), m_entities.end(), [&](const auto& record) {
                    return record.m_uuid == parent;
                });
                if (found == m_entities.end())
                    return false;

                parent = found->m_parentUuid;
            }

            for (uint32_t column = 0; column < entity.m_components.size(); ++column)
            {
                const auto& component = entity.m_components[column];
                if (!component.m_type.IsValid() || component.m_payloadSize == 0 || component.m_payloadOffset > m_payload.size())
                    return false;

                if (component.m_payloadSize > m_payload.size() - component.m_payloadOffset)
                    return false;

                for (uint32_t other = 0; other < column; ++other)
                {
                    if (entity.m_components[other].m_type == component.m_type)
                        return false;
                }

                for (const auto& dependency : component.m_dependencies)
                {
                    if (!dependency.m_asset.IsValid() || !dependency.m_expectedType.IsValid()
                        || dependency.m_kind > IO::DependencyKind::kOptional)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }


    bool EntityCollection::ValidatePayloads() const
    {
        if (!Validate())
            return false;

        for (const auto& entity : m_entities)
        {
            for (const auto& component : entity.m_components)
            {
                const auto* type = Rtti::TypeRegistry::FindType(component.m_type);
                if (!type || !type->m_defaultConstructor || !type->m_destructor || !type->m_deserialize)
                    return false;

                if (type->m_serializationVersion != component.m_version
                    || type->m_serializationSchemaHash != component.m_schemaHash)
                {
                    return false;
                }

                void* data = Memory::DefaultAllocate(type->m_size, type->m_alignment);
                type->m_defaultConstructor(data);
                auto cleanup = festd::defer([&] {
                    type->m_destructor(data);
                    Memory::DefaultFree(data);
                });

                IO::ReadOnlyMemoryStream stream(m_payload.data() + component.m_payloadOffset, component.m_payloadSize);
                Serialization::PackedBinaryFormat format;
                Serialization::DeserializationContext context(&stream, format);
                if (context.Load(*type, data) != Serialization::ResultCode::kSuccess || stream.Tell() != stream.Length())
                    return false;

                EntityCollection verified;
                EntityRecord record;
                if (!verified.CookComponent(record, *type, data))
                    return false;

                const auto& dependencies = record.m_components.front().m_dependencies;
                if (dependencies.size() != component.m_dependencies.size())
                    return false;

                for (const auto& expected : dependencies)
                {
                    const auto found =
                        festd::find_if(component.m_dependencies.begin(), component.m_dependencies.end(), [&](const auto& actual) {
                            return actual.m_asset == expected.m_asset && actual.m_expectedType == expected.m_expectedType
                                && actual.m_kind == expected.m_kind;
                        });

                    if (found == component.m_dependencies.end())
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
            m_rootUuid = NewEntityUuid();

        festd::vector<EntityUuidBinding> bindings;
        for (const auto& entity : collection.m_entities)
        {
            auto found = festd::find_if(m_bindings.begin(), m_bindings.end(), [&](const auto& binding) {
                return binding.m_sourceUuid == entity.m_uuid;
            });

            bindings.push_back({ entity.m_uuid, found == m_bindings.end() ? NewEntityUuid() : found->m_entityUuid });
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
        copy.m_rootUuid = NewEntityUuid();
        remapping.push_back({ m_rootUuid, copy.m_rootUuid });

        for (uint32_t index = 0; index < copy.m_bindings.size(); ++index)
        {
            copy.m_bindings[index].m_entityUuid = NewEntityUuid();
            remapping.push_back({ m_bindings[index].m_entityUuid, copy.m_bindings[index].m_entityUuid });
        }

        EntityCollection root;
        EntityRecord record;
        record.m_uuid = copy.m_rootUuid;
        record.m_name = m_root.m_entities.front().m_name;
        for (const auto& component : m_root.m_entities.front().m_components)
        {
            const auto* type = Rtti::TypeRegistry::FindType(component.m_type);
            if (!type || !type->m_defaultConstructor || !type->m_destructor || !type->m_deserialize)
                return false;

            if (type->m_serializationVersion != component.m_version || type->m_serializationSchemaHash != component.m_schemaHash)
                return false;

            void* data = Memory::DefaultAllocate(type->m_size, type->m_alignment);
            type->m_defaultConstructor(data);
            auto cleanup = festd::defer([&] {
                type->m_destructor(data);
                Memory::DefaultFree(data);
            });

            IO::ReadOnlyMemoryStream stream(m_root.m_payload.data() + component.m_payloadOffset, component.m_payloadSize);
            Serialization::PackedBinaryFormat format;
            Serialization::DeserializationContext context(&stream, format);
            context.SetObjectReferenceRemapper(&remapping, [](void* user, Uuid uuid) {
                for (const auto& binding : *static_cast<festd::vector<EntityUuidBinding>*>(user))
                {
                    if (binding.m_sourceUuid == uuid)
                        return binding.m_entityUuid;
                }
                return uuid;
            });

            if (context.Load(*type, data) != Serialization::ResultCode::kSuccess || stream.Tell() != stream.Length())
                return false;

            if (!root.CookComponent(record, *type, data))
                return false;
        }

        root.m_entities.push_back(std::move(record));
        copy.m_root = std::move(root);
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
                festd::find_if(collection.m_entities.begin(), collection.m_entities.end(), [&](const auto& entity) {
                    return entity.m_uuid == binding.m_sourceUuid;
                });

            if (found == collection.m_entities.end())
                return false;
        }

        return true;
    }
} // namespace FE::Framework

#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/Reflection.h>
#include <FeCore/RTTI/ReflectionContext.h>
#include <FeCore/RTTI/ReflectionPrivate.h>
#include <festd/unordered_map.h>

namespace FE::RTTI
{
    namespace
    {
        struct TypeRegistryImpl final : public ReflectionContext
        {
            void RegisterType(Type& type) override
            {
                FE_AssertDebug(m_typeIDMap.find(type.m_id) == m_typeIDMap.end());
                FE_AssertDebug(m_typeNameMap.find(type.m_qualifiedName) == m_typeNameMap.end());

                m_typeIDMap[type.m_id] = &type;
                m_typeNameMap[type.m_qualifiedName] = &type;
            }

            festd::unordered_dense_map<UUID, Type*> m_typeIDMap;
            festd::unordered_dense_map<festd::ascii_view, Type*> m_typeNameMap;
            festd::intrusive_list<Type> m_typeList;
        };

        TypeRegistryImpl* GTypeRegistry;
    } // namespace


    void TypeRegistry::Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GTypeRegistry == nullptr, "Reflection already initialized");
        GTypeRegistry = Memory::New<TypeRegistryImpl>(allocator);
    }


    void TypeRegistry::Internal::Shutdown()
    {
        FE_CoreAssert(GTypeRegistry != nullptr, "Reflection not initialized");
        GTypeRegistry->~TypeRegistryImpl();
        GTypeRegistry = nullptr;
    }


    const Type* TypeRegistry::FindType(const UUID id)
    {
        auto it = GTypeRegistry->m_typeIDMap.find(id);
        if (it == GTypeRegistry->m_typeIDMap.end())
            return nullptr;

        return it->second;
    }


    const Type* TypeRegistry::FindType(const festd::ascii_view qualifiedName)
    {
        auto it = GTypeRegistry->m_typeNameMap.find(qualifiedName);
        if (it == GTypeRegistry->m_typeNameMap.end())
            return nullptr;

        return it->second;
    }


    TypeList TypeRegistry::GetTypes()
    {
        return TypeList(GTypeRegistry->m_typeList);
    }


    TypeRegistrar::TypeRegistrar(const RegisterFunc func)
    {
        func(*GTypeRegistry);
    }
} // namespace FE::RTTI

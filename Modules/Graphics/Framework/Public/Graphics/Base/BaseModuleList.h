#pragma once
#include <FeCore/Memory/Memory.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    template<class TParent, class TBaseModule>
    struct BaseModuleList final
    {
        explicit BaseModuleList(TParent* parent)
            : m_parent(parent)
        {
        }

        template<class TModule>
        void Add()
        {
            AddImpl<TModule>(true);
        }

        template<class TModule>
        void AddDeactivated()
        {
            AddImpl<TModule>(false);
        }

        template<class TModule>
        void Remove()
        {
            if (const auto it = m_typeMap.find(TModule::TypeID); it != m_typeMap.end())
            {
                const uint32_t index = it->second;
                m_list[index].Reset();
                m_typeMap.erase(TModule::TypeID);
                m_active.reset(index);
                m_free.set(index);
            }
            else
            {
                FE_AssertDebug(false, "Not found");
            }
        }

        [[nodiscard]] TBaseModule* TryFind(const RTTI::TypeID typeID) const
        {
            if (const auto it = m_typeMap.find(typeID); it != m_typeMap.end())
                return m_list[it->second].Get();

            return nullptr;
        }

        template<class TModule>
        [[nodiscard]] TModule* TryFind() const
        {
            if (const auto it = m_typeMap.find(TModule::TypeID); it != m_typeMap.end())
                return RTTI::AssertCast<TModule*>(m_list[it->second].Get());

            return nullptr;
        }

        [[nodiscard]] bool Contains(const RTTI::TypeID typeID) const
        {
            return m_typeMap.contains(typeID);
        }

        template<class TModule>
        [[nodiscard]] bool Contains() const
        {
            return m_typeMap.contains(TModule::TypeID);
        }

        [[nodiscard]] bool IsActive(const RTTI::TypeID typeID) const
        {
            if (const auto it = m_typeMap.find(typeID); it != m_typeMap.end())
                return m_active.test(it->second);

            return false;
        }

        template<class TModule>
        [[nodiscard]] bool IsActive() const
        {
            return IsActive(TModule::TypeID);
        }

        template<class TModule>
        [[nodiscard]] TModule& Find() const
        {
            TModule* result = TryFind<TModule>();
            FE_AssertDebug(result, "Not found");
            return *result;
        }

        template<class TModule>
        void Activate()
        {
            const uint32_t index = m_typeMap.at(TModule::TypeID);
            m_active.set(index);
        }

        template<class TModule>
        void Deactivate()
        {
            const uint32_t index = m_typeMap.at(TModule::TypeID);
            m_active.reset(index);
        }

        template<class TModule>
        void Enable(const bool active)
        {
            if (const auto it = m_typeMap.find(TModule::TypeID); it != m_typeMap.end())
            {
                const uint32_t index = it->second;
                m_active.set(index, active);
            }
            else if (active)
            {
                AddImpl<TModule>(true);
            }
        }

        template<class TFunctor>
        void ForEach(TFunctor functor)
        {
            for (uint32_t moduleIndex = 0; moduleIndex < m_list.size(); ++moduleIndex)
            {
                if (!m_free.test(moduleIndex))
                    functor(*m_list[moduleIndex].Get());
            }
        }

        template<class TFunctor>
        void ForEachActive(TFunctor functor)
        {
            for (uint32_t moduleIndex = 0; moduleIndex < m_list.size(); ++moduleIndex)
            {
                if (m_active.test(moduleIndex))
                {
                    FE_AssertDebug(!m_free.test(moduleIndex));
                    functor(*m_list[moduleIndex].Get());
                }
            }
        }

    private:
        static_assert(std::is_base_of_v<Memory::RefCountedObjectBase, TBaseModule>);

        template<class TModule>
        void AddImpl(const bool active)
        {
            static_assert(std::is_base_of_v<TBaseModule, TModule>);
            FE_Assert(TModule::TypeID != TBaseModule::TypeID);

            uint32_t index = m_free.find_first();
            if (index == kInvalidIndex)
            {
                index = m_list.size();
                m_list.push_back(nullptr);

                if (m_active.size() < index + 1)
                {
                    m_active.resize(m_active.size() + 32, false);
                    m_free.resize(m_free.size() + 32, false);
                }
            }

            if (const auto [it, inserted] = m_typeMap.insert({ TModule::TypeID, index }); inserted)
            {
                TModule* module = Memory::DefaultNew<TModule>(m_parent);
                m_list.push_back(module);
                m_free.reset(index);
                m_active.set(index, active);
            }
            else
            {
                FE_Assert(false, "Cannot add the same module more than once");
            }
        }

        TParent* m_parent = nullptr;
        festd::vector<Rc<TBaseModule>> m_list;
        festd::bit_vector m_active;
        festd::bit_vector m_free;
        festd::unordered_dense_map<RTTI::TypeID, uint32_t> m_typeMap;
    };
} // namespace FE::Graphics

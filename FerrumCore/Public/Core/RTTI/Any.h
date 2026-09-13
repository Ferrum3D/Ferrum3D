#pragma once
#include <Core/Memory/Memory.h>
#include <Core/RTTI/RTTI.h>
#include <Core/RTTI/Reflection.h>

namespace FE::Rtti
{
    //! Type-erased owning value backed by reflected construction, lifetime, and serialization callbacks.
    struct Any final
    {
        Any() = default;
        ~Any();

        Any(const Any& other);
        Any& operator=(const Any& other);

        Any(Any&& other) noexcept;
        Any& operator=(Any&& other) noexcept;

        template<class T, class... TArgs>
        T& Emplace(TArgs&&... args)
        {
            Reset();
            m_type = &Rtti::GetType<T>();
            m_value = Memory::DefaultAllocate(sizeof(T), alignof(T));
            return *::new (m_value) T(std::forward<TArgs>(args)...);
        }

        void Reset();

        [[nodiscard]] bool HasValue() const
        {
            return m_value != nullptr;
        }

        [[nodiscard]] const Type* GetType() const
        {
            return m_type;
        }

        [[nodiscard]] void* GetValue()
        {
            return m_value;
        }

        [[nodiscard]] const void* GetValue() const
        {
            return m_value;
        }

        template<class T>
        [[nodiscard]] T* TryGet()
        {
            return Is<T>() ? static_cast<T*>(m_value) : nullptr;
        }

        template<class T>
        [[nodiscard]] const T* TryGet() const
        {
            return Is<T>() ? static_cast<const T*>(m_value) : nullptr;
        }

        template<class T>
        [[nodiscard]] bool Is() const
        {
            return m_type != nullptr && m_type->m_id == Rtti::GetTypeID<T>();
        }

        Serialization::ResultCode Serialize(Serialization::SerializationContext& context) const;
        Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context);
        static uint64_t RTTI_GetSerializationSchemaHash();
        static uint32_t RTTI_GetSerializationVersion();

        FE_RTTI_Reflect("AB67D9CB-4C40-44E8-8C75-B67B46B1B868");

    private:
        const Type* m_type = nullptr;
        void* m_value = nullptr;
    };
} // namespace FE::Rtti

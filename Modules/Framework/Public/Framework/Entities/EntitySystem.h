#pragma once
#include <Framework/Entities/Base.h>
#include <festd/span.h>

namespace FE::Framework
{
    struct IComponentProvider
    {
        virtual ~IComponentProvider() = default;

        template<class TComponent>
        [[nodiscard]] TComponent* GetRequiredComponent() const
        {
            constexpr ComponentTypeID componentType = ComponentTypeID::Create<TComponent>();
            return static_cast<TComponent*>(GetComponentByTypeID(componentType));
        }

        template<class TComponent>
        [[nodiscard]] TComponent* SafeGetComponent() const
        {
            constexpr ComponentTypeID componentType = ComponentTypeID::Create<TComponent>();
            return static_cast<TComponent*>(SafeGetComponentByTypeID(componentType));
        }

        template<class TComponent>
        [[nodiscard]] bool HasComponent() const
        {
            constexpr ComponentTypeID componentType = ComponentTypeID::Create<TComponent>();
            return HasComponentByTypeID(componentType);
        }

        [[nodiscard]] virtual void* GetComponentByTypeID(ComponentTypeID componentType) const = 0;
        [[nodiscard]] virtual void* SafeGetComponentByTypeID(ComponentTypeID componentType) const = 0;
        [[nodiscard]] virtual bool HasComponentByTypeID(ComponentTypeID componentType) const = 0;
    };


    struct EntitySystem
    {
        virtual ~EntitySystem() = default;

        EntitySystem(const EntitySystem&) = delete;
        EntitySystem(EntitySystem&&) = delete;
        EntitySystem& operator=(const EntitySystem&) = delete;
        EntitySystem& operator=(EntitySystem&&) = delete;

        virtual void Init() {}
        virtual void Shutdown() {}

        virtual void Destroy()
        {
            Memory::DefaultDelete(this);
        }

        virtual void Update(const EntityUpdateContext& context) = 0;

        virtual void OnArchetypeChanged([[maybe_unused]] const IComponentProvider* componentProvider,
                                        [[maybe_unused]] const festd::span<const ComponentTypeID> addedComponents,
                                        [[maybe_unused]] const festd::span<const ComponentTypeID> removedComponents)
        {
        }

    protected:
        EntitySystem() = default;
    };
} // namespace FE::Framework

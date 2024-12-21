#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <festd/vector.h>

namespace FE
{
    template<class TEvent>
    struct EventHandler : public TEvent
    {
    protected:
        FE_RTTI_Class(EventHandler, "389F7F29-DB23-42CE-A877-A5F003701988");

        EventHandler()
        {
            RegisterBus();
        }

        ~EventHandler()
        {
            UnregisterBus();
        }

        void RegisterBus();
        void UnregisterBus();
    };


    template<class>
    struct IEventBus : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IEventBus, "953EC245-BCDD-467C-B1C1-E3CFCB582F4D");
    };


    template<class TEvent>
    struct EventBus final : public ServiceLocatorImplBase<IEventBus<TEvent>>
    {
        using Handler = EventHandler<TEvent>;

        FE_RTTI_Class(EventBus, "0D1E5B6E-ECB5-4859-AB57-4EAACF66AC10");

        EventBus() = default;

        static void RegisterHandler(EventHandler<TEvent>* handler);
        static void UnregisterHandler(EventHandler<TEvent>* handler);

        template<class F, class... Args>
        FE_FORCE_INLINE static void SendEvent(F&& function, Args&&... args);

    private:
        Threading::SpinLock m_lock;
        festd::vector<EventHandler<TEvent>*> m_Handlers;

        template<class F, class... Args>
        void SendEventInternal(F&& function, Args&&... args);

        void RegisterHandlerInternal(EventHandler<TEvent>* handler);
        void UnregisterHandlerInternal(EventHandler<TEvent>* handler);

        static EventBus* Get();
    };


    template<class TEvent>
    template<class F, class... Args>
    void EventBus<TEvent>::SendEventInternal(F&& function, Args&&... args)
    {
        std::lock_guard lock{ m_lock };
        for (Handler* h : m_Handlers)
        {
            std::invoke(function, h, std::forward<Args>(args)...);
        }
    }

    template<class TEvent>
    template<class F, class... Args>
    void EventBus<TEvent>::SendEvent(F&& function, Args&&... args)
    {
        Get()->SendEventInternal(std::forward<F>(function), std::forward<Args>(args)...);
    }

    template<class TEvent>
    void EventBus<TEvent>::RegisterHandlerInternal(Handler* handler)
    {
        std::lock_guard lock{ m_lock };
        m_Handlers.push_back(handler);
    }

    template<class TEvent>
    void EventBus<TEvent>::UnregisterHandlerInternal(Handler* handler)
    {
        std::lock_guard lock{ m_lock };
        m_Handlers.erase(eastl::find(m_Handlers.begin(), m_Handlers.end(), handler));
    }

    template<class TEvent>
    void EventBus<TEvent>::RegisterHandler(EventHandler<TEvent>* handler)
    {
        Get()->RegisterHandlerInternal(handler);
    }

    template<class TEvent>
    void EventBus<TEvent>::UnregisterHandler(EventHandler<TEvent>* handler)
    {
        Get()->UnregisterHandlerInternal(handler);
    }

    template<class TEvent>
    EventBus<TEvent>* EventBus<TEvent>::Get()
    {
        return fe_assert_cast<EventBus<TEvent>*>(FE::ServiceLocator<IEventBus<TEvent>>::Get());
    }

    template<class TEvent>
    void EventHandler<TEvent>::RegisterBus()
    {
        EventBus<TEvent>::RegisterHandler(this);
    }

    template<class TEvent>
    void EventHandler<TEvent>::UnregisterBus()
    {
        EventBus<TEvent>::UnregisterHandler(this);
    }
} // namespace FE

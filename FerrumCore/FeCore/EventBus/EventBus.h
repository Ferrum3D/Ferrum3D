#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <functional>

namespace FE
{
    template<class TEvent>
    class EventHandler : public TEvent
    {
    protected:
        FE_CLASS_RTTI(EventHandler, "389F7F29-DB23-42CE-A877-A5F003701988");

        inline EventHandler()
        {
            RegisterBus();
        }

        inline ~EventHandler()
        {
            UnregisterBus();
        }

        inline void RegisterBus();
        inline void UnregisterBus();
    };

    template<class>
    class IEventBus : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(IEventBus, "953EC245-BCDD-467C-B1C1-E3CFCB582F4D");
    };

    template<class TEvent>
    class EventBus : public ServiceLocatorImplBase<IEventBus<TEvent>>
    {
        eastl::vector<EventHandler<TEvent>*> m_Handlers;

        template<class F, class... Args>
        inline void SendEventInternal(F&& function, Args&&... args);

        inline void RegisterHandlerInternal(EventHandler<TEvent>* handler);
        inline void UnregisterHandlerInternal(EventHandler<TEvent>* handler);

        static EventBus* Get();

    public:
        using Handler = EventHandler<TEvent>;

        FE_CLASS_RTTI(EventBus, "0D1E5B6E-ECB5-4859-AB57-4EAACF66AC10");

        inline EventBus() = default;

        inline static void RegisterHandler(EventHandler<TEvent>* handler);
        inline static void UnregisterHandler(EventHandler<TEvent>* handler);

        template<class F, class... Args>
        FE_FINLINE static void SendEvent(F&& function, Args&&... args);
    };

    template<class TEvent>
    template<class F, class... Args>
    void EventBus<TEvent>::SendEventInternal(F&& function, Args&&... args)
    {
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
    void FE::EventBus<TEvent>::RegisterHandlerInternal(Handler* handler)
    {
        m_Handlers.push_back(handler);
    }

    template<class TEvent>
    void FE::EventBus<TEvent>::UnregisterHandlerInternal(Handler* handler)
    {
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

#pragma once
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    namespace Internal
    {
        template<class T>
        struct QueryTerm
        {
            static_assert(!std::is_reference_v<T> && !std::is_pointer_v<std::remove_pointer_t<T>>,
                          "Query terms are component types or single optional pointers");
            using Raw = std::remove_pointer_t<T>;
            using Component = std::remove_const_t<Raw>;
            using Argument = std::conditional_t<std::is_pointer_v<T>, T, T&>;
            static constexpr bool kOptional = std::is_pointer_v<T>;
            static constexpr bool kWrite = !std::is_const_v<Raw>;
            static constexpr bool kParent = false;
            static Argument Get(void* data)
            {
                if constexpr (kOptional)
                    return static_cast<T>(data);
                else
                    return *static_cast<T*>(data);
            }
        };


        template<class T>
        struct QueryTerm<Parent<T>> : QueryTerm<T>
        {
            static_assert(!QueryTerm<T>::kWrite, "Parent terms support read-only access");
            static constexpr bool kParent = true;
        };


        template<class... T>
        struct UniqueTerms;
        template<>
        struct UniqueTerms<>
        {
            static constexpr bool kValue = true;
        };


        template<class Head, class... Tail>
        struct UniqueTerms<Head, Tail...>
        {
            static constexpr bool kValue =
                ((!std::is_same_v<typename QueryTerm<Head>::Component, typename QueryTerm<Tail>::Component>
                  || QueryTerm<Head>::kParent != QueryTerm<Tail>::kParent)
                 && ...)
                && UniqueTerms<Tail...>::kValue;
        };
    } // namespace Internal

    template<bool Cascade, class... Terms>
    struct BasicQuery
    {
        static_assert(sizeof...(Terms) > 0, "Query needs at least one component term");
        static_assert(Internal::UniqueTerms<Terms...>::kValue, "Duplicate component terms in the same source");
        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase, Callable&& callable)
        {
            return Traverse(context, phase, std::initializer_list<Rc<WaitGroup>>{}, std::forward<Callable>(callable));
        }


        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase, ExecutionPolicy policy, Callable&& callable)
        {
            return Traverse(context, phase, std::initializer_list<Rc<WaitGroup>>{}, std::forward<Callable>(callable), policy);
        }


        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase,
                                      std::initializer_list<Rc<WaitGroup>> prerequisites, Callable&& callable,
                                      ExecutionPolicy policy = ExecutionPolicy::kSequential, ChangeCursor* cursor = nullptr)
        {
            using Function = std::decay_t<Callable>;
            constexpr bool withEntity = std::is_invocable_v<Function&, Entity&, typename Internal::QueryTerm<Terms>::Argument...>;
            constexpr bool withoutEntity = std::is_invocable_v<Function&, typename Internal::QueryTerm<Terms>::Argument...>;
            static_assert(withEntity || withoutEntity, "Callback must accept query values, optionally preceded by Entity&");
            (context.m_world.Components().Register<typename Internal::QueryTerm<Terms>::Component>(), ...);
            const QueryAccess accesses[] = { { Rtti::GetTypeID<typename Internal::QueryTerm<Terms>::Component>(),
                                               Internal::QueryTerm<Terms>::kOptional,
                                               Internal::QueryTerm<Terms>::kWrite,
                                               Internal::QueryTerm<Terms>::kParent }... };
            void* storage = context.m_world.AllocateTraversal(sizeof(Function), alignof(Function));
            ::new (storage) Function(std::forward<Callable>(callable));
            return context.m_world.RecordTraversal(
                context,
                phase,
                accesses,
                storage,
                [](void* function, Entity& entity, void** values) {
                    Invoke(*static_cast<Function*>(function), entity, values, std::index_sequence_for<Terms...>{});
                },
                [](void* function) {
                    static_cast<Function*>(function)->~Function();
                },
                festd::span<const Rc<WaitGroup>>(prerequisites.begin(), static_cast<uint32_t>(prerequisites.size())),
                policy,
                Cascade,
                cursor);
        }


        template<class Callable>
        static Rc<WaitGroup> TraverseChanged(EntityUpdateContext& context, Phase phase, ChangeCursor& cursor, Callable&& callable,
                                             ExecutionPolicy policy = ExecutionPolicy::kSequential)
        {
            return Traverse(context, phase, {}, std::forward<Callable>(callable), policy, &cursor);
        }

    private:
        template<class Function, size_t... Indices>
        static void Invoke(Function& function, Entity& entity, void** values, std::index_sequence<Indices...>)
        {
            if constexpr (std::is_invocable_v<Function&, Entity&, typename Internal::QueryTerm<Terms>::Argument...>)
                function(entity, Internal::QueryTerm<Terms>::Get(values[Indices])...);
            else
                function(Internal::QueryTerm<Terms>::Get(values[Indices])...);
        }
    };


    template<class... Terms>
    using Query = BasicQuery<false, Terms...>;


    template<class... Terms>
    using CascadeQuery = BasicQuery<true, Terms...>;
} // namespace FE::Framework

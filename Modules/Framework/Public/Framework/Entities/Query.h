#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <tuple>

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
            static constexpr bool kExcluded = false;
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
            static_assert(!QueryTerm<T>::kExcluded, "Without terms only filter the queried entity");
            static constexpr bool kParent = true;
        };


        template<class T>
        struct QueryTerm<Without<T>>
        {
            static_assert(!std::is_pointer_v<T> && !std::is_reference_v<T>, "Without requires a component type");
            using Component = std::remove_const_t<T>;
            static constexpr bool kOptional = false;
            static constexpr bool kWrite = false;
            static constexpr bool kParent = false;
            static constexpr bool kExcluded = true;
        };


        template<size_t... Left, size_t... Right>
        auto JoinQueryIndices(std::index_sequence<Left...>, std::index_sequence<Right...>)
            -> std::index_sequence<Left..., Right...>;


        template<size_t Index, class... Terms>
        struct QueryValueIndices;


        template<size_t Index>
        struct QueryValueIndices<Index>
        {
            using Type = std::index_sequence<>;
        };


        template<size_t Index, class Head, class... Tail>
        struct QueryValueIndices<Index, Head, Tail...>
        {
            using HeadIndex = std::conditional_t<QueryTerm<Head>::kExcluded, std::index_sequence<>, std::index_sequence<Index>>;
            using Type = decltype(JoinQueryIndices(HeadIndex{}, typename QueryValueIndices<Index + 1, Tail...>::Type{}));
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
    //! @brief Typed query using required references, optional pointers, read-only parent terms, and exclusion filters.
    struct BasicQuery
    {
        static_assert(sizeof...(Terms) > 0, "Query needs at least one component term");
        static_assert(Internal::UniqueTerms<Terms...>::kValue, "Duplicate component terms in the same source");
        //! @brief Record typed component access and an epoch-owned callback; optional pointer terms may receive null.
        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase, Callable&& callable)
        {
            return Traverse(context, phase, std::initializer_list<Rc<WaitGroup>>{}, std::forward<Callable>(callable));
        }

        //! @brief Record typed component access and an epoch-owned callback; optional pointer terms may receive null.
        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase, ExecutionPolicy policy, Callable&& callable)
        {
            return Traverse(context, phase, std::initializer_list<Rc<WaitGroup>>{}, std::forward<Callable>(callable), policy);
        }

        //! @brief Record typed component access and an epoch-owned callback; optional pointer terms may receive null.
        template<class Callable>
        static Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase,
                                      std::initializer_list<Rc<WaitGroup>> prerequisites, Callable&& callable,
                                      ExecutionPolicy policy = ExecutionPolicy::kSequential, ChangeCursor* cursor = nullptr)
        {
            using Function = std::decay_t<Callable>;
            static_assert(AcceptsCallback<Function>(ValueIndices{}),
                          "Callback must accept query values, optionally preceded by Entity&; Without has no argument");
            (context.m_world.Components().Register<typename Internal::QueryTerm<Terms>::Component>(), ...);

            const QueryAccess accesses[] = { { Rtti::GetTypeID<typename Internal::QueryTerm<Terms>::Component>(),
                                               Internal::QueryTerm<Terms>::kOptional,
                                               Internal::QueryTerm<Terms>::kWrite,
                                               Internal::QueryTerm<Terms>::kParent,
                                               Internal::QueryTerm<Terms>::kExcluded }... };

            void* storage = context.m_world.AllocateTraversal(sizeof(Function), alignof(Function));
            ::new (storage) Function(std::forward<Callable>(callable));

            return context.m_world.RecordTraversal(
                context,
                { phase,
                  accesses,
                  storage,
                  [](void* function, Entity& entity, void** values) {
                      Invoke(*static_cast<Function*>(function), entity, values, ValueIndices{});
                  },
                  [](void* function) {
                      static_cast<Function*>(function)->~Function();
                  },
                  festd::span<const Rc<WaitGroup>>(prerequisites.begin(), static_cast<uint32_t>(prerequisites.size())),
                  cursor,
                  policy,
                  Cascade });
        }

        //! @brief Record work using a persistent cursor; one cursor may be consumed only once per epoch.
        template<class Callable>
        static Rc<WaitGroup> TraverseChanged(EntityUpdateContext& context, Phase phase, ChangeCursor& cursor, Callable&& callable,
                                             ExecutionPolicy policy = ExecutionPolicy::kSequential)
        {
            return Traverse(context, phase, {}, std::forward<Callable>(callable), policy, &cursor);
        }

    private:
        using ValueIndices = typename Internal::QueryValueIndices<0, Terms...>::Type;
        template<size_t Index>
        using ValueTerm = Internal::QueryTerm<std::tuple_element_t<Index, std::tuple<Terms...>>>;

        template<class Function, size_t... Indices>
        static constexpr bool AcceptsCallback(std::index_sequence<Indices...>)
        {
            return std::is_invocable_v<Function&, Entity&, typename ValueTerm<Indices>::Argument...>
                || std::is_invocable_v<Function&, typename ValueTerm<Indices>::Argument...>;
        }


        template<class Function, size_t... Indices>
        static void Invoke(Function& function, Entity& entity, void** values, std::index_sequence<Indices...>)
        {
            if constexpr (std::is_invocable_v<Function&, Entity&, typename ValueTerm<Indices>::Argument...>)
                function(entity, ValueTerm<Indices>::Get(values[Indices])...);
            else
                function(ValueTerm<Indices>::Get(values[Indices])...);
        }
    };


    //! @brief Match active entities by required/optional components and Without filters, without hierarchy ordering.
    template<class... Terms>
    using Query = BasicQuery<false, Terms...>;

    //! @brief Match components while completing each active parent before its descendants.
    template<class... Terms>
    using CascadeQuery = BasicQuery<true, Terms...>;
} // namespace FE::Framework

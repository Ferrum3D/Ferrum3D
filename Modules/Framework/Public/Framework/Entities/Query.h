#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <array>
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


    //! @brief Typed query using required references, optional pointers, read-only parent terms, and exclusion filters.
    template<bool TCascade, class... TTerms>
    struct BasicQuery
    {
        static_assert(Internal::UniqueTerms<TTerms...>::kValue, "Duplicate component terms in the same source");

        template<class TFunctor>
        Rc<WaitGroup> Traverse(EntityUpdateContext& context, Phase phase, TFunctor&& functor)
        {
            return TraverseImpl(context, phase, std::forward<TFunctor>(functor));
        }

        static BasicQuery Create()
        {
            return BasicQuery{};
        }

        [[nodiscard]] BasicQuery& SetChangeCursor(ChangeCursor& cursor)
        {
            m_changeCursor = &cursor;
            return *this;
        }

        [[nodiscard]] BasicQuery& SetExecutionPolicy(const ExecutionPolicy policy)
        {
            m_executionPolicy = policy;
            return *this;
        }

        [[nodiscard]] BasicQuery& SetPrerequisites(const festd::span<const Rc<WaitGroup>> prerequisites)
        {
            m_prerequisites.assign(prerequisites.begin(), prerequisites.end());
            return *this;
        }

        [[nodiscard]] BasicQuery& SetPrerequisites(std::initializer_list<Rc<WaitGroup>> prerequisites)
        {
            return SetPrerequisites(festd::span<const Rc<WaitGroup>>(prerequisites.begin(), prerequisites.end()));
        }

        //! @brief Addition filters require a persistent cursor and consume only published additions.
        [[nodiscard]] BasicQuery& SetFilter(const QueryFilter filter)
        {
            m_filter = filter;
            return *this;
        }

    private:
        using ValueIndices = typename Internal::QueryValueIndices<0, TTerms...>::Type;

        template<size_t TIndex>
        using ValueTerm = Internal::QueryTerm<std::tuple_element_t<TIndex, std::tuple<TTerms...>>>;

        ChangeCursor* m_changeCursor = nullptr;
        ExecutionPolicy m_executionPolicy = ExecutionPolicy::kSequential;
        QueryFilter m_filter = QueryFilter::kAll;
        festd::inline_vector<Rc<WaitGroup>> m_prerequisites;

        template<class Function, size_t... Indices>
        static constexpr bool AcceptsCallback(std::index_sequence<Indices...>)
        {
            return std::is_invocable_v<Function&, Entity&, typename ValueTerm<Indices>::Argument...>
                || std::is_invocable_v<Function&, typename ValueTerm<Indices>::Argument...>;
        }

        template<class TFunction, size_t... TIndices>
        static bool Invoke(TFunction& function, Entity& entity, void** values, std::index_sequence<TIndices...>)
        {
            if constexpr (std::is_invocable_v<TFunction&, Entity&, typename ValueTerm<TIndices>::Argument...>)
            {
                if constexpr (std::same_as<std::invoke_result_t<TFunction&, Entity&, typename ValueTerm<TIndices>::Argument...>,
                                           QueryWriteResult>)
                {
                    return function(entity, ValueTerm<TIndices>::Get(values[TIndices])...) == QueryWriteResult::kChanged;
                }
                else
                {
                    function(entity, ValueTerm<TIndices>::Get(values[TIndices])...);
                    return true;
                }
            }
            else
            {
                if constexpr (std::same_as<std::invoke_result_t<TFunction&, typename ValueTerm<TIndices>::Argument...>,
                                           QueryWriteResult>)
                {
                    return function(ValueTerm<TIndices>::Get(values[TIndices])...) == QueryWriteResult::kChanged;
                }
                else
                {
                    function(ValueTerm<TIndices>::Get(values[TIndices])...);
                    return true;
                }
            }
        }

        template<class TFunctor>
        Rc<WaitGroup> TraverseImpl(EntityUpdateContext& context, const Phase phase, TFunctor&& callable)
        {
            using Function = std::decay_t<TFunctor>;
            static_assert(AcceptsCallback<Function>(ValueIndices{}),
                          "Callback must accept query values, optionally preceded by Entity&; Without has no argument");
            (context.m_world.Components().Register<typename Internal::QueryTerm<TTerms>::Component>(), ...);

            const std::array<QueryAccess, sizeof...(TTerms)> accesses = {
                { { Rtti::GetTypeID<typename Internal::QueryTerm<TTerms>::Component>(),
                    Internal::QueryTerm<TTerms>::kOptional,
                    Internal::QueryTerm<TTerms>::kWrite,
                    Internal::QueryTerm<TTerms>::kParent,
                    Internal::QueryTerm<TTerms>::kExcluded }... }
            };

            void* storage = context.m_world.AllocateTraversal(sizeof(Function), alignof(Function));
            ::new (storage) Function(std::forward<TFunctor>(callable));

            return context.m_world.RecordTraversal(
                context,
                { phase,
                  { accesses.data(), static_cast<uint32_t>(accesses.size()) },
                  storage,
                  [](void* function, Entity& entity, void** values) {
                      return Invoke(*static_cast<Function*>(function), entity, values, ValueIndices{});
                  },
                  [](void* function) {
                      static_cast<Function*>(function)->~Function();
                  },
                  m_prerequisites,
                  m_changeCursor,
                  m_executionPolicy,
                  TCascade,
                  m_filter });
        }
    };


    //! @brief Match active entities by required/optional components and Without filters, without hierarchy ordering.
    template<class... Terms>
    using Query = BasicQuery<false, Terms...>;

    //! @brief Match components while completing each active parent before its descendants.
    template<class... Terms>
    using CascadeQuery = BasicQuery<true, Terms...>;
} // namespace FE::Framework

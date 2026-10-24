#pragma once
#include <Core/RTTI/Reflection.h>
#include <festd/vector.h>

namespace FE::Rtti
{
    //! One step from a value to a value nested in it.
    struct PathStep final
    {
        enum class Kind : uint8_t
        {
            //! Index into Type::m_baseTypes.
            kBase,
            //! Index into Type::m_fields.
            kField,
            //! Element of a fixed array or sequence.
            kElement,
            //! Active alternative of a variant; the index names the alternative.
            kAlternative,
            //! Value held by a dynamic shape.
            kDynamic,
        };

        Kind m_kind = Kind::kField;
        uint32_t m_index = 0;

        friend bool operator==(const PathStep&, const PathStep&) = default;
    };


    //! Location of a value nested in a root value, e.g. an element of a sequence field of a component.
    using ValuePath = festd::inline_vector<PathStep, 8>;


    struct ValueRef final
    {
        const Type* m_type = nullptr;
        void* m_value = nullptr;

        [[nodiscard]] explicit operator bool() const
        {
            return m_type != nullptr && m_value != nullptr;
        }
    };


    //! @brief Element type of a fixed array or sequence; null for other types.
    [[nodiscard]] const Type* GetElementType(const Type& type);


    //! @brief Value one step into another; null when the step does not apply, e.g. an index past the end of a sequence.
    ValueRef ResolveStep(const Type& type, void* value, PathStep step);


    //! @brief Follow a path from a root value; null when the structure no longer matches it.
    ValueRef ResolvePath(const Type& type, void* value, festd::span<const PathStep> path);


    enum class VisitResult : uint8_t
    {
        kContinue,
        kSkipChildren,
        kStop,
    };


    enum class VisitFlags : uint32_t
    {
        kNone = 0,
        //! Skip fields that are not serialized, such as runtime state.
        kSkipTransient = 1 << 0,
    };

    FE_ENUM_OPERATORS(VisitFlags);


    struct VisitNode final
    {
        const Type& m_type;
        void* m_value;
        //! Field that holds the value; null for the root, bases, elements, alternatives and dynamic values.
        const FieldInfo* m_field;
        const ValuePath& m_path;
    };


    using VisitCallback = VisitResult (*)(void* userData, const VisitNode& node);


    //! @brief Visit a value and every value reachable from it, parents before children.
    //!
    //! Structs visit their bases, then their fields. Dynamic values are visited as their most derived type.
    //! @return False when the callback stopped the traversal.
    bool Visit(const Type& type, void* value, VisitFlags flags, void* userData, VisitCallback callback);


    template<class TCallback>
    bool Visit(const Type& type, void* value, const VisitFlags flags, TCallback&& callback)
    {
        using Function = std::remove_reference_t<TCallback>;
        return Visit(type, value, flags, &callback, [](void* userData, const VisitNode& node) {
            return (*static_cast<Function*>(userData))(node);
        });
    }


    //! @brief Read-only Visit; callbacks must not write through VisitNode::m_value.
    template<class TCallback>
    bool VisitConst(const Type& type, const void* value, const VisitFlags flags, TCallback&& callback)
    {
        return Visit(type, const_cast<void*>(value), flags, std::forward<TCallback>(callback));
    }


    //! @brief Check whether values of a type can contain a value of the given shape; dynamic values always can.
    [[nodiscard]] bool CanContainShape(const Type& type, TypeShape shape);


    //! @brief Check whether values of a type can contain a value of the target type; dynamic values always can.
    [[nodiscard]] bool CanContainType(const Type& type, const Type& target);


    //! @brief Copy the reflected structure of source into an initialized destination of the same type.
    //!
    //! Structs copy field by field, so with kSkipTransient the destination keeps its own runtime state.
    //! Sequences, variants and dynamic values adopt the source's size, alternative and type. Leaves are copy-assigned.
    //! Source and destination must not be the same value.
    //!
    //! @return False when a value cannot be copied, e.g. a leaf without copy assignment; the destination is partially updated.
    bool CopyValue(const Type& type, const void* source, void* destination, VisitFlags flags = VisitFlags::kNone);
} // namespace FE::Rtti

#include <Core/RTTI/Traversal.h>

namespace FE::Rtti
{
    namespace
    {
        void* CastToBase(const Type& type, void* value, const TypeID baseId)
        {
            return type.m_cast ? type.m_cast(value, baseId) : nullptr;
        }


        void* GetElement(const Type& type, void* value, const uint32_t index)
        {
            if (const auto* array = type.TryGetShape<FixedArrayShape>())
                return index < array->m_count ? static_cast<std::byte*>(value) + index * array->m_stride : nullptr;

            if (const auto* sequence = type.TryGetShape<SequenceShape>())
                return index < sequence->m_size(value) ? sequence->m_at(value, index) : nullptr;

            return nullptr;
        }


        uint32_t GetElementCount(const Type& type, const void* value)
        {
            if (const auto* array = type.TryGetShape<FixedArrayShape>())
                return array->m_count;

            if (const auto* sequence = type.TryGetShape<SequenceShape>())
                return sequence->m_size(value);

            return 0;
        }


        bool IsVisitedField(const FieldInfo& field, const VisitFlags flags)
        {
            if (!Bit::AnySet(field.m_flags, FieldFlags::kInstance))
                return false;

            return !Bit::AllSet(flags, VisitFlags::kSkipTransient) || !Bit::AnySet(field.m_flags, FieldFlags::kTransient);
        }


        struct Visitor final
        {
            VisitFlags m_flags;
            void* m_userData;
            VisitCallback m_callback;
            ValuePath m_path;

            //! @return False when the traversal was stopped.
            bool VisitValue(const Type& type, void* value, const FieldInfo* field)
            {
                const VisitResult result = m_callback(m_userData, VisitNode{ type, value, field, m_path });
                if (result == VisitResult::kStop)
                    return false;

                if (result == VisitResult::kSkipChildren)
                    return true;

                return VisitChildren(type, value);
            }

        private:
            bool VisitChild(const PathStep step, const Type& type, void* value, const FieldInfo* field)
            {
                m_path.push_back(step);
                const bool isRunning = VisitValue(type, value, field);
                m_path.pop_back();
                return isRunning;
            }


            bool VisitStruct(const Type& type, void* value)
            {
                for (uint32_t baseIndex = 0; baseIndex < type.m_baseTypes.size(); ++baseIndex)
                {
                    const Type* base = TypeRegistry::FindType(type.m_baseTypes[baseIndex]);
                    void* baseValue = CastToBase(type, value, type.m_baseTypes[baseIndex]);
                    if (base == nullptr || baseValue == nullptr)
                        continue;

                    // Bases are flattened, so each one contributes only its own fields.
                    m_path.push_back({ PathStep::Kind::kBase, baseIndex });
                    const bool isRunning = VisitFields(*base, baseValue);
                    m_path.pop_back();
                    if (!isRunning)
                        return false;
                }

                return VisitFields(type, value);
            }


            bool VisitFields(const Type& type, void* value)
            {
                for (uint32_t fieldIndex = 0; fieldIndex < type.m_fields.size(); ++fieldIndex)
                {
                    const FieldInfo& field = type.m_fields[fieldIndex];
                    if (!IsVisitedField(field, m_flags))
                        continue;

                    if (!VisitChild({ PathStep::Kind::kField, fieldIndex }, *field.m_type, field.GetAddress(value), &field))
                        return false;
                }

                return true;
            }


            bool VisitElements(const Type& type, void* value)
            {
                const Type* elementType = GetElementType(type);
                const uint32_t count = GetElementCount(type, value);
                for (uint32_t index = 0; index < count; ++index)
                {
                    if (!VisitChild({ PathStep::Kind::kElement, index }, *elementType, GetElement(type, value, index), nullptr))
                        return false;
                }

                return true;
            }


            bool VisitChildren(const Type& type, void* value)
            {
                switch (type.m_shape)
                {
                case TypeShape::kStruct:
                    return VisitStruct(type, value);

                case TypeShape::kFixedArray:
                case TypeShape::kSequence:
                    return VisitElements(type, value);

                case TypeShape::kVariant:
                    {
                        const auto& variant = *type.TryGetShape<VariantShape>();
                        const uint32_t index = variant.m_index(value);
                        if (index >= variant.m_alternatives.size())
                            return true;

                        const PathStep step{ PathStep::Kind::kAlternative, index };
                        return VisitChild(step, *variant.m_alternatives[index], variant.m_get(value), nullptr);
                    }

                case TypeShape::kDynamic:
                    {
                        const auto& dynamic = *type.TryGetShape<DynamicShape>();
                        const Type* heldType = dynamic.m_type(value);
                        if (heldType == nullptr)
                            return true;

                        return VisitChild({ PathStep::Kind::kDynamic, 0 }, *heldType, dynamic.m_value(value), nullptr);
                    }

                default:
                    return true;
                }
            }
        };


        //! Searches the static structure of a type for a nested type matching a predicate.
        struct ContainmentSearch final
        {
            bool (*m_matches)(const Type& type, const void* context);
            const void* m_context;
            festd::inline_vector<const Type*, 16> m_stack;

            bool Contains(const Type& type)
            {
                if (m_matches(type, m_context) || type.m_shape == TypeShape::kDynamic)
                    return true;

                // A type that contains itself through a container adds nothing new while it is being searched.
                if (festd::find(m_stack, &type) != m_stack.end())
                    return false;

                m_stack.push_back(&type);
                const bool contains = ContainsChildren(type);
                m_stack.pop_back();
                return contains;
            }

        private:
            bool ContainsFields(const Type& type)
            {
                for (const FieldInfo& field : type.m_fields)
                {
                    if (Contains(*field.m_type))
                        return true;
                }

                return false;
            }


            bool ContainsChildren(const Type& type)
            {
                switch (type.m_shape)
                {
                case TypeShape::kStruct:
                    for (const TypeID baseId : type.m_baseTypes)
                    {
                        const Type* base = TypeRegistry::FindType(baseId);
                        if (base != nullptr && ContainsFields(*base))
                            return true;
                    }

                    return ContainsFields(type);

                case TypeShape::kFixedArray:
                case TypeShape::kSequence:
                    return Contains(*GetElementType(type));

                case TypeShape::kVariant:
                    for (const Type* alternative : type.TryGetShape<VariantShape>()->m_alternatives)
                    {
                        if (Contains(*alternative))
                            return true;
                    }

                    return false;

                default:
                    return false;
                }
            }
        };


        bool CopyLeaf(const Type& type, const void* source, void* destination)
        {
            if (type.m_copyAssignment == nullptr)
                return false;

            type.m_copyAssignment(destination, source);
            return true;
        }


        bool CopyFields(const Type& type, const void* source, void* destination, const VisitFlags flags)
        {
            for (const FieldInfo& field : type.m_fields)
            {
                if (!IsVisitedField(field, flags))
                    continue;

                if (!CopyValue(*field.m_type, field.GetAddress(source), field.GetAddress(destination), flags))
                    return false;
            }

            return true;
        }


        bool CopyStruct(const Type& type, const void* source, void* destination, const VisitFlags flags)
        {
            if (Bit::AnySet(type.m_flags, TypeFlags::kHasUnreflectedFields))
                return CopyLeaf(type, source, destination);

            for (const TypeID baseId : type.m_baseTypes)
            {
                const Type* base = TypeRegistry::FindType(baseId);
                const void* sourceBase = CastToBase(type, const_cast<void*>(source), baseId);
                void* destinationBase = CastToBase(type, destination, baseId);
                if (base == nullptr || sourceBase == nullptr || destinationBase == nullptr)
                    continue;

                if (Bit::AnySet(base->m_flags, TypeFlags::kHasUnreflectedFields))
                    return CopyLeaf(type, source, destination);

                if (!CopyFields(*base, sourceBase, destinationBase, flags))
                    return false;
            }

            return CopyFields(type, source, destination, flags);
        }


        bool CopyElements(const Type& type, const void* source, void* destination, const VisitFlags flags)
        {
            const uint32_t count = GetElementCount(type, source);
            if (const auto* sequence = type.TryGetShape<SequenceShape>())
            {
                if (!sequence->m_resize || !sequence->m_resize(destination, count))
                    return CopyLeaf(type, source, destination);
            }

            const Type& elementType = *GetElementType(type);
            for (uint32_t index = 0; index < count; ++index)
            {
                const void* sourceElement = GetElement(type, const_cast<void*>(source), index);
                if (!CopyValue(elementType, sourceElement, GetElement(type, destination, index), flags))
                    return false;
            }

            return true;
        }


        bool CopyVariant(const Type& type, const void* source, void* destination, const VisitFlags flags)
        {
            const auto& variant = *type.TryGetShape<VariantShape>();
            const uint32_t index = variant.m_index(source);
            if (index >= variant.m_alternatives.size())
                return CopyLeaf(type, source, destination);

            if (variant.m_index(destination) != index)
                variant.m_emplace(destination, index);

            const void* sourceValue = variant.m_get(const_cast<void*>(source));
            return CopyValue(*variant.m_alternatives[index], sourceValue, variant.m_get(destination), flags);
        }


        bool CopyDynamic(const Type& type, const void* source, void* destination, const VisitFlags flags)
        {
            const auto& dynamic = *type.TryGetShape<DynamicShape>();
            const Type* sourceType = dynamic.m_type(source);

            // Shared objects, e.g. two Rc to one instance, get a value of their own like a serialization round trip.
            const bool isShared =
                sourceType != nullptr && dynamic.m_value(const_cast<void*>(source)) == dynamic.m_value(destination);
            const bool needsReset = isShared || dynamic.m_type(destination) != sourceType;
            if (needsReset && !dynamic.m_reset(destination, sourceType))
                return false;

            if (sourceType == nullptr)
                return true;

            const void* sourceValue = dynamic.m_value(const_cast<void*>(source));
            return CopyValue(*sourceType, sourceValue, dynamic.m_value(destination), flags);
        }
    } // namespace


    const Type* GetElementType(const Type& type)
    {
        if (const auto* array = type.TryGetShape<FixedArrayShape>())
            return array->m_element;

        if (const auto* sequence = type.TryGetShape<SequenceShape>())
            return sequence->m_element;

        return nullptr;
    }


    ValueRef ResolveStep(const Type& type, void* value, const PathStep step)
    {
        switch (step.m_kind)
        {
        case PathStep::Kind::kBase:
            {
                if (step.m_index >= type.m_baseTypes.size())
                    return {};

                const TypeID baseId = type.m_baseTypes[step.m_index];
                return { TypeRegistry::FindType(baseId), CastToBase(type, value, baseId) };
            }

        case PathStep::Kind::kField:
            {
                if (type.m_shape != TypeShape::kStruct || step.m_index >= type.m_fields.size())
                    return {};

                const FieldInfo& field = type.m_fields[step.m_index];
                return { field.m_type, field.GetAddress(value) };
            }

        case PathStep::Kind::kElement:
            return { GetElementType(type), GetElement(type, value, step.m_index) };

        case PathStep::Kind::kAlternative:
            {
                const auto* variant = type.TryGetShape<VariantShape>();
                if (variant == nullptr || variant->m_index(value) != step.m_index)
                    return {};

                return { variant->m_alternatives[step.m_index], variant->m_get(value) };
            }

        case PathStep::Kind::kDynamic:
            {
                const auto* dynamic = type.TryGetShape<DynamicShape>();
                if (dynamic == nullptr)
                    return {};

                return { dynamic->m_type(value), dynamic->m_value(value) };
            }

        default:
            return {};
        }
    }


    ValueRef ResolvePath(const Type& type, void* value, const festd::span<const PathStep> path)
    {
        ValueRef current{ &type, value };
        for (const PathStep step : path)
        {
            current = ResolveStep(*current.m_type, current.m_value, step);
            if (!current)
                return {};
        }

        return current;
    }


    bool Visit(const Type& type, void* value, const VisitFlags flags, void* userData, const VisitCallback callback)
    {
        FE_Assert(value && callback);

        Visitor visitor{ flags, userData, callback, {} };
        return visitor.VisitValue(type, value, nullptr);
    }


    bool CanContainShape(const Type& type, const TypeShape shape)
    {
        const auto matches = [](const Type& candidate, const void* context) {
            return candidate.m_shape == *static_cast<const TypeShape*>(context);
        };

        ContainmentSearch search{ matches, &shape, {} };
        return search.Contains(type);
    }


    bool CanContainType(const Type& type, const Type& target)
    {
        const auto matches = [](const Type& candidate, const void* context) {
            return &candidate == context;
        };

        ContainmentSearch search{ matches, &target, {} };
        return search.Contains(type);
    }


    bool CopyValue(const Type& type, const void* source, void* destination, const VisitFlags flags)
    {
        FE_Assert(source && destination);
        FE_AssertDebug(source != destination);

        switch (type.m_shape)
        {
        case TypeShape::kStruct:
            return CopyStruct(type, source, destination, flags);

        case TypeShape::kFixedArray:
        case TypeShape::kSequence:
            return CopyElements(type, source, destination, flags);

        case TypeShape::kVariant:
            return CopyVariant(type, source, destination, flags);

        case TypeShape::kDynamic:
            return CopyDynamic(type, source, destination, flags);

        default:
            return CopyLeaf(type, source, destination);
        }
    }
} // namespace FE::Rtti

#include <Core/Math/Aabb.h>
#include <Core/Math/Color.h>
#include <Core/Math/Obb.h>
#include <Core/Math/Sphere.h>
#include <Core/Math/Transform.h>
#include <Core/Math/Vector3Int.h>
#include <Core/Math/Vector3UInt.h>
#include <Core/Utils/Icons.h>
#include <GameFramework/Editor/Controls.h>
#include <cmath>
#include <festd/string.h>
#include <imgui.h>

namespace FE::Editor
{
    namespace
    {
        template<class T, ImGuiDataType TDataType>
        bool DrawScalar(const char* label, void* value)
        {
            if constexpr (std::same_as<T, float>)
            {
                const float speed = Math::Max(std::abs(*static_cast<float*>(value)) * 0.01f, 0.000001f);
                return ImGui::DragScalar(label, TDataType, value, speed, nullptr, nullptr, "%.6g");
            }
            else
            {
                return ImGui::DragScalar(label, TDataType, static_cast<T*>(value), 0.05f);
            }
        }


        template<class T, uint32_t TCount, ImGuiDataType TDataType = ImGuiDataType_Float>
        bool DrawVector(const char* label, void* value)
        {
            if constexpr (TDataType == ImGuiDataType_Float)
            {
                const auto* values = static_cast<T*>(value)->m_values;
                float magnitude = 0;
                for (uint32_t index = 0; index < TCount; ++index)
                    magnitude = Math::Max(magnitude, std::abs(values[index]));

                const float speed = Math::Max(magnitude * 0.01f, 0.000001f);
                return ImGui::DragScalarN(label,
                                          TDataType,
                                          static_cast<T*>(value)->m_values,
                                          TCount,
                                          speed,
                                          nullptr,
                                          nullptr,
                                          "%.6g");
            }
            else
            {
                return ImGui::DragScalarN(label, TDataType, static_cast<T*>(value)->m_values, TCount, 0.05f);
            }
        }


        bool DrawBool(const char* label, void* value)
        {
            return ImGui::Checkbox(label, static_cast<bool*>(value));
        }


        bool DrawColor(const char* label, void* value)
        {
            return ImGui::ColorEdit4(label, static_cast<Color4F*>(value)->m_values, ImGuiColorEditFlags_Float);
        }


        bool DrawQuaternion(const char* label, void* value)
        {
            Quaternion candidate = *static_cast<Quaternion*>(value);
            if (!ImGui::DragFloat4(label, candidate.m_values, 0.01f))
                return false;

            const float lengthSquared = Math::LengthSquared(candidate);
            if (!std::isfinite(lengthSquared) || lengthSquared <= 0)
                return false;

            *static_cast<Quaternion*>(value) = Math::Normalize(candidate);
            return true;
        }


        bool DrawTransform(const char* label, void* value)
        {
            if (!ImGui::TreeNode(label))
                return false;

            auto& transform = *static_cast<Transform*>(value);
            bool changed = ImGui::DragFloat3("Translation", transform.m_translationScale.m_values, 0.05f);
            changed |= DrawQuaternion("Rotation", &transform.m_rotation);
            changed |= ImGui::DragFloat("Scale", &transform.m_translationScale.w, 0.01f);
            ImGui::TreePop();
            return changed;
        }


        bool DrawMatrix(const char* label, void* value)
        {
            if (!ImGui::TreeNode(label))
                return false;

            auto& matrix = *static_cast<Matrix4x4*>(value);
            bool changed = false;
            for (uint32_t index = 0; index < 4; ++index)
                changed |= ImGui::DragFloat4(Fmt::FixedFormat("Row {}", index).c_str(), matrix.m_rows[index].m_values, 0.05f);

            ImGui::TreePop();
            return changed;
        }


        template<class T>
        bool DrawBounds(const char* label, void* value)
        {
            if (!ImGui::TreeNode(label))
                return false;

            auto& bounds = *static_cast<T*>(value);
            bool changed = ImGui::DragFloat3("Min", bounds.min.m_values, 0.05f);
            changed |= ImGui::DragFloat3("Max", bounds.max.m_values, 0.05f);
            ImGui::TreePop();
            return changed;
        }


        bool DrawObb(const char* label, void* value)
        {
            if (!ImGui::TreeNode(label))
                return false;

            auto& bounds = *static_cast<Obb*>(value);
            bool changed = ImGui::DragFloat3("Center", bounds.center.m_values, 0.05f);
            changed |= ImGui::DragFloat3("Extents", bounds.extents.m_values, 0.05f);
            changed |= DrawQuaternion("Rotation", &bounds.rotation);
            ImGui::TreePop();
            return changed;
        }


        bool DrawSphere(const char* label, void* value)
        {
            if (!ImGui::TreeNode(label))
                return false;

            auto& sphere = *static_cast<Sphere*>(value);
            bool changed = ImGui::DragFloat3("Center", sphere.m_centerRadius.m_values, 0.05f);
            changed |= ImGui::DragFloat("Radius", &sphere.m_centerRadius.w, 0.05f);
            ImGui::TreePop();
            return changed;
        }


        using DrawControl = bool (*)(const char*, void*);


        struct Control final
        {
            Rtti::TypeID m_type;
            DrawControl m_draw;
        };


        //! Controls edit a byte copy of the value, so their types must be trivially copyable and fit an InspectorEdit.
        template<class T>
        Control MakeControl(const DrawControl draw)
        {
            static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= InspectorEdit::kMaxSize && alignof(T) <= 16);
            return { Rtti::GetTypeID<T>(), draw };
        }


        DrawControl FindControl(const Rtti::TypeID type)
        {
            static const Control kControls[] = {
                MakeControl<bool>(DrawBool),
                MakeControl<int8_t>(DrawScalar<int8_t, ImGuiDataType_S8>),
                MakeControl<uint8_t>(DrawScalar<uint8_t, ImGuiDataType_U8>),
                MakeControl<int16_t>(DrawScalar<int16_t, ImGuiDataType_S16>),
                MakeControl<uint16_t>(DrawScalar<uint16_t, ImGuiDataType_U16>),
                MakeControl<int32_t>(DrawScalar<int32_t, ImGuiDataType_S32>),
                MakeControl<uint32_t>(DrawScalar<uint32_t, ImGuiDataType_U32>),
                MakeControl<int64_t>(DrawScalar<int64_t, ImGuiDataType_S64>),
                MakeControl<uint64_t>(DrawScalar<uint64_t, ImGuiDataType_U64>),
                MakeControl<float>(DrawScalar<float, ImGuiDataType_Float>),
                MakeControl<double>(DrawScalar<double, ImGuiDataType_Double>),
                MakeControl<Vector2>(DrawVector<Vector2, 2>),
                MakeControl<Vector2Int>(DrawVector<Vector2Int, 2, ImGuiDataType_S32>),
                MakeControl<Vector2UInt>(DrawVector<Vector2UInt, 2, ImGuiDataType_U32>),
                MakeControl<Vector3>(DrawVector<Vector3, 3>),
                MakeControl<Vector3Int>(DrawVector<Vector3Int, 3, ImGuiDataType_S32>),
                MakeControl<Vector3UInt>(DrawVector<Vector3UInt, 3, ImGuiDataType_U32>),
                MakeControl<Vector4>(DrawVector<Vector4, 4>),
                MakeControl<PackedVector3F>(DrawVector<PackedVector3F, 3>),
                MakeControl<PackedVector3Int>(DrawVector<PackedVector3Int, 3, ImGuiDataType_S32>),
                MakeControl<PackedVector3UInt>(DrawVector<PackedVector3UInt, 3, ImGuiDataType_U32>),
                MakeControl<PackedVector4F>(DrawVector<PackedVector4F, 4>),
                MakeControl<RectF>(DrawVector<RectF, 4>),
                MakeControl<RectInt>(DrawVector<RectInt, 4, ImGuiDataType_S32>),
                MakeControl<RectUInt>(DrawVector<RectUInt, 4, ImGuiDataType_U32>),
                MakeControl<Color4F>(DrawColor),
                MakeControl<Quaternion>(DrawQuaternion),
                MakeControl<Transform>(DrawTransform),
                MakeControl<Matrix4x4>(DrawMatrix),
                MakeControl<Aabb>(DrawBounds<Aabb>),
                MakeControl<PackedAabb>(DrawBounds<PackedAabb>),
                MakeControl<Obb>(DrawObb),
                MakeControl<Sphere>(DrawSphere),
            };
            for (const auto& control : kControls)
            {
                if (control.m_type == type)
                    return control.m_draw;
            }

            return nullptr;
        }


        festd::ascii_view GetFieldLabel(const Rtti::FieldInfo& field)
        {
            const auto* display = field.TryGetAttribute<Rtti::DisplayName>();
            return display ? display->m_name : field.m_name;
        }


        const char* GetEnumName(const Rtti::Type& type, const int64_t value)
        {
            for (uint32_t index = 0; index < type.m_enumValues.size(); ++index)
            {
                if (type.m_enumValues[index] != value)
                    continue;

                const bool hasDisplayName = index < type.m_enumDisplayNames.size() && !type.m_enumDisplayNames[index].empty();
                return (hasDisplayName ? type.m_enumDisplayNames[index] : type.m_enumNames[index]).data();
            }

            return nullptr;
        }


        //! Types a dynamic value can hold: registered default-constructible classes derived from its base, or serializable ones.
        bool IsDynamicCandidate(const Rtti::Type& candidate, const Rtti::Type* base)
        {
            if (!candidate.m_defaultConstructor || !candidate.m_destructor || candidate.m_shape != Rtti::TypeShape::kStruct)
                return false;

            if (base == nullptr)
                return candidate.m_serialize != nullptr;

            if (&candidate == base)
                return true;

            return festd::find(candidate.m_baseTypes, base->m_id) != candidate.m_baseTypes.end();
        }


        //! Recursive drawing state; records the first edit and the path to the value it changes.
        struct Inspector final
        {
            InspectorEdit& m_edit;
            const InspectorContext& m_context;
            Rtti::ValuePath m_path;

            [[nodiscard]] bool HasEdit() const
            {
                return m_edit.m_kind != InspectorEdit::Kind::kNone;
            }


            void DrawRoot(const Rtti::Type& type, const void* value, const void* runtimeValue)
            {
                if (type.m_shape == Rtti::TypeShape::kStruct)
                    DrawStruct(type, value, runtimeValue, true);
                else
                    DrawValue("Value", type, value, false);
            }

        private:
            //! Begin an edit of the value at the current path; later edits in the same frame are dropped.
            bool BeginEdit(const InspectorEdit::Kind kind)
            {
                if (HasEdit())
                    return false;

                m_edit.m_kind = kind;
                m_edit.m_path = m_path;
                return true;
            }


            bool BeginTree(const char* label) const
            {
                if (m_context.m_expandAll)
                    ImGui::SetNextItemOpen(true);

                return ImGui::TreeNode(label);
            }


            void PushStep(const Rtti::PathStep::Kind kind, const uint32_t index)
            {
                m_path.push_back({ kind, index });
                ImGui::PushID(static_cast<int>(festd::to_underlying(kind) << 24 | index));
            }


            void PopStep()
            {
                ImGui::PopID();
                m_path.pop_back();
            }


            void DrawValue(const char* label, const Rtti::Type& type, const void* value, const bool readOnly)
            {
                ImGui::BeginDisabled(readOnly);
                if (const DrawControl draw = FindControl(type.m_id))
                    DrawLeaf(label, type, value, draw);
                else
                    DrawShape(label, type, value);

                ImGui::EndDisabled();
            }


            void DrawLeaf(const char* label, const Rtti::Type& type, const void* value, const DrawControl draw)
            {
                alignas(16) std::byte copy[InspectorEdit::kMaxSize];
                memcpy(copy, value, type.m_size);
                if (!draw(label, copy) || !BeginEdit(InspectorEdit::Kind::kAssign))
                    return;

                m_edit.m_size = type.m_size;
                memcpy(m_edit.m_value, copy, type.m_size);
            }


            void DrawShape(const char* label, const Rtti::Type& type, const void* value)
            {
                switch (type.m_shape)
                {
                case Rtti::TypeShape::kStruct:
                    if (BeginTree(label))
                    {
                        DrawStruct(type, value, nullptr, true);
                        ImGui::TreePop();
                    }
                    break;

                case Rtti::TypeShape::kEnum:
                    DrawEnum(label, type, value);
                    break;

                case Rtti::TypeShape::kString:
                    DrawString(label, *type.TryGetShape<Rtti::StringShape>(), value);
                    break;

                case Rtti::TypeShape::kHandle:
                    DrawHandle(label, type, value);
                    break;

                case Rtti::TypeShape::kFixedArray:
                case Rtti::TypeShape::kSequence:
                    DrawElements(label, type, value);
                    break;

                case Rtti::TypeShape::kVariant:
                    DrawVariant(label, *type.TryGetShape<Rtti::VariantShape>(), value);
                    break;

                case Rtti::TypeShape::kDynamic:
                    DrawDynamic(label, *type.TryGetShape<Rtti::DynamicShape>(), value);
                    break;

                default:
                    ImGui::TextDisabled("%s (unsupported type)", label);
                    break;
                }
            }


            //! Bases are flattened into the most derived type, so a base is drawn with its own fields only.
            void DrawStruct(const Rtti::Type& type, const void* value, const void* runtimeValue, const bool includeBases)
            {
                if (includeBases)
                {
                    for (uint32_t baseIndex = 0; baseIndex < type.m_baseTypes.size(); ++baseIndex)
                        DrawBase(type, baseIndex, value);
                }

                for (uint32_t fieldIndex = 0; fieldIndex < type.m_fields.size(); ++fieldIndex)
                {
                    const Rtti::FieldInfo& field = type.m_fields[fieldIndex];
                    if (!Bit::AnySet(field.m_flags, Rtti::FieldFlags::kInstance))
                        continue;

                    // Runtime state is shown from the live value and cannot be edited.
                    const bool isTransient = Bit::AnySet(field.m_flags, Rtti::FieldFlags::kTransient);
                    const void* storage = isTransient && runtimeValue ? runtimeValue : value;
                    const festd::inline_string label(GetFieldLabel(field));

                    PushStep(Rtti::PathStep::Kind::kField, fieldIndex);
                    if (Bit::AnySet(field.m_flags, Rtti::FieldFlags::kPointer))
                        ImGui::TextDisabled("%s (pointer)", label.c_str());
                    else
                        DrawValue(label.c_str(), *field.m_type, field.GetAddress(storage), isTransient);

                    PopStep();
                }
            }


            void DrawBase(const Rtti::Type& type, const uint32_t baseIndex, const void* value)
            {
                const Rtti::PathStep step{ Rtti::PathStep::Kind::kBase, baseIndex };
                const Rtti::ValueRef base = Rtti::ResolveStep(type, const_cast<void*>(value), step);
                if (!base || base.m_type->m_fields.empty())
                    return;

                PushStep(step.m_kind, step.m_index);
                if (BeginTree(festd::inline_string(GetDisplayName(*base.m_type)).c_str()))
                {
                    DrawStruct(*base.m_type, base.m_value, nullptr, false);
                    ImGui::TreePop();
                }

                PopStep();
            }


            void DrawEnum(const char* label, const Rtti::Type& type, const void* value)
            {
                const int64_t current = type.TryGetShape<Rtti::EnumShape>()->m_get(value);
                if (type.m_enumValues.empty())
                {
                    int64_t candidate = current;
                    if (ImGui::InputScalar(label, ImGuiDataType_S64, &candidate) && candidate != current)
                        AssignEnum(type, value, candidate);

                    return;
                }

                const char* currentName = GetEnumName(type, current);
                const festd::fixed_string unknown = Fmt::FixedFormat("{}", current);
                if (!ImGui::BeginCombo(label, currentName ? currentName : unknown.c_str()))
                    return;

                for (const int64_t option : type.m_enumValues)
                {
                    if (ImGui::Selectable(GetEnumName(type, option), option == current) && option != current)
                        AssignEnum(type, value, option);
                }

                ImGui::EndCombo();
            }


            void AssignEnum(const Rtti::Type& type, const void* value, const int64_t underlying)
            {
                if (!BeginEdit(InspectorEdit::Kind::kAssign))
                    return;

                m_edit.m_size = type.m_size;
                memcpy(m_edit.m_value, value, type.m_size);
                type.TryGetShape<Rtti::EnumShape>()->m_set(m_edit.m_value, underlying);
            }


            void DrawString(const char* label, const Rtti::StringShape& shape, const void* value)
            {
                const std::string_view text = shape.m_get(value);
                char input[256];
                const size_t size = Math::Min(text.size(), sizeof(input) - 1);
                memcpy(input, text.data(), size);
                input[size] = '\0';

                const ImGuiInputTextFlags flags = shape.m_set ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_ReadOnly;
                if (!ImGui::InputText(label, input, sizeof(input), flags) || !BeginEdit(InspectorEdit::Kind::kSetString))
                    return;

                m_edit.m_text = festd::fixed_string{ input, static_cast<uint32_t>(strlen(input)) };
            }


            void DrawHandle(const char* label, const Rtti::Type& type, const void* value)
            {
                const Uuid current = type.TryGetShape<Rtti::HandleShape>()->m_get(value);
                if (const auto* reference = type.TryGetAttribute<IO::AssetReference>())
                {
                    DrawAssetPicker(label, reference->m_getAssetType(), current);
                    return;
                }

                const festd::fixed_string text = current.IsValid() ? Fmt::FixedFormat("{}", current) : festd::fixed_string{};
                char input[64];
                memcpy(input, text.data(), text.size());
                input[text.size()] = '\0';

                const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank;
                if (ImGui::InputTextWithHint(label, "None", input, sizeof(input), flags))
                    SetHandle(Uuid::Parse(festd::ascii_view{ input, static_cast<uint32_t>(strlen(input)) }), current);
            }


            void DrawAssetPicker(const char* label, const Rtti::TypeID assetType, const Uuid current)
            {
                const AssetEntry* selected = nullptr;
                for (const AssetEntry& asset : m_context.m_assets)
                {
                    if (asset.m_id == current)
                        selected = &asset;
                }

                // References to assets outside the browsed directory still show their identity.
                const festd::fixed_string missing = Fmt::FixedFormat("{}", current);
                const char* preview = selected ? selected->m_name.c_str() : (current.IsValid() ? missing.c_str() : "None");
                if (!ImGui::BeginCombo(label, preview))
                    return;

                if (ImGui::Selectable("None", !current.IsValid()))
                    SetHandle(Uuid::kNull, current);

                for (const AssetEntry& asset : m_context.m_assets)
                {
                    if (asset.m_type != assetType)
                        continue;

                    ImGui::PushID(Fmt::FixedFormat("{}", asset.m_id).c_str());
                    if (ImGui::Selectable(asset.m_name.c_str(), asset.m_id == current))
                        SetHandle(asset.m_id, current);

                    ImGui::SetItemTooltip("%s", asset.m_path.AsBaseString().c_str());
                    ImGui::PopID();
                }

                ImGui::EndCombo();
            }


            void SetHandle(const Uuid handle, const Uuid current)
            {
                if (handle != current && BeginEdit(InspectorEdit::Kind::kSetHandle))
                    m_edit.m_handle = handle;
            }


            void DrawElements(const char* label, const Rtti::Type& type, const void* value)
            {
                const auto* sequence = type.TryGetShape<Rtti::SequenceShape>();
                const uint32_t count = sequence ? sequence->m_size(value) : type.TryGetShape<Rtti::FixedArrayShape>()->m_count;
                const festd::fixed_string header = Fmt::FixedFormat("{} [{}]###{}", label, count, label);
                if (!BeginTree(header.c_str()))
                    return;

                const Rtti::Type& elementType = *Rtti::GetElementType(type);
                for (uint32_t index = 0; index < count; ++index)
                {
                    const Rtti::PathStep step{ Rtti::PathStep::Kind::kElement, index };
                    const Rtti::ValueRef element = Rtti::ResolveStep(type, const_cast<void*>(value), step);

                    if (sequence && sequence->m_erase)
                        DrawEraseButton(index);

                    PushStep(step.m_kind, step.m_index);
                    DrawValue(Fmt::FixedFormat("[{}]", index).c_str(), elementType, element.m_value, false);
                    PopStep();
                }

                const bool canInsert = sequence && sequence->m_insert;
                if (canInsert && ImGui::SmallButton(FE_ICON_ADD " Add") && BeginEdit(InspectorEdit::Kind::kInsertElement))
                    m_edit.m_index = count;

                ImGui::TreePop();
            }


            void DrawEraseButton(const uint32_t index)
            {
                ImGui::PushID(static_cast<int>(index));
                if (ImGui::SmallButton(FE_ICON_REMOVE) && BeginEdit(InspectorEdit::Kind::kEraseElement))
                    m_edit.m_index = index;

                ImGui::SetItemTooltip("Remove");
                ImGui::PopID();
                ImGui::SameLine();
            }


            void DrawVariant(const char* label, const Rtti::VariantShape& shape, const void* value)
            {
                const uint32_t current = shape.m_index(value);
                const char* preview = current < shape.m_alternatives.size() ? shape.m_alternatives[current]->m_name.data() : "";
                if (ImGui::BeginCombo(label, preview))
                {
                    for (uint32_t index = 0; index < shape.m_alternatives.size(); ++index)
                    {
                        ImGui::PushID(static_cast<int>(index));
                        const bool isSelected = index == current;
                        if (ImGui::Selectable(shape.m_alternatives[index]->m_name.data(), isSelected) && !isSelected)
                            SetAlternative(index);

                        ImGui::PopID();
                    }

                    ImGui::EndCombo();
                }

                if (current >= shape.m_alternatives.size())
                    return;

                ImGui::Indent();
                PushStep(Rtti::PathStep::Kind::kAlternative, current);
                DrawValue("Value", *shape.m_alternatives[current], shape.m_get(const_cast<void*>(value)), false);
                PopStep();
                ImGui::Unindent();
            }


            void SetAlternative(const uint32_t index)
            {
                if (BeginEdit(InspectorEdit::Kind::kSetAlternative))
                    m_edit.m_index = index;
            }


            void DrawDynamic(const char* label, const Rtti::DynamicShape& shape, const void* value)
            {
                const Rtti::Type* current = shape.m_type(value);
                const festd::inline_string preview(current ? GetDisplayName(*current) : festd::ascii_view{ "None" });
                if (ImGui::BeginCombo(label, preview.c_str()))
                {
                    if (ImGui::Selectable("None", current == nullptr) && current != nullptr)
                        SetDynamicType(nullptr);

                    for (const Rtti::Type& candidate : Rtti::TypeRegistry::GetTypes())
                    {
                        if (!IsDynamicCandidate(candidate, shape.m_base))
                            continue;

                        ImGui::PushID(&candidate);
                        const bool isSelected = &candidate == current;
                        if (ImGui::Selectable(festd::inline_string(GetDisplayName(candidate)).c_str(), isSelected) && !isSelected)
                            SetDynamicType(&candidate);

                        ImGui::PopID();
                    }

                    ImGui::EndCombo();
                }

                if (current == nullptr)
                    return;

                void* heldValue = shape.m_value(const_cast<void*>(value));
                ImGui::Indent();
                PushStep(Rtti::PathStep::Kind::kDynamic, 0);
                if (current->m_shape == Rtti::TypeShape::kStruct)
                    DrawStruct(*current, heldValue, nullptr, true);
                else
                    DrawValue("Value", *current, heldValue, false);

                PopStep();
                ImGui::Unindent();
            }


            void SetDynamicType(const Rtti::Type* type)
            {
                if (BeginEdit(InspectorEdit::Kind::kSetDynamicType))
                    m_edit.m_dynamicType = type;
            }
        };
    } // namespace


    bool InspectorEdit::Apply(const Rtti::Type& type, void* value) const
    {
        const Rtti::ValueRef target = Rtti::ResolvePath(type, value, m_path);
        if (!target)
            return false;

        switch (m_kind)
        {
        case Kind::kAssign:
            {
                if (target.m_type->m_size != m_size)
                    return false;

                memcpy(target.m_value, m_value, m_size);
                return true;
            }

        case Kind::kSetString:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::StringShape>();
                if (shape == nullptr || shape->m_set == nullptr)
                    return false;

                shape->m_set(target.m_value, std::string_view{ m_text.data(), m_text.size() });
                return true;
            }

        case Kind::kSetHandle:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::HandleShape>();
                if (shape == nullptr)
                    return false;

                shape->m_set(target.m_value, m_handle);
                return true;
            }

        case Kind::kInsertElement:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::SequenceShape>();
                if (shape == nullptr || shape->m_insert == nullptr || m_index > shape->m_size(target.m_value))
                    return false;

                return shape->m_insert(target.m_value, m_index);
            }

        case Kind::kEraseElement:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::SequenceShape>();
                if (shape == nullptr || m_index >= shape->m_size(target.m_value))
                    return false;

                shape->m_erase(target.m_value, m_index);
                return true;
            }

        case Kind::kSetAlternative:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::VariantShape>();
                if (shape == nullptr || m_index >= shape->m_alternatives.size())
                    return false;

                shape->m_emplace(target.m_value, m_index);
                return true;
            }

        case Kind::kSetDynamicType:
            {
                const auto* shape = target.m_type->TryGetShape<Rtti::DynamicShape>();
                return shape != nullptr && shape->m_reset(target.m_value, m_dynamicType);
            }

        default:
            return false;
        }
    }


    bool DrawReflectedValue(const Rtti::Type& type, const void* value, InspectorEdit& edit, const InspectorContext& context,
                            const bool readOnly, const void* runtimeValue)
    {
        edit = {};
        Inspector inspector{ edit, context, {} };
        ImGui::BeginDisabled(readOnly);
        inspector.DrawRoot(type, value, runtimeValue);
        ImGui::EndDisabled();
        return inspector.HasEdit() && !readOnly;
    }


    festd::ascii_view GetDisplayName(const Rtti::Type& type)
    {
        if (const auto* display = type.TryGetAttribute<Rtti::DisplayName>())
            return display->m_name;

        return type.m_name;
    }


    festd::ascii_view GetIcon(const Rtti::Type* type)
    {
        if (type)
        {
            if (const auto* icon = type->TryGetAttribute<Rtti::Icon>())
                return icon->m_glyph;
        }

        return FE_ICON_COMPONENT;
    }
} // namespace FE::Editor

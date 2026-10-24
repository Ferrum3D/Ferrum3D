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

        DrawControl FindControl(const Rtti::TypeID type)
        {
            struct Control final
            {
                Rtti::TypeID m_type;
                DrawControl m_draw;
            };

            static const Control kControls[] = {
                { Rtti::GetTypeID<bool>(), DrawBool },
                { Rtti::GetTypeID<int8_t>(), DrawScalar<int8_t, ImGuiDataType_S8> },
                { Rtti::GetTypeID<uint8_t>(), DrawScalar<uint8_t, ImGuiDataType_U8> },
                { Rtti::GetTypeID<int16_t>(), DrawScalar<int16_t, ImGuiDataType_S16> },
                { Rtti::GetTypeID<uint16_t>(), DrawScalar<uint16_t, ImGuiDataType_U16> },
                { Rtti::GetTypeID<int32_t>(), DrawScalar<int32_t, ImGuiDataType_S32> },
                { Rtti::GetTypeID<uint32_t>(), DrawScalar<uint32_t, ImGuiDataType_U32> },
                { Rtti::GetTypeID<int64_t>(), DrawScalar<int64_t, ImGuiDataType_S64> },
                { Rtti::GetTypeID<uint64_t>(), DrawScalar<uint64_t, ImGuiDataType_U64> },
                { Rtti::GetTypeID<float>(), DrawScalar<float, ImGuiDataType_Float> },
                { Rtti::GetTypeID<double>(), DrawScalar<double, ImGuiDataType_Double> },
                { Rtti::GetTypeID<Vector2>(), DrawVector<Vector2, 2> },
                { Rtti::GetTypeID<Vector2Int>(), DrawVector<Vector2Int, 2, ImGuiDataType_S32> },
                { Rtti::GetTypeID<Vector2UInt>(), DrawVector<Vector2UInt, 2, ImGuiDataType_U32> },
                { Rtti::GetTypeID<Vector3>(), DrawVector<Vector3, 3> },
                { Rtti::GetTypeID<Vector3Int>(), DrawVector<Vector3Int, 3, ImGuiDataType_S32> },
                { Rtti::GetTypeID<Vector3UInt>(), DrawVector<Vector3UInt, 3, ImGuiDataType_U32> },
                { Rtti::GetTypeID<Vector4>(), DrawVector<Vector4, 4> },
                { Rtti::GetTypeID<PackedVector3F>(), DrawVector<PackedVector3F, 3> },
                { Rtti::GetTypeID<PackedVector3Int>(), DrawVector<PackedVector3Int, 3, ImGuiDataType_S32> },
                { Rtti::GetTypeID<PackedVector3UInt>(), DrawVector<PackedVector3UInt, 3, ImGuiDataType_U32> },
                { Rtti::GetTypeID<PackedVector4F>(), DrawVector<PackedVector4F, 4> },
                { Rtti::GetTypeID<RectF>(), DrawVector<RectF, 4> },
                { Rtti::GetTypeID<RectInt>(), DrawVector<RectInt, 4, ImGuiDataType_S32> },
                { Rtti::GetTypeID<RectUInt>(), DrawVector<RectUInt, 4, ImGuiDataType_U32> },
                { Rtti::GetTypeID<Color4F>(), DrawColor },
                { Rtti::GetTypeID<Quaternion>(), DrawQuaternion },
                { Rtti::GetTypeID<Transform>(), DrawTransform },
                { Rtti::GetTypeID<Matrix4x4>(), DrawMatrix },
                { Rtti::GetTypeID<Aabb>(), DrawBounds<Aabb> },
                { Rtti::GetTypeID<PackedAabb>(), DrawBounds<PackedAabb> },
                { Rtti::GetTypeID<Obb>(), DrawObb },
                { Rtti::GetTypeID<Sphere>(), DrawSphere },
            };
            for (const auto& control : kControls)
            {
                if (control.m_type == type)
                    return control.m_draw;
            }

            return nullptr;
        }


        bool DrawFields(const Rtti::Type& type, void* value, const bool includeBases, const void* runtimeValue)
        {
            bool changed = false;
            if (includeBases && type.m_cast)
            {
                for (const auto baseId : type.m_baseTypes)
                {
                    const auto* base = Rtti::TypeRegistry::FindType(baseId);
                    void* baseValue = type.m_cast(value, baseId);
                    if (!base || !baseValue)
                        continue;

                    ImGui::PushID(Fmt::FixedFormat("{}", baseId).c_str());
                    if (ImGui::TreeNode(festd::inline_string(GetDisplayName(*base)).c_str()))
                    {
                        const void* runtimeBase = runtimeValue ? type.m_cast(const_cast<void*>(runtimeValue), baseId) : nullptr;
                        changed |= DrawFields(*base, baseValue, false, runtimeBase);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }

            for (const auto& field : type.m_fields)
            {
                if (!Bit::AnySet(field.m_flags, Rtti::FieldFlags::kInstance))
                    continue;

                ImGui::PushID(static_cast<int>(field.m_offset));
                const auto* display = field.TryGetAttribute<Rtti::DisplayName>();
                const festd::inline_string name(display ? display->m_name : field.m_name);
                const bool transient = Bit::AnySet(field.m_flags, Rtti::FieldFlags::kTransient);
                const auto* storage = transient && runtimeValue ? runtimeValue : value;
                void* fieldValue = const_cast<std::byte*>(static_cast<const std::byte*>(storage)) + field.m_offset;
                const bool isSingleValue = field.m_arraySize == 1 && !Bit::AnySet(field.m_flags, Rtti::FieldFlags::kPointer);
                const auto draw = isSingleValue ? FindControl(field.m_type) : nullptr;
                if (draw)
                {
                    ImGui::BeginDisabled(transient);
                    changed |= draw(name.c_str(), fieldValue);
                    ImGui::EndDisabled();
                }
                else
                {
                    const Rtti::Type* nested = isSingleValue ? Rtti::TypeRegistry::FindType(field.m_type) : nullptr;
                    if (nested && !nested->m_fields.empty())
                    {
                        ImGui::BeginDisabled(transient);
                        if (ImGui::TreeNode(name.c_str()))
                        {
                            const void* nestedRuntime =
                                runtimeValue ? static_cast<const std::byte*>(runtimeValue) + field.m_offset : nullptr;
                            changed |= DrawFields(*nested, fieldValue, true, nestedRuntime);
                            ImGui::TreePop();
                        }

                        ImGui::EndDisabled();
                    }
                    else
                    {
                        ImGui::TextDisabled("%s (unsupported type)", name.c_str());
                    }
                }

                ImGui::PopID();
            }

            return changed;
        }
    } // namespace


    bool DrawReflectedFields(const Rtti::Type& type, void* value, const bool readOnly, const void* runtimeValue)
    {
        ImGui::BeginDisabled(readOnly);
        const bool changed = DrawFields(type, value, true, runtimeValue);
        ImGui::EndDisabled();
        return changed && !readOnly;
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

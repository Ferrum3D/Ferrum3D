#include <AssetBuilder/Reflection.gen.h>
#include <Core/IO/AssetFile.h>
#include <Core/RTTI/ReflectionContext.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Editor/Controls.h>
#include <GameFramework/Reflection.gen.h>
#include <Graphics/Reflection.gen.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <imgui.h>

using namespace FE;

namespace
{
    using Values = festd::vector<festd::variant<int32_t, festd::string>>;


    Editor::InspectorEdit MakeEdit(const Editor::InspectorEdit::Kind kind, const std::initializer_list<Rtti::PathStep> path,
                                   const uint32_t index = 0)
    {
        Editor::InspectorEdit edit;
        edit.m_kind = kind;
        edit.m_path.assign(path.begin(), path.end());
        edit.m_index = index;
        return edit;
    }


    const Rtti::FieldInfo& FindField(const Rtti::Type& type, const festd::ascii_view name)
    {
        for (const Rtti::FieldInfo& field : type.m_fields)
        {
            if (field.m_name == name)
                return field;
        }

        FE_DebugBreak();
        return type.m_fields[0];
    }
} // namespace


TEST(EditorInspector, EditsApplyThroughValuePaths)
{
    const Rtti::Type& type = Rtti::ResolveType<Values>();
    Values values;

    ASSERT_TRUE(MakeEdit(Editor::InspectorEdit::Kind::kInsertElement, {}, 0).Apply(type, &values));
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0].index(), 0u);

    const Rtti::PathStep element{ Rtti::PathStep::Kind::kElement, 0 };
    ASSERT_TRUE(MakeEdit(Editor::InspectorEdit::Kind::kSetAlternative, { element }, 1).Apply(type, &values));
    ASSERT_EQ(values[0].index(), 1u);

    Editor::InspectorEdit text =
        MakeEdit(Editor::InspectorEdit::Kind::kSetString, { element, { Rtti::PathStep::Kind::kAlternative, 1 } });
    text.m_text = "hello";
    ASSERT_TRUE(text.Apply(type, &values));
    EXPECT_EQ(festd::get<festd::string>(values[0]), "hello");

    // An edit recorded for the previous alternative no longer resolves.
    Editor::InspectorEdit stale =
        MakeEdit(Editor::InspectorEdit::Kind::kAssign, { element, { Rtti::PathStep::Kind::kAlternative, 0 } });
    stale.m_size = sizeof(int32_t);
    EXPECT_FALSE(stale.Apply(type, &values));

    ASSERT_TRUE(MakeEdit(Editor::InspectorEdit::Kind::kEraseElement, {}, 0).Apply(type, &values));
    EXPECT_TRUE(values.empty());
    EXPECT_FALSE(MakeEdit(Editor::InspectorEdit::Kind::kEraseElement, {}, 0).Apply(type, &values));
}


TEST(EditorInspector, DynamicValuesChangeTypeAndEditTheirFields)
{
    const Rtti::Type& componentType = Rtti::GetType<GameFramework::TransformComponent>();
    Rtti::Any value;

    Editor::InspectorEdit setType = MakeEdit(Editor::InspectorEdit::Kind::kSetDynamicType, {});
    setType.m_dynamicType = &componentType;
    ASSERT_TRUE(setType.Apply(Rtti::GetType<Rtti::Any>(), &value));
    ASSERT_NE(value.TryGet<GameFramework::TransformComponent>(), nullptr);

    const Rtti::FieldInfo& local = FindField(componentType, "m_local");
    const uint32_t fieldIndex = static_cast<uint32_t>(&local - componentType.m_fields.data());
    Editor::InspectorEdit assign =
        MakeEdit(Editor::InspectorEdit::Kind::kAssign,
                 { { Rtti::PathStep::Kind::kDynamic, 0 }, { Rtti::PathStep::Kind::kField, fieldIndex } });
    const Transform transform = Transform::Translation(Vector3(1, 2, 3));
    assign.m_size = sizeof(Transform);
    memcpy(assign.m_value, &transform, sizeof(Transform));
    ASSERT_TRUE(assign.Apply(Rtti::GetType<Rtti::Any>(), &value));
    EXPECT_EQ(value.TryGet<GameFramework::TransformComponent>()->m_local, transform);

    Editor::InspectorEdit clear = MakeEdit(Editor::InspectorEdit::Kind::kSetDynamicType, {});
    ASSERT_TRUE(clear.Apply(Rtti::GetType<Rtti::Any>(), &value));
    EXPECT_FALSE(value.HasValue());
}


TEST(EditorInspector, HandlesAndAssetReferencesAreEditable)
{
    const Rtti::Type& type = Rtti::ResolveType<IO::Link<Framework::EntityWorldAsset>>();
    ASSERT_NE(type.TryGetAttribute<IO::AssetReference>(), nullptr);

    IO::Link<Framework::EntityWorldAsset> link;
    Editor::InspectorEdit edit = MakeEdit(Editor::InspectorEdit::Kind::kSetHandle, {});
    edit.m_handle = Uuid::Random();
    ASSERT_TRUE(edit.Apply(type, &link));
    EXPECT_EQ(link.GetAssetID(), edit.m_handle);
}


TEST(EditorInspector, DrawsAuthoredWorldDefinitionWithoutEditing)
{
    CallLinkerAnchor_GameFramework();
    CallLinkerAnchor_Graphics();
    CallLinkerAnchor_AssetBuilder();

    const std::filesystem::path path =
        std::filesystem::path(__FILE__).parent_path() / "../../../../Samples/GameSample/Assets/World.asset";
    IO::AssetFile file;
    ASSERT_TRUE(IO::LoadAssetDefinition(IO::Path(path.generic_string().c_str()), file));
    ASSERT_FALSE(file.m_artifacts.empty());
    const Rtti::Any& definition = file.m_artifacts.front().m_buildSettings;
    ASSERT_TRUE(definition.Is<Framework::EntityWorldAsset>());

    ImGui::SetAllocatorFunctions(
        [](const size_t size, void*) {
            return Memory::DefaultAllocate(size);
        },
        [](void* pointer, void*) {
            Memory::DefaultFree(pointer);
        });
    ImGuiContext* context = ImGui::CreateContext();
    const auto destroy = festd::defer([context] {
        ImGui::DestroyContext(context);
    });

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800, 600);
    io.DeltaTime = 1.0f / 60.0f;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    for (uint32_t frame = 0; frame < 2; ++frame)
    {
        ImGui::NewFrame();
        ImGui::Begin("Inspector");

        // Every nested value is drawn, which covers all shapes used by the world definition.
        Editor::InspectorContext inspector;
        inspector.m_expandAll = true;
        Editor::InspectorEdit edit;
        EXPECT_FALSE(Editor::DrawReflectedValue(*definition.GetType(), definition.GetValue(), edit, inspector));
        EXPECT_EQ(edit.m_kind, Editor::InspectorEdit::Kind::kNone);

        ImGui::End();
        ImGui::EndFrame();
    }
}

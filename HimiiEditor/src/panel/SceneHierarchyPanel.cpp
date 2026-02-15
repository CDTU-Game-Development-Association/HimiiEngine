#include "SceneHierarchyPanel.h"
#include "Himii/Project/Project.h"

#include "Himii/Scripting/ScriptEngine.h"
#include "Himii/Scene/TileSet.h"
#include "Himii/Scene/TileMapData.h"
#include "Himii/Asset/AssetSerializer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <filesystem>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace Himii
{
    extern const std::filesystem::path s_AssetsPath;
    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        // Load Component Icons
        m_ComponentIcons["Transform"] = Texture2D::Create("resources/icons/Component_Transform.png");
        m_ComponentIcons["Camera"] = Texture2D::Create("resources/icons/Component_Camera.png");
        m_ComponentIcons["Script"] = Texture2D::Create("resources/icons/Component_Script.png");
        m_ComponentIcons["Sprite Renderer"] = Texture2D::Create("resources/icons/Component_SpriteRenderer.png");
        m_ComponentIcons["Circle Renderer"] = Texture2D::Create("resources/icons/Component_CircleRenderer.png");
        m_ComponentIcons["Rigidbody2D"] = Texture2D::Create("resources/icons/Component_Rigidbody.png");
        m_ComponentIcons["Box Collider2D"] = Texture2D::Create("resources/icons/Component_BoxCollider.png");
        m_ComponentIcons["Circle Collider2D"] = Texture2D::Create("resources/icons/Component_CircleCollider.png");
        m_ComponentIcons["Sprite Animation"] = Texture2D::Create("resources/icons/Component_Animator.png");
        // Using SpriteRenderer icon as placeholder, or generate new one. For now re-use or use nullptr
        m_ComponentIcons["Mesh Renderer"] = Texture2D::Create("resources/icons/Component_SpriteRenderer.png"); 
    }

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
        : SceneHierarchyPanel()
    {
        SetContext(context);
    }
    void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
    {
        m_Context = context;
    }
    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);

        if (m_Context)
        {
            const auto &view = m_Context->m_Registry.view<TagComponent>();
            for (auto entity: view)
            {
                Entity e{entity, m_Context.get()};
                DrawEntityNode(e);
            }
        }

        if (ImGui::BeginPopupContextWindow(0, 1))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                m_Context->CreateEntity("Empty Entity");
            }
            if (ImGui::MenuItem("Create Cube"))
            {
                auto entity = m_Context->CreateEntity("Cube");
                entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Cube;
            }
            if (ImGui::MenuItem("Create Sphere"))
            {
                auto entity = m_Context->CreateEntity("Sphere");
                entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Sphere;
            }
            if (ImGui::MenuItem("Create Capsule"))
            {
                auto entity = m_Context->CreateEntity("Capsule");
                entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Capsule;
            }
            ImGui::EndPopup();
        }
        ImGui::End();

        ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse);

        if (m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);

            if (ImGui::BeginPopupContextWindow(0, 1))
            {
                DisplayAddComponentEntry<CameraComponent>("Camera");
                DisplayAddComponentEntry<ScriptComponent>("Script Component");
                DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
                DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
                DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody2D");
                DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider2D");
                DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider2D");
                DisplayAddComponentEntry<MeshComponent>("Mesh Renderer");

                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto &tag = entity.GetComponent<TagComponent>().Tag;

        ImGuiTreeNodeFlags flags =
                ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        flags |= ImGuiTreeNodeFlags_Leaf; // Fix: Treat as leaf node preventing arrow
        bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
            {
                entityDeleted = true;
            }
            ImGui::EndPopup();
        }

        if (opened)
        {
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            m_Context->DestroyEntity(entity);
            if (m_SelectionContext == entity)
                m_SelectionContext = {};
        }
    }

    static void DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f,
                                float columnWidth = 100.0f)
    {
        ImGuiIO &io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());
        
        // Use true for border to create the separator
        if (ImGui::BeginTable(label.c_str(), 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
             ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
             ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
             // ImGui::TableHeadersRow();

             ImGui::TableNextColumn();
             ImGui::Text("%s", label.c_str());
             ImGui::TableNextColumn();


             ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

             float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
             ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
             float widthEach = (ImGui::GetContentRegionAvail().x - 3 * buttonSize.x) / 3.0f;

             // X
             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.23f, 0.12f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
             ImGui::PushFont(boldFont);
             if (ImGui::Button("X", buttonSize))
                 values.x = resetValue;
             ImGui::PopFont();
             ImGui::PopStyleColor(3);

             ImGui::SameLine();
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
             ImGui::SameLine();

             // Y
             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.12f, 0.7f, 0.2f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
             ImGui::PushFont(boldFont);
             if (ImGui::Button("Y", buttonSize))
                 values.y = resetValue;
             ImGui::PopFont();
             ImGui::PopStyleColor(3);

             ImGui::SameLine();
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
             ImGui::SameLine();

             // Z
             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.13f, 0.4f, 0.8f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
             ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
             ImGui::PushFont(boldFont);
             if (ImGui::Button("Z", buttonSize))
                 values.z = resetValue;
             ImGui::PopFont();
             ImGui::PopStyleColor(3);

             ImGui::SameLine();
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");

             ImGui::PopStyleVar();

             ImGui::EndTable();
        }

        ImGui::PopID();
    }

    static void DrawFloatControl(const std::string& label, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##FloatControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Value", &value, speed, min, max);
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    static void DrawCheckboxControl(const std::string& label, bool& value, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##CheckboxControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::Checkbox("##Value", &value);
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    static void DrawColorControl(const std::string& label, glm::vec4& value, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##ColorControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit4("##Value", glm::value_ptr(value));
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string &name, Entity entity, Ref<Texture2D> icon, UIFunction uiFunction)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                 ImGuiTreeNodeFlags_SpanAvailWidth |
                                                 ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (entity.HasComponent<T>())
        {
            auto &component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            
        // --- Custom Icon Header ---
            ImGui::PushID(name.c_str());
            
            bool open = ImGui::GetStateStorage()->GetInt(ImGui::GetID("IsOpen"), 1);

            // Calculate start pos
            ImVec2 cursorPos = ImGui::GetCursorScreenPos(); // Screen Space
            ImVec2 elementPos = ImGui::GetCursorPos();      // Window Space (for restoring)
            
            // 1. Draw Background (Lighter color)
            ImU32 headerColor = ImGui::GetColorU32(ImVec4{0.27f, 0.275f, 0.28f, 1.0f}); // Brighter gray
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos, 
                ImVec2(cursorPos.x + contentRegionAvailable.x, cursorPos.y + lineHeight), 
                headerColor
            );

            // 2. Draw Selectable (Input handling)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0)); 
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0)); 
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0)); 

            if (ImGui::Selectable("##Header", false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(contentRegionAvailable.x, lineHeight)))
            {
                open = !open;
                ImGui::GetStateStorage()->SetInt(ImGui::GetID("IsOpen"), open ? 1 : 0);
            }
            ImGui::PopStyleColor(3);

            // 3. Draw Arrow & Icon & Text (Overlay)
            // Reset cursor to start of the item to draw on top
            ImGui::SetCursorPos(ImVec2(elementPos.x + ImGui::GetStyle().WindowPadding.x, elementPos.y));

            // Arrow
            ImGui::RenderArrow(ImGui::GetWindowDrawList(), 
                ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + (lineHeight - GImGui->FontSize)/2), 
                ImGui::GetColorU32(ImGuiCol_Text), 
                open ? ImGuiDir_Down : ImGuiDir_Right
            );
            
            // Move cursor past arrow
            ImGui::SetCursorPos(ImVec2(elementPos.x + ImGui::GetStyle().WindowPadding.x + GImGui->FontSize + 4.0f, elementPos.y));

            // Icon
            if (icon)
            {
                ImGui::Image((ImTextureID)icon->GetRendererID(), ImVec2{lineHeight - 4, lineHeight - 4}, {0, 1}, {1, 0});
                ImGui::SameLine();
            }
            
            // Text alignment fix (center vertically roughly)
            float textOffsetY = (lineHeight - GImGui->FontSize) / 2.0f;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffsetY);
            ImGui::TextUnformatted(name.c_str());
            
            // Restore proper cursor Y for next item (although Selectable already advanced it, we messed with it)
            // Selectable advanced Y by lineHeight. 
            // Our overlay drawing logic mostly stayed on the same line or advanced locally.
            // We should ensure we are at the line *after* the header.
            // Left empty to remove the misplaced PopID in previous block
            ImGui::SetCursorPos(ImVec2(elementPos.x, elementPos.y + lineHeight));
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
            {
                ImGui::OpenPopup("ComponentSetting");
            }
            
            ImGui::PopStyleVar(); // Pop FramePadding after header and button

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSetting"))
            {
                if (ImGui::MenuItem("Remove component"))
                    removeComponent = true;
                ImGui::EndPopup();
            }

            if (open)
            {
                uiFunction(component);
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
            
            ImGui::PopID(); // Pop Component ID at the very end
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto &tag = entity.GetComponent<TagComponent>().Tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));

            strcat(buffer, tag.c_str());
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }
        ImGui::SameLine();
        // ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<CameraComponent>("Camera");
            DisplayAddComponentEntry<ScriptComponent>("Script Component");
            DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
            DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
            DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody2D");
            DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider2D");
            DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider2D");
            DisplayAddComponentEntry<SpriteAnimationComponent>("Sprite Animation");
            DisplayAddComponentEntry<MeshComponent>("Mesh Renderer");
            DisplayAddComponentEntry<TilemapComponent>("Tilemap");

            ImGui::EndPopup();
        }

        DrawComponent<TransformComponent>("Transform", entity, m_ComponentIcons["Transform"],
                                          [](auto &component)
                                          {
                                              DrawVec3Control("Position", component.Position);
                                              glm::vec3 rotation = glm::degrees(component.Rotation);
                                              DrawVec3Control("Rotation", rotation);
                                              component.Rotation = glm::radians(rotation);
                                              DrawVec3Control("Scale", component.Scale, 1.0f);
                                          });

        DrawComponent<CameraComponent>("Camera", entity, m_ComponentIcons["Camera"],
                                       [](auto &component)
                                       {
                                           auto &camera = component.Camera;
                                           
                                           glm::vec4 backgroundColor = camera.GetBackgroundColor();
                                           if (ImGui::ColorEdit4("Background Color", glm::value_ptr(backgroundColor)))
                                           {
                                               camera.SetBackgroundColor(backgroundColor);
                                           }

                                           DrawCheckboxControl("Primary", component.Primary);

                                           const char *projectionTypeStrings[] = {"Perspective", "Orthographic"};
                                           const char *currentProjectionTypeString =
                                                   projectionTypeStrings[(int)camera.GetProjectionType()];
                                           
                                           ImGui::PushID("Projection");
                                           if(ImGui::BeginTable("##Projection", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                                           {
                                                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                                                ImGui::TableNextColumn();
                                                ImGui::Text("Projection");
                                                ImGui::TableNextColumn();
                                                ImGui::PushItemWidth(-1);
                                                if (ImGui::BeginCombo("##Projection", currentProjectionTypeString))
                                                {
                                                    for (int i = 0; i < 2; i++)
                                                    {
                                                        bool isSelected =
                                                                currentProjectionTypeString == projectionTypeStrings[i];
                                                        if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                                                        {
                                                            currentProjectionTypeString = projectionTypeStrings[i];
                                                            camera.SetProjectionType((SceneCamera::ProjectionType)i);
                                                        }
                                                        if (isSelected)
                                                            ImGui::SetItemDefaultFocus();
                                                    }
                                                    ImGui::EndCombo();
                                                }
                                                ImGui::PopItemWidth();
                                                ImGui::EndTable();
                                           }
                                           ImGui::PopID();

                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                                           {
                                               float perspectiveFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
                                               DrawFloatControl("Vertical FOV", perspectiveFOV);
                                               if (perspectiveFOV != glm::degrees(camera.GetPerspectiveVerticalFOV()))
                                                   camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveFOV));

                                               float perspectiveNear = camera.GetPerspectiveNearClip();
                                               DrawFloatControl("Near", perspectiveNear);
                                               camera.SetPerspectiveNearClip(perspectiveNear);

                                               float perspectiveFar = camera.GetPerspectiveFarClip();
                                               DrawFloatControl("Far", perspectiveFar);
                                               camera.SetPerspectiveFarClip(perspectiveFar);
                                           }
                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                                           {
                                               float orthographicSize = camera.GetOrthographicSize();
                                               DrawFloatControl("Size", orthographicSize);
                                               camera.SetOrthographicSize(orthographicSize);

                                               float orthographicNear = camera.GetOrthographicNearClip();
                                               DrawFloatControl("Near", orthographicNear);
                                               camera.SetOrthographicNearClip(orthographicNear);

                                               float orthographicFar = camera.GetOrthographicFarClip();
                                               DrawFloatControl("Far", orthographicFar);
                                               camera.SetOrthographicFarClip(orthographicFar);

                                               DrawCheckboxControl("Fixed Aspect Ratio", component.FixedAspectRatio);
                                           }
                                       });

        #include <cstdlib> // for system()

        // ...

        DrawComponent<ScriptComponent>(
                "Script", entity, m_ComponentIcons["Script"],
                [entity, scene = m_Context](auto &component) mutable
                {
                    //bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

                    ImGui::Text("Class Name");
                    ImGui::SameLine();

                    char buffer[256];
                    memset(buffer, 0, sizeof(buffer));
                    strcpy_s(buffer, sizeof(buffer), component.ClassName.c_str());

                    bool isEdited = ImGui::InputText("##ClassName", buffer, sizeof(buffer));
                    if (isEdited)
                    {
                        component.ClassName = buffer;
                    }

                    bool scriptClassExists = false;
                    if (!component.ClassName.empty())
                    {
                        scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);
                    }

                    // 4. 根据查询结果绘制 UI
                    if (!scriptClassExists && !component.ClassName.empty())
                    {
                        // 只有当不是正在输入时，才显示红色警告（可选优化）
                        if (!ImGui::IsItemActive())
                            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Invalid Script Class!");
                    }
                    else if (scriptClassExists)
                    {
                        // --- Edit Button ---
                        ImGui::SameLine();
                        if (ImGui::Button("Edit"))
                        {
                            if (auto project = Project::GetActive())
                            {
                                auto projectDir = Project::GetProjectDirectory();
                                std::filesystem::path scriptPath =
                                        projectDir / "assets" / "scripts" / (component.ClassName + ".cs");

                                if (std::filesystem::exists(scriptPath))
                                {
                                    std::string cmd =
                                            "code \"" + projectDir.string() + "\" \"" + scriptPath.string() + "\"";
                                    system(cmd.c_str());
                                }
                            }
                        }

                        if (!isEdited)
                        {
                             // Get fields from ScriptEngine (Populates ScriptComponent::Fields)
                             auto& fields = ScriptEngine::GetScriptFieldMap(entity);
                             
                             for (auto& [name, field] : fields)
                             {
                                 // Float
                                 if (field.Type == ScriptFieldType::Float)
                                 {
                                     float data = field.GetValue<float>();
                                     // Use DrawFloatControl for consistency
                                     // Note: DrawFloatControl takes float&, so we pass local var and update field on change
                                     // But DrawFloatControl already does ImGui internally.
                                     // We need to know IF it changed. DrawFloatControl doesn't return bool (void).
                                     // Let's modify DrawFloatControl or just check change manually?
                                     // Looking at DrawFloatControl: it takes float& value. It modifies it inside.
                                     // So we pass data, then check if data != field.GetValue().
                                     
                                     float oldData = data;
                                     DrawFloatControl(name, data);
                                     if (data != oldData)
                                     {
                                         field.SetValue(data);
                                         ScriptEngine::SetFloat(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                     }
                                 }
                                 // Int
                                 else if (field.Type == ScriptFieldType::Int)
                                 {
                                     int data = field.GetValue<int>();
                                     // No DrawIntControl, use generic ImGui or custom? 
                                     // Let's us ImGui::DragInt matching style of DrawFloatControl as much as possible
                                     ImGui::PushID(name.c_str());
                                     if(ImGui::BeginTable("##IntControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                                     {
                                         ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                         ImGui::TableSetupColumn("Value");
                                         ImGui::TableNextColumn();
                                         ImGui::Text("%s", name.c_str());
                                         ImGui::TableNextColumn();
                                         ImGui::PushItemWidth(-1);
                                         if (ImGui::DragInt("##Value", &data))
                                         {
                                             field.SetValue(data);
                                             ScriptEngine::SetInt(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                         }
                                         ImGui::PopItemWidth();
                                         ImGui::EndTable();
                                     }
                                     ImGui::PopID();
                                 }
                                 // Bool
                                 else if (field.Type == ScriptFieldType::Bool)
                                 {
                                     bool data = field.GetValue<bool>();
                                     bool oldData = data;
                                     DrawCheckboxControl(name, data);
                                     if (data != oldData)
                                     {
                                         field.SetValue(data);
                                         ScriptEngine::SetBool(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                     }
                                 }
                                 // Vector2
                                 else if (field.Type == ScriptFieldType::Vector2)
                                 {
                                     glm::vec2 data = field.GetValue<glm::vec2>();
                                     glm::vec2 oldData = data;
                                     // No DrawVec2Control? We can use DrawVec3Control and ignore Z or implement simplified
                                     // Or just ImGui::DragFloat2
                                     ImGui::PushID(name.c_str());
                                     if(ImGui::BeginTable("##Vec2Control", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                                     {
                                         ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                         ImGui::TableSetupColumn("Value");
                                         ImGui::TableNextColumn();
                                         ImGui::Text("%s", name.c_str());
                                         ImGui::TableNextColumn();
                                         ImGui::PushItemWidth(-1);
                                         if (ImGui::DragFloat2("##Value", glm::value_ptr(data), 0.1f))
                                         {
                                             field.SetValue(data);
                                             ScriptEngine::SetVector2(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                         }
                                         ImGui::PopItemWidth();
                                         ImGui::EndTable();
                                     }
                                     ImGui::PopID();
                                 }
                                 // Vector3
                                 else if (field.Type == ScriptFieldType::Vector3)
                                 {
                                     glm::vec3 data = field.GetValue<glm::vec3>();
                                     glm::vec3 oldData = data;
                                     DrawVec3Control(name, data);
                                     if (data != oldData)
                                     {
                                         field.SetValue(data);
                                         ScriptEngine::SetVector3(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                     }
                                 }
                                 // Vector4
                                 else if (field.Type == ScriptFieldType::Vector4)
                                 {
                                     glm::vec4 data = field.GetValue<glm::vec4>();
                                     glm::vec4 oldData = data;
                                     DrawColorControl(name, data); // Assuming Vec4 is Color mostly? Or just numbers?
                                     // If it's a color, DrawColorControl is good. If generic vector, maybe DragFloat4.
                                     // Let's assume generic vector for now since Color is specific?
                                     // But DrawColorControl is generic enough.
                                     // Let's check matching.
                                     if (data != oldData)
                                     {
                                         field.SetValue(data);
                                         ScriptEngine::SetVector4(ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                     }
                                 }
                             }
                        }
                    }
                });

        DrawComponent<SpriteRendererComponent>(
                "Sprite Renderer", entity, m_ComponentIcons["Sprite Renderer"],
                [](auto &component)
                {
                    DrawColorControl("Color", component.Color);
                    // Texture
                    ImGui::PushID("Texture");
                    if(ImGui::BeginTable("##Texture", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Texture");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::Button("Texture", ImVec2(100.0f, 0.0f));
                        ImGui::PopItemWidth();
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                            {
                                const wchar_t *path = (const wchar_t *)payload->Data;
                                std::filesystem::path texturePath = Project::GetAssetDirectory() / path;

                                if (std::filesystem::exists(texturePath))
                                    component.Texture = Texture2D::Create(texturePath.string());
                                else
                                    HIMII_CORE_WARNING("Texture not found: {0}", texturePath.string());
                            }

                            ImGui::EndDragDropTarget();
                        }
                        ImGui::EndTable();
                    }
                    ImGui::PopID();

                    DrawFloatControl("Tiling Factor", component.TilingFactor, 0.1f, 0.0f, 100.0f);
                });

        DrawComponent<MeshComponent>(
                 "Mesh Renderer", entity, m_ComponentIcons["Mesh Renderer"],
                 [](auto &component)
                 {
                     const char* meshTypeStrings[] = { "Cube", "Plane", "Sphere", "Capsule" };
                     const char* currentMeshTypeString = meshTypeStrings[(int)component.Type];
                     
                     if (ImGui::BeginCombo("Mesh Type", currentMeshTypeString))
                     {
                         for (int i = 0; i < 4; i++)
                         {
                             bool isSelected = (currentMeshTypeString == meshTypeStrings[i]);
                             if (ImGui::Selectable(meshTypeStrings[i], isSelected))
                             {
                                 currentMeshTypeString = meshTypeStrings[i];
                                 component.Type = (MeshComponent::MeshType)i;
                             }
                             if (isSelected)
                                 ImGui::SetItemDefaultFocus();
                         }
                         ImGui::EndCombo();
                     }

                     DrawColorControl("Color", component.Color);
                 });

        DrawComponent<CircleRendererComponent>("Circle Renderer", entity, m_ComponentIcons["Circle Renderer"],
                                               [](auto &component)
                                               {
                                                   DrawColorControl("Color", component.Color);
                                                   DrawFloatControl("Thickness", component.Thickness, 0.025f, 0.0f, 1.0f);
                                                   DrawFloatControl("Fade", component.Fade, 0.0003f, 0.0f, 1.0f);
                                               });

        DrawComponent<Rigidbody2DComponent>("Rigidbody2D", entity, m_ComponentIcons["Rigidbody2D"],
                                            [](auto &component)
                                            {
                                                ImGui::PushID("Body Type");
                                                if(ImGui::BeginTable("##BodyType", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                                                {
                                                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                                                    ImGui::TableNextColumn();

                                                    ImGui::Text("Body Type");
                                                    ImGui::TableNextColumn();
                                                    ImGui::PushItemWidth(-1);
                                                    const char *bodyTypeStrings[] = {"Static", "Dynamic", "Kinematic"};
                                                    const char *currentBodyTypeString = bodyTypeStrings[(int)component.Type];
                                                    if (ImGui::BeginCombo("##Body Type", currentBodyTypeString))
                                                    {
                                                        for (int i = 0; i < 3; i++)
                                                        {
                                                            bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
                                                            if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
                                                            {
                                                                currentBodyTypeString = bodyTypeStrings[i];
                                                                component.Type = (Rigidbody2DComponent::BodyType)i;
                                                            }
                                                            if (isSelected) ImGui::SetItemDefaultFocus();
                                                        }
                                                        ImGui::EndCombo();
                                                    }
                                                    ImGui::PopItemWidth();
                                                    ImGui::EndTable();
                                                }
                                                ImGui::PopID();

                                                DrawCheckboxControl("Fixed Rotation", component.FixedRotation);
                                            });

        DrawComponent<BoxCollider2DComponent>(
                "Box Collider2D", entity, m_ComponentIcons["Box Collider2D"],
                [](auto &component)
                {
                    // For vec2, we can just use DrawFloatControl twice or keep usage? 
                    // Let's implement vec2 logic or simpler:
                    ImGui::PushID("Offset");
                    if(ImGui::BeginTable("##Offset", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Offset");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    ImGui::PushID("Size");
                    if(ImGui::BeginTable("##Size", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Size");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Size", glm::value_ptr(component.Size), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    DrawFloatControl("Density", component.Density, 0.1f);
                    DrawFloatControl("Friction", component.Friction, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution", component.Restitution, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution Threshold", component.RestitutionThreshold, 0.1f);
                });
        DrawComponent<CircleCollider2DComponent>(
                "Circle Collider2D", entity, m_ComponentIcons["Circle Collider2D"],
                [](auto &component)
                {
                    ImGui::PushID("Offset");
                    if(ImGui::BeginTable("##Offset", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Offset");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    DrawFloatControl("Radius", component.Radius, 0.1f);
                    DrawFloatControl("Density", component.Density, 0.1f);
                    DrawFloatControl("Friction", component.Friction, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution", component.Restitution, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution Threshold", component.RestitutionThreshold, 0.1f);
                });
        DrawComponent<SpriteAnimationComponent>(
                "Sprite Animation", entity, m_ComponentIcons["Sprite Animation"],
                [](auto &component)
                {
                    ImGui::Text("Asset Handle: %llu", (uint64_t)component.AnimationHandle);

                    ImGui::Button("Animation Asset", ImVec2(100.0f, 0.0f));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            const wchar_t *path = (const wchar_t *)payload->Data;
                            std::filesystem::path assetPath = path;

                            if (assetPath.extension() == ".anim")
                            {
                                // 获取 AssetManager (需要 #include "Himii/Project/Project.h")
                                auto assetManager = Project::GetAssetManager();
                                if (assetManager)
                                {
                                    AssetHandle handle = assetManager->ImportAsset(assetPath);
                                    if (handle != 0)
                                        component.AnimationHandle = handle;
                                }
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::DragFloat("Frame Rate", &component.FrameRate, 0.1f, 0.0f, 60.0f);
                    ImGui::Checkbox("Playing", &component.Playing);
                    ImGui::Text("Current Frame: %d", component.CurrentFrame);
                    ImGui::Text("Timer: %.2f", component.Timer);
                });

        DrawComponent<TilemapComponent>(
                "Tilemap", entity, nullptr,
                [](auto &component)
                {
                    auto assetManager = Project::GetAssetManager();

                    // ---- TileMap Asset 引用 ----
                    ImGui::PushID("TileMapHandle");
                    {
                        std::string label = "None (drag .tilemap)";
                        if (component.TileMapHandle != 0 && assetManager && assetManager->IsAssetHandleValid(component.TileMapHandle))
                        {
                            label = "TileMap: " + std::to_string((uint64_t)component.TileMapHandle);
                        }

                        ImGui::Button(label.c_str(), ImVec2(-1, 0.0f));

                        // 拖拽 .tilemap 文件
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                            {
                                const wchar_t *path = (const wchar_t *)payload->Data;
                                std::filesystem::path assetPath(path);
                                if (assetPath.extension() == ".tilemap" && assetManager)
                                {
                                    AssetHandle handle = assetManager->ImportAsset(assetPath);
                                    if (handle != 0)
                                        component.TileMapHandle = handle;
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }
                    }
                    ImGui::PopID();

                    // ---- "新建 TileMap" 按钮 ----
                    if (component.TileMapHandle == 0)
                    {
                        ImGui::Spacing();
                        if (ImGui::Button("Create New TileMap", ImVec2(-1, 0)))
                        {
                            if (assetManager)
                            {
                                auto assetDir = Project::GetAssetDirectory();

                                // 创建默认 TileSet
                                auto tileSetAsset = std::make_shared<TileSet>();
                                tileSetAsset->Handle = AssetHandle();

                                TileDef whiteTile;
                                whiteTile.ID = 1;
                                whiteTile.SourceType = TileSourceType::Atlas;
                                whiteTile.Tint = {1.0f, 1.0f, 1.0f, 1.0f};
                                tileSetAsset->AddTileDef(whiteTile);

                                std::filesystem::path tsDir = assetDir / "tilesets";
                                std::filesystem::create_directories(tsDir);
                                std::filesystem::path tsPath = tsDir / "default.tileset";
                                // 避免覆盖：如果文件已存在则加序号
                                int idx = 0;
                                while (std::filesystem::exists(tsPath))
                                    tsPath = tsDir / ("default_" + std::to_string(++idx) + ".tileset");

                                TileSetSerializer::Serialize(tsPath, tileSetAsset);
                                auto tsRelPath = std::filesystem::relative(tsPath, assetDir);
                                AssetHandle tsHandle = assetManager->ImportAsset(tsRelPath);
                                tileSetAsset->Handle = tsHandle;
                                TileSetSerializer::Serialize(tsPath, tileSetAsset);

                                // 创建默认 TileMapData (16x16)
                                auto mapAsset = std::make_shared<TileMapData>();
                                mapAsset->Handle = AssetHandle();
                                mapAsset->SetTileSetHandle(tsHandle);
                                mapAsset->Resize(16, 16);
                                mapAsset->SetCellSize(1.0f);

                                std::filesystem::path tmDir = assetDir / "tilemaps";
                                std::filesystem::create_directories(tmDir);
                                std::filesystem::path tmPath = tmDir / "new_tilemap.tilemap";
                                idx = 0;
                                while (std::filesystem::exists(tmPath))
                                    tmPath = tmDir / ("new_tilemap_" + std::to_string(++idx) + ".tilemap");

                                TileMapDataSerializer::Serialize(tmPath, mapAsset);
                                auto tmRelPath = std::filesystem::relative(tmPath, assetDir);
                                AssetHandle tmHandle = assetManager->ImportAsset(tmRelPath);
                                mapAsset->Handle = tmHandle;
                                TileMapDataSerializer::Serialize(tmPath, mapAsset);

                                assetManager->SerializeAssetRegistry();

                                component.TileMapHandle = tmHandle;
                            }
                        }
                    }

                    // ---- 如果已绑定 TileMap，显示编辑 UI ----
                    if (component.TileMapHandle != 0 && assetManager && assetManager->IsAssetHandleValid(component.TileMapHandle))
                    {
                        auto asset = assetManager->GetAsset(component.TileMapHandle);
                        if (asset)
                        {
                            auto mapData = std::static_pointer_cast<TileMapData>(asset);

                            ImGui::Separator();
                            ImGui::Text("TileMap Properties");

                            // Width / Height
                            int width = (int)mapData->GetWidth();
                            int height = (int)mapData->GetHeight();
                            float cellSize = mapData->GetCellSize();

                            bool changed = false;
                            if (ImGui::DragInt("Width", &width, 1.0f, 1, 1024))
                            {
                                mapData->Resize((uint32_t)width, mapData->GetHeight());
                                changed = true;
                            }
                            if (ImGui::DragInt("Height", &height, 1.0f, 1, 1024))
                            {
                                mapData->Resize(mapData->GetWidth(), (uint32_t)height);
                                changed = true;
                            }
                            if (ImGui::DragFloat("Cell Size", &cellSize, 0.05f, 0.1f, 10.0f))
                            {
                                mapData->SetCellSize(cellSize);
                                changed = true;
                            }

                            // TileSet 引用
                            ImGui::Separator();
                            AssetHandle tsHandle = mapData->GetTileSetHandle();
                            std::string tsLabel = (tsHandle != 0)
                                ? "TileSet: " + std::to_string((uint64_t)tsHandle)
                                : "No TileSet";
                            ImGui::Text("%s", tsLabel.c_str());

                            // 简易 Tile 编辑网格
                            ImGui::Separator();
                            ImGui::Text("Tile Grid (click to toggle)");

                            uint32_t w = mapData->GetWidth();
                            uint32_t h = mapData->GetHeight();

                            // 限制显示大小，避免太大的地图卡 UI
                            uint32_t displayW = (w > 32) ? 32 : w;
                            uint32_t displayH = (h > 32) ? 32 : h;
                            if (w > 32 || h > 32)
                                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Showing %dx%d of %dx%d", displayW, displayH, w, h);

                            float btnSize = 18.0f;
                            for (uint32_t y = 0; y < displayH; ++y)
                            {
                                for (uint32_t x = 0; x < displayW; ++x)
                                {
                                    if (x > 0) ImGui::SameLine(0, 1);

                                    ImGui::PushID((int)(x + y * w));
                                    uint16_t tileID = mapData->GetTile(x, y);

                                    // 有 Tile 显示白色，空显示灰色
                                    if (tileID > 0)
                                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                                    else
                                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                                    if (ImGui::Button("##tile", ImVec2(btnSize, btnSize)))
                                    {
                                        // 点击切换: 0 ↔ 1
                                        mapData->SetTile(x, y, tileID > 0 ? 0 : 1);
                                        changed = true;
                                    }
                                    ImGui::PopStyleColor();
                                    ImGui::PopID();
                                }
                            }

                            // 保存按钮
                            ImGui::Spacing();
                            if (ImGui::Button("Save TileMap", ImVec2(-1, 0)) || changed)
                            {
                                // 重新序列化到文件
                                auto &registry = assetManager->GetAssetRegistry();
                                auto it = registry.find(component.TileMapHandle);
                                if (it != registry.end())
                                {
                                    auto fullPath = Project::GetAssetFileSystemPath(it->second.FilePath);
                                    TileMapDataSerializer::Serialize(fullPath, mapData);
                                }
                            }

                            // 解绑按钮
                            if (ImGui::Button("Detach TileMap", ImVec2(-1, 0)))
                            {
                                component.TileMapHandle = 0;
                            }
                        }
                    }
                });
    }

    template<typename T>
    void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string &entryName)
    {
        if (!m_SelectionContext.HasComponent<T>())
        {
            if (ImGui::MenuItem(entryName.c_str()))
            {
                m_SelectionContext.AddComponent<T>();
                ImGui::CloseCurrentPopup();
            }
        }
    }

} // namespace Himii

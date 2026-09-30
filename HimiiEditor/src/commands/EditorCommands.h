#pragma once

#include "IEditorCommand.h"
#include "EntitySnapshot.h"
#include "Engine.h"
#include <functional>

namespace Himii
{
    class CreateEntityCommand : public IEditorCommand
    {
    public:
        using CreateEntityFunction = std::function<Entity(Ref<Scene>)>;

        CreateEntityCommand(const Ref<Scene>& scene, CreateEntityFunction createEntityFunction);

        void Execute() override;
        void Undo() override;

    private:
        Ref<Scene> m_Scene;
        CreateEntityFunction m_CreateEntityFunction;
        UUID m_EntityIdentifier = 0;
        std::string m_EntitySnapshotYaml;
    };

    class DeleteEntityCommand : public IEditorCommand
    {
    public:
        DeleteEntityCommand(const Ref<Scene>& scene, Entity entity,
                            std::function<void(Entity)> onEntityRestored = nullptr);

        void Execute() override;
        void Undo() override;

    private:
        Ref<Scene> m_Scene;
        UUID m_EntityIdentifier = 0;
        std::string m_EntitySnapshotYaml;
        std::function<void(Entity)> m_OnEntityRestored;
    };

    class ModifyTransformCommand : public IEditorCommand
    {
    public:
        ModifyTransformCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                               const TransformComponent& beforeTransform,
                               const TransformComponent& afterTransform);

        void Execute() override;
        void Undo() override;
        bool TryMerge(const IEditorCommand& other) override;

    private:
        void ApplyTransform(const TransformComponent& transform);

        Ref<Scene> m_Scene;
        UUID m_EntityIdentifier;
        TransformComponent m_BeforeTransform;
        TransformComponent m_AfterTransform;
    };

    class ModifyRectTransformCommand : public IEditorCommand
    {
    public:
        ModifyRectTransformCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                                   const RectTransformComponent& beforeTransform,
                                   const RectTransformComponent& afterTransform);

        void Execute() override;
        void Undo() override;
        bool TryMerge(const IEditorCommand& other) override;

    private:
        void ApplyTransform(const RectTransformComponent& transform);

        Ref<Scene> m_Scene;
        UUID m_EntityIdentifier;
        RectTransformComponent m_BeforeTransform;
        RectTransformComponent m_AfterTransform;
    };

    class ModifyUITextFontSizeCommand : public IEditorCommand
    {
    public:
        ModifyUITextFontSizeCommand(const Ref<Scene>& scene, UUID entityIdentifier, float beforeFontSize,
                                    float afterFontSize);

        void Execute() override;
        void Undo() override;
        bool TryMerge(const IEditorCommand& other) override;

    private:
        void ApplyFontSize(float fontSize);

        Ref<Scene> m_Scene;
        UUID m_EntityIdentifier;
        float m_BeforeFontSize = 48.0f;
        float m_AfterFontSize = 48.0f;
    };

    class ModifyEntityTagCommand : public IEditorCommand
    {
    public:
        ModifyEntityTagCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                               std::string beforeTag, std::string afterTag);

        void Execute() override;
        void Undo() override;

    private:
        void ApplyTag(const std::string& tag);

        Ref<Scene> m_Scene;
        UUID m_EntityIdentifier;
        std::string m_BeforeTag;
        std::string m_AfterTag;
    };

    class InstantiatePrefabCommand : public IEditorCommand
    {
    public:
        InstantiatePrefabCommand(const Ref<Scene>& scene, UUID rootIdentifier);

        void Execute() override;
        void Undo() override;

        UUID GetRootIdentifier() const { return m_RootIdentifier; }

    private:
        Ref<Scene> m_Scene;
        UUID m_RootIdentifier = 0;
        std::string m_SubtreeYaml;
        bool m_SkipNextExecute = true;
    };

    class ReparentEntityCommand : public IEditorCommand
    {
    public:
        ReparentEntityCommand(const Ref<Scene>& scene, Entity child, Entity newParent, bool keepWorldPosition = true);

        void Execute() override;
        void Undo() override;

    private:
        void ApplyParent(UUID parentIdentifier, const TransformComponent& localTransform,
                         const RectTransformComponent& rectTransform, bool isUserInterface);

        Ref<Scene> m_Scene;
        UUID m_ChildIdentifier = 0;
        UUID m_ParentBeforeIdentifier = 0;
        UUID m_ParentAfterIdentifier = 0;
        bool m_IsUserInterface = false;
        bool m_KeepWorldPosition = true;
        TransformComponent m_LocalTransformBefore{};
        TransformComponent m_LocalTransformAfter{};
        RectTransformComponent m_RectTransformBefore{};
        RectTransformComponent m_RectTransformAfter{};
    };
}

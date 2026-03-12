#include "ParticleEmitterEditorPanel.h"

#include "imgui.h"
#include "Himii/Asset/AssetManager.h"
#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Project/Project.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Core/Log.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    ParticleEmitterEditorPanel::ParticleEmitterEditorPanel()
        : m_Camera(-5.0f, 5.0f, -5.0f, 5.0f)
    {
        FramebufferSpecification fbSpec{ 320, 240 };
        fbSpec.Attachments = { FramebufferFormat::RGBA8, FramebufferFormat::Depth };
        m_Framebuffer = Framebuffer::Create(fbSpec);
    }

    void ParticleEmitterEditorPanel::Open(AssetHandle emitterHandle)
    {
        m_EmitterHandle = emitterHandle;
        m_Asset.reset();
        LoadAsset();
        m_PreviewAccumulator = 0.0f;
    }

    void ParticleEmitterEditorPanel::LoadAsset()
    {
        if (!Project::GetActive() || m_EmitterHandle == 0)
            return;
        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;
        Ref<Asset> ref = assetManager->GetAsset(m_EmitterHandle);
        if (ref)
            m_Asset = std::static_pointer_cast<ParticleEmitterAsset>(ref);
    }

    void ParticleEmitterEditorPanel::UpdatePreview(float deltaTime)
    {
        if (!m_Asset || !m_PreviewPlaying)
        {
            m_PreviewParticleSystem.OnUpdate(deltaTime);
            return;
        }
        m_PreviewAccumulator += deltaTime * m_Asset->EmissionRate;
        int emitCount = static_cast<int>(std::floor(m_PreviewAccumulator));
        if (emitCount > 0)
        {
            m_PreviewAccumulator -= static_cast<float>(emitCount);
            ParticleProps props = m_Asset->TemplateProps;
            props.position = glm::vec3(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < emitCount; ++i)
                m_PreviewParticleSystem.Emit(props);
        }
        m_PreviewParticleSystem.OnUpdate(deltaTime);
    }

    void ParticleEmitterEditorPanel::RenderPreview()
    {
        ImVec2 region = ImGui::GetContentRegionAvail();
        if (region.x < 1.0f || region.y < 1.0f)
            return;

        uint32_t w = static_cast<uint32_t>(region.x);
        uint32_t h = static_cast<uint32_t>(region.y);
        if (m_Framebuffer->GetSpecification().Width != w || m_Framebuffer->GetSpecification().Height != h)
            m_Framebuffer->Resize(w, h);

        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({ 0.12f, 0.12f, 0.15f, 1.0f });
        RenderCommand::Clear();

        Renderer2D::BeginScene(m_Camera);
        m_PreviewParticleSystem.ForEachAlive([&](const ParticleSystem::ParticleView& p)
        {
            float t = 1.0f - p.remainingLife / p.lifetime;
            glm::vec4 color = glm::mix(p.colorBegin, p.colorEnd, t);
            float size = glm::mix(p.sizeBegin, p.sizeEnd, t);
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), p.position)
                * glm::rotate(glm::mat4(1.0f), p.rotation, glm::vec3(0, 0, 1))
                * glm::scale(glm::mat4(1.0f), glm::vec3(size));
            Renderer2D::DrawQuad(transform, color);
        });
        Renderer2D::EndScene();

        m_Framebuffer->Unbind();

        uint64_t texId = m_Framebuffer->GetColorAttachmentRendererID(0);
        ImGui::Image(reinterpret_cast<void*>(texId), region, { 0, 1 }, { 1, 0 });
    }

    void ParticleEmitterEditorPanel::UI_Properties()
    {
        if (!m_Asset)
            return;

        ParticleProps& p = m_Asset->TemplateProps;

        ImGui::Text("Template (ParticleProps)");
        ImGui::Separator();
        ImGui::DragFloat3("Position", &p.position.x, 0.05f);
        ImGui::DragFloat3("Velocity", &p.velocity.x, 0.05f);
        ImGui::DragFloat3("Velocity Variation", &p.velocityVariation.x, 0.01f);
        ImGui::DragFloat("Lifetime", &p.lifetime, 0.05f, 0.01f, 10.0f);
        ImGui::ColorEdit4("Color Begin", &p.colorBegin.x);
        ImGui::ColorEdit4("Color End", &p.colorEnd.x);
        ImGui::DragFloat("Size Begin", &p.sizeBegin, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Size End", &p.sizeEnd, 0.01f, 0.0f, 5.0f);

        ImGui::Spacing();
        ImGui::Text("Emitter");
        ImGui::Separator();
        ImGui::DragFloat("Emission Rate", &m_Asset->EmissionRate, 1.0f, 0.0f, 500.0f);
        ImGui::Checkbox("Looping", &m_Asset->Looping);
    }

    void ParticleEmitterEditorPanel::SaveAsset()
    {
        if (!m_Asset || m_EmitterHandle == 0 || !Project::GetActive())
            return;
        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;
        const auto& registry = assetManager->GetAssetRegistry();
        auto it = registry.find(m_EmitterHandle);
        if (it != registry.end())
        {
            std::filesystem::path fullPath = Project::GetAssetFileSystemPath(it->second.FilePath);
            ParticleEmitterAssetSerializer::Serialize(fullPath, m_Asset);
            HIMII_CORE_INFO("ParticleEmitter saved to: {0}", fullPath.string());
        }
    }

    void ParticleEmitterEditorPanel::OnImGuiRender(bool& isOpen)
    {
        if (!isOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Particle Emitter Editor", &isOpen))
        {
            ImGui::End();
            return;
        }

        if (m_EmitterHandle == 0 || !m_Asset)
        {
            ImGui::TextDisabled("No emitter loaded.");
            ImGui::TextDisabled("Select an entity with ParticleEmitterComponent and click 'Open in Particle Emitter Editor'.");
            ImGui::End();
            return;
        }

        UpdatePreview(ImGui::GetIO().DeltaTime);

        if (ImGui::Button("Save"))
            SaveAsset();
        ImGui::SameLine();
        if (ImGui::Button(m_PreviewPlaying ? "Pause" : "Play"))
            m_PreviewPlaying = !m_PreviewPlaying;
        ImGui::SameLine();
        ImGui::Text("Handle: %llu", (uint64_t)m_EmitterHandle);
        ImGui::Separator();

        float totalWidth = ImGui::GetContentRegionAvail().x;
        float propsWidth = 220.0f;
        float previewWidth = totalWidth - propsWidth - 8.0f;
        if (previewWidth < 160.0f)
            previewWidth = 160.0f;

        ImGui::BeginChild("Preview", ImVec2(previewWidth, -1), true);
        RenderPreview();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Properties", ImVec2(propsWidth, -1), true);
        UI_Properties();
        ImGui::EndChild();

        ImGui::End();
    }
}

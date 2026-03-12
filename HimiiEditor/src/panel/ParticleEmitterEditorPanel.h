#pragma once

#include "Himii/Core/Core.h"
#include "Himii/Asset/Asset.h"
#include "Himii/Renderer/OrthographicCamera.h"
#include "Himii/Renderer/Framebuffer.h"
#include "Himii/Scene/ParticleEmitterAsset.h"
#include "Himii/Particle/ParticleSystem.h"

#include <glm/glm.hpp>

namespace Himii
{
    class ParticleEmitterEditorPanel
    {
    public:
        ParticleEmitterEditorPanel();

        void OnImGuiRender(bool& isOpen);
        void Open(AssetHandle emitterHandle);

    private:
        void LoadAsset();
        void UpdatePreview(float deltaTime);
        void RenderPreview();
        void UI_Properties();
        void SaveAsset();

        Ref<Framebuffer> m_Framebuffer;
        OrthographicCamera m_Camera;
        glm::vec2 m_PreviewSize{ 320.0f, 240.0f };

        AssetHandle m_EmitterHandle = 0;
        Ref<ParticleEmitterAsset> m_Asset;

        ParticleSystem m_PreviewParticleSystem{ 5000 };
        bool m_PreviewPlaying = true;
        float m_PreviewAccumulator = 0.0f;
    };
}

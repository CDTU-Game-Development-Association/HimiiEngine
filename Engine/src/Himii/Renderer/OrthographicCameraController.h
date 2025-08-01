#pragma once
#include "Himii/Renderer/OrthographicCamera.h"
#include "Himii/Core/Timestep.h"
#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Events/MouseEvent.h"

namespace Himii
{
    class OrthographicCameraController 
    {
    public:
        OrthographicCameraController(float aspectRatio,bool rotation=false);

        void OnUpdate(Timestep ts);
        void OnEvent(Event &e);

        void OnResize(float width, float height);

        OrthographicCamera &GetCamera()
        {
            return m_Camera;
        }
        const OrthographicCamera &GetCamera() const
        {
            return m_Camera;
        }

        float GetZoomLevel() const
        {
            return m_ZoomLevel;
        }
        void SetZoomLevel(float level)
        {
            m_ZoomLevel = level;
        }

    private:
        bool OnMouseScrolled(MouseScrolledEvent &e);
        bool OnWindowResize(WindowResizeEvent &e);

    private:
        float m_AspectRatio;
        float m_ZoomLevel=1.0f;
        OrthographicCamera m_Camera;

        bool m_Rotation;
        glm::vec3 m_CameraPosition = {0.0f, 0.0f, 0.0f};
        float m_CameraRotation = 0.0f;
        float m_CameraTranslationSpeed = 1.0f, m_CameraRotationSpeed = 1.0f;
    };
} // namespace Himii

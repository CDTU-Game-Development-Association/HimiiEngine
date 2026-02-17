#pragma once

#include "Himii/Core/Core.h"
#include "Himii/Asset/Asset.h"
#include "Himii/Renderer/Camera.h"
#include "Himii/Renderer/Framebuffer.h"
#include "Himii/Scene/TileSet.h"
#include "Himii/Scene/TileMapData.h"

#include <glm/glm.hpp>

namespace Himii
{

    class TileMapEditorPanel {
    public:
        TileMapEditorPanel();

        void OnImGuiRender(bool &isOpen);
        void Open(AssetHandle tileMapHandle);

    private:
        enum class Tool { Brush, Eraser, Fill };

        // Canvas rendering
        Ref<Framebuffer> m_Framebuffer;
        Camera m_Camera;
        glm::vec2 m_CameraPosition{0.0f, 0.0f};
        float m_ZoomLevel = 10.0f;
        glm::vec2 m_CanvasSize{1.0f, 1.0f};

        // Asset references
        AssetHandle m_TileMapHandle = 0;
        Ref<TileMapData> m_MapData;
        Ref<TileSet> m_TileSet;

        // Tool state
        Tool m_CurrentTool = Tool::Brush;
        uint16_t m_SelectedTileID = 1;

        // Canvas interaction
        bool m_IsCanvasHovered = false;
        bool m_IsCanvasFocused = false;
        bool m_IsPanning = false;
        glm::vec2 m_LastMousePos{0.0f, 0.0f};
        glm::ivec2 m_HoveredTile{-1, -1};
        glm::vec2 m_CanvasOrigin{0.0f, 0.0f};

        // Internal methods
        void LoadAssets();
        void UpdateCamera();
        void RenderCanvas();
        void DrawGrid();
        void DrawTileHighlight();
        void HandleCanvasInput();
        glm::vec2 ScreenToWorld(glm::vec2 screenPos);
        glm::ivec2 WorldToTile(glm::vec2 worldPos);

        void UI_Toolbar();
        void UI_TilePalette();
        void UI_Properties();

        void ApplyTool(int tileX, int tileY);
        void FloodFill(int x, int y, uint16_t target, uint16_t replacement);

        void SaveTileMap();
    };

} // namespace Himii

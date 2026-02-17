#include "TileMapEditorPanel.h"

#include "imgui.h"
#include "Himii/Asset/AssetManager.h"
#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Project/Project.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <queue>

namespace Himii
{

    TileMapEditorPanel::TileMapEditorPanel()
    {
        FramebufferSpecification fbSpec{800, 600};
        fbSpec.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::Depth};
        m_Framebuffer = Framebuffer::Create(fbSpec);
    }

    void TileMapEditorPanel::Open(AssetHandle tileMapHandle)
    {
        m_TileMapHandle = tileMapHandle;
        m_MapData = nullptr;
        m_TileSet = nullptr;
        LoadAssets();

        if (m_MapData)
        {
            // Map is centered at origin, so camera starts at (0, 0)
            m_CameraPosition = {0.0f, 0.0f};
            float mapW = (float)m_MapData->GetWidth() * m_MapData->GetCellSize();
            float mapH = (float)m_MapData->GetHeight() * m_MapData->GetCellSize();
            m_ZoomLevel = std::max(mapW, mapH) * 0.7f;
            if (m_ZoomLevel < 2.0f) m_ZoomLevel = 2.0f;
        }
    }

    void TileMapEditorPanel::LoadAssets()
    {
        if (!Project::GetActive() || m_TileMapHandle == 0)
            return;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;

        auto mapAsset = assetManager->GetAsset(m_TileMapHandle);
        if (mapAsset)
            m_MapData = std::static_pointer_cast<TileMapData>(mapAsset);

        if (m_MapData && m_MapData->GetTileSetHandle() != 0)
        {
            auto tsAsset = assetManager->GetAsset(m_MapData->GetTileSetHandle());
            if (tsAsset)
                m_TileSet = std::static_pointer_cast<TileSet>(tsAsset);
        }
    }

    void TileMapEditorPanel::OnImGuiRender(bool &isOpen)
    {
        if (!isOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("TileMap Editor", &isOpen))
        {
            ImGui::End();
            return;
        }

        if (m_TileMapHandle == 0 || !m_MapData)
        {
            ImGui::TextDisabled("No TileMap loaded.");
            ImGui::TextDisabled("Select an entity with TilemapComponent and click 'Open in TileMap Editor'.");
            ImGui::End();
            return;
        }

        // Top bar: Save button + info
        if (ImGui::Button("Save"))
            SaveTileMap();
        ImGui::SameLine();
        ImGui::Text("TileMap: %llu  |  %dx%d  |  CellSize: %.2f",
                     (uint64_t)m_TileMapHandle,
                     m_MapData->GetWidth(), m_MapData->GetHeight(),
                     m_MapData->GetCellSize());
        ImGui::Separator();

        // Main area: 3-column layout using ImGui child windows
        float totalWidth = ImGui::GetContentRegionAvail().x;
        float paletteWidth = 160.0f;
        float propertiesWidth = 160.0f;
        float canvasWidth = totalWidth - paletteWidth - propertiesWidth - 16.0f;
        if (canvasWidth < 200.0f) canvasWidth = 200.0f;

        float contentHeight = ImGui::GetContentRegionAvail().y;

        // Left: Tile Palette
        ImGui::BeginChild("TilePalette", ImVec2(paletteWidth, contentHeight), true);
        UI_TilePalette();
        ImGui::EndChild();

        ImGui::SameLine();

        // Center: Toolbar + Canvas
        ImGui::BeginChild("CanvasArea", ImVec2(canvasWidth, contentHeight), false);
        {
            UI_Toolbar();
            ImGui::Separator();

            // Canvas area
            ImVec2 canvasRegion = ImGui::GetContentRegionAvail();
            if (canvasRegion.x > 1.0f && canvasRegion.y > 1.0f)
            {
                m_CanvasSize = {canvasRegion.x, canvasRegion.y};

                // Resize framebuffer if needed
                FramebufferSpecification spec = m_Framebuffer->GetSpecification();
                if (spec.Width != (uint32_t)m_CanvasSize.x || spec.Height != (uint32_t)m_CanvasSize.y)
                {
                    m_Framebuffer->Resize((uint32_t)m_CanvasSize.x, (uint32_t)m_CanvasSize.y);
                }

                RenderCanvas();

                // Display the framebuffer texture via DrawList + InvisibleButton overlay
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                m_CanvasOrigin = {cursorPos.x, cursorPos.y};

                uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                drawList->AddImage(
                    reinterpret_cast<void *>(textureID),
                    cursorPos,
                    ImVec2(cursorPos.x + m_CanvasSize.x, cursorPos.y + m_CanvasSize.y),
                    ImVec2(0, 1), ImVec2(1, 0));

                // InvisibleButton captures all mouse input (click, drag, scroll)
                ImGui::InvisibleButton("##tilemap_canvas", ImVec2(m_CanvasSize.x, m_CanvasSize.y),
                                        ImGuiButtonFlags_MouseButtonLeft |
                                        ImGuiButtonFlags_MouseButtonRight |
                                        ImGuiButtonFlags_MouseButtonMiddle);

                m_IsCanvasHovered = ImGui::IsItemHovered();
                m_IsCanvasFocused = ImGui::IsItemActive() || m_IsCanvasHovered;

                HandleCanvasInput();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Right: Properties
        ImGui::BeginChild("Properties", ImVec2(propertiesWidth, contentHeight), true);
        UI_Properties();
        ImGui::EndChild();

        ImGui::End();
    }

    // ======================= Canvas Rendering =======================

    void TileMapEditorPanel::UpdateCamera()
    {
        float aspect = m_CanvasSize.x / m_CanvasSize.y;
        float halfZoom = m_ZoomLevel * 0.5f;
        m_Camera = Camera(glm::ortho(
            -halfZoom * aspect, halfZoom * aspect,
            -halfZoom, halfZoom,
            -1.0f, 1.0f));
    }

    void TileMapEditorPanel::RenderCanvas()
    {
        UpdateCamera();

        m_Framebuffer->Bind();

        RenderCommand::SetClearColor({0.15f, 0.15f, 0.15f, 1.0f});
        RenderCommand::Clear();

        glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f),
                                                     glm::vec3(m_CameraPosition, 0.0f));
        Renderer2D::BeginScene(m_Camera, cameraTransform);

        // Draw all tiles (centered: origin at map center)
        if (m_MapData)
        {
            uint32_t width = m_MapData->GetWidth();
            uint32_t height = m_MapData->GetHeight();
            float cellSize = m_MapData->GetCellSize();
            const auto &tiles = m_MapData->GetTiles();

            float offsetX = -(float)width * cellSize * 0.5f;
            float offsetY = -(float)height * cellSize * 0.5f;

            for (uint32_t y = 0; y < height; ++y)
            {
                for (uint32_t x = 0; x < width; ++x)
                {
                    uint16_t tileID = tiles[x + y * width];

                    float px = offsetX + (float)x * cellSize;
                    float py = offsetY + (float)y * cellSize;
                    glm::vec3 pos = {px + cellSize * 0.5f, py + cellSize * 0.5f, 0.0f};
                    glm::vec2 size = {cellSize * 0.98f, cellSize * 0.98f};

                    if (tileID == 0)
                    {
                        Renderer2D::DrawQuad(pos, size, {0.2f, 0.2f, 0.2f, 0.5f});
                    }
                    else
                    {
                        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
                        if (m_TileSet)
                        {
                            const TileDef *def = m_TileSet->GetTileDef(tileID);
                            if (def)
                                color = def->Tint;
                        }
                        Renderer2D::DrawQuad(pos, size, color);
                    }
                }
            }
        }

        DrawGrid();
        DrawTileHighlight();

        Renderer2D::EndScene();

        m_Framebuffer->Unbind();
    }

    void TileMapEditorPanel::DrawGrid()
    {
        if (!m_MapData)
            return;

        uint32_t width = m_MapData->GetWidth();
        uint32_t height = m_MapData->GetHeight();
        float cellSize = m_MapData->GetCellSize();
        float mapW = (float)width * cellSize;
        float mapH = (float)height * cellSize;

        float left = -mapW * 0.5f;
        float bottom = -mapH * 0.5f;
        float right = mapW * 0.5f;
        float top = mapH * 0.5f;

        glm::vec4 gridColor = {0.4f, 0.4f, 0.4f, 0.6f};

        // Vertical lines
        for (uint32_t x = 0; x <= width; ++x)
        {
            float px = left + (float)x * cellSize;
            Renderer2D::DrawLine({px, bottom, 0.01f}, {px, top, 0.01f}, gridColor);
        }
        // Horizontal lines
        for (uint32_t y = 0; y <= height; ++y)
        {
            float py = bottom + (float)y * cellSize;
            Renderer2D::DrawLine({left, py, 0.01f}, {right, py, 0.01f}, gridColor);
        }

        // Border
        glm::vec4 borderColor = {0.8f, 0.8f, 0.2f, 0.8f};
        Renderer2D::DrawLine({left, bottom, 0.02f}, {right, bottom, 0.02f}, borderColor);
        Renderer2D::DrawLine({right, bottom, 0.02f}, {right, top, 0.02f}, borderColor);
        Renderer2D::DrawLine({right, top, 0.02f}, {left, top, 0.02f}, borderColor);
        Renderer2D::DrawLine({left, top, 0.02f}, {left, bottom, 0.02f}, borderColor);

        // Origin crosshair
        glm::vec4 originColor = {0.5f, 0.5f, 0.5f, 0.3f};
        float ext = std::max(mapW, mapH) * 0.1f;
        Renderer2D::DrawLine({-ext, 0.0f, 0.005f}, {ext, 0.0f, 0.005f}, originColor);
        Renderer2D::DrawLine({0.0f, -ext, 0.005f}, {0.0f, ext, 0.005f}, originColor);
    }

    void TileMapEditorPanel::DrawTileHighlight()
    {
        if (!m_MapData || m_HoveredTile.x < 0 || m_HoveredTile.y < 0)
            return;

        if (m_HoveredTile.x >= (int)m_MapData->GetWidth() || m_HoveredTile.y >= (int)m_MapData->GetHeight())
            return;

        float cellSize = m_MapData->GetCellSize();
        float offsetX = -(float)m_MapData->GetWidth() * cellSize * 0.5f;
        float offsetY = -(float)m_MapData->GetHeight() * cellSize * 0.5f;
        float px = offsetX + (float)m_HoveredTile.x * cellSize;
        float py = offsetY + (float)m_HoveredTile.y * cellSize;
        glm::vec3 pos = {px + cellSize * 0.5f, py + cellSize * 0.5f, 0.05f};
        glm::vec2 size = {cellSize, cellSize};

        glm::vec4 highlightColor;
        if (m_CurrentTool == Tool::Eraser)
            highlightColor = {1.0f, 0.3f, 0.3f, 0.4f};
        else
            highlightColor = {0.3f, 0.8f, 1.0f, 0.4f};

        Renderer2D::DrawQuad(pos, size, highlightColor);
    }

    // ======================= Canvas Interaction =======================

    glm::vec2 TileMapEditorPanel::ScreenToWorld(glm::vec2 screenPos)
    {
        float aspect = m_CanvasSize.x / m_CanvasSize.y;
        float halfZoom = m_ZoomLevel * 0.5f;

        float ndcX = (screenPos.x / m_CanvasSize.x) * 2.0f - 1.0f;
        float ndcY = 1.0f - (screenPos.y / m_CanvasSize.y) * 2.0f;

        float worldX = ndcX * halfZoom * aspect + m_CameraPosition.x;
        float worldY = ndcY * halfZoom + m_CameraPosition.y;

        return {worldX, worldY};
    }

    glm::ivec2 TileMapEditorPanel::WorldToTile(glm::vec2 worldPos)
    {
        if (!m_MapData)
            return {-1, -1};

        float cellSize = m_MapData->GetCellSize();
        float offsetX = -(float)m_MapData->GetWidth() * cellSize * 0.5f;
        float offsetY = -(float)m_MapData->GetHeight() * cellSize * 0.5f;

        int tx = (int)std::floor((worldPos.x - offsetX) / cellSize);
        int ty = (int)std::floor((worldPos.y - offsetY) / cellSize);
        return {tx, ty};
    }

    void TileMapEditorPanel::HandleCanvasInput()
    {
        if (!m_IsCanvasHovered && !m_IsPanning)
            return;

        ImGuiIO &io = ImGui::GetIO();

        // Mouse position relative to canvas
        glm::vec2 mouseScreen = {
            io.MousePos.x - m_CanvasOrigin.x,
            io.MousePos.y - m_CanvasOrigin.y
        };

        // Update hovered tile
        if (m_IsCanvasHovered)
        {
            glm::vec2 worldPos = ScreenToWorld(mouseScreen);
            m_HoveredTile = WorldToTile(worldPos);
        }
        else
        {
            m_HoveredTile = {-1, -1};
        }

        // Zoom with scroll wheel
        if (m_IsCanvasHovered && std::abs(io.MouseWheel) > 0.0f)
        {
            m_ZoomLevel -= io.MouseWheel * m_ZoomLevel * 0.1f;
            m_ZoomLevel = std::clamp(m_ZoomLevel, 1.0f, 100.0f);
        }

        // Pan with middle mouse button
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && m_IsCanvasHovered)
        {
            m_IsPanning = true;
            m_LastMousePos = {io.MousePos.x, io.MousePos.y};
        }
        if (m_IsPanning)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                glm::vec2 currentMouse = {io.MousePos.x, io.MousePos.y};
                glm::vec2 delta = currentMouse - m_LastMousePos;

                float aspect = m_CanvasSize.x / m_CanvasSize.y;
                float scaleX = m_ZoomLevel * aspect / m_CanvasSize.x;
                float scaleY = m_ZoomLevel / m_CanvasSize.y;

                m_CameraPosition.x -= delta.x * scaleX;
                m_CameraPosition.y += delta.y * scaleY;

                m_LastMousePos = currentMouse;
            }
            else
            {
                m_IsPanning = false;
            }
        }

        // Paint with left mouse button
        if (m_IsCanvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_IsPanning)
        {
            if (m_HoveredTile.x >= 0 && m_HoveredTile.y >= 0 &&
                m_HoveredTile.x < (int)m_MapData->GetWidth() &&
                m_HoveredTile.y < (int)m_MapData->GetHeight())
            {
                ApplyTool(m_HoveredTile.x, m_HoveredTile.y);
            }
        }

        // Erase with right mouse button (quick erase regardless of tool)
        if (m_IsCanvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right) && !m_IsPanning)
        {
            if (m_HoveredTile.x >= 0 && m_HoveredTile.y >= 0 &&
                m_HoveredTile.x < (int)m_MapData->GetWidth() &&
                m_HoveredTile.y < (int)m_MapData->GetHeight())
            {
                m_MapData->SetTile(m_HoveredTile.x, m_HoveredTile.y, 0);
            }
        }
    }

    // ======================= Tools =======================

    void TileMapEditorPanel::ApplyTool(int tileX, int tileY)
    {
        if (!m_MapData)
            return;

        switch (m_CurrentTool)
        {
            case Tool::Brush:
                m_MapData->SetTile(tileX, tileY, m_SelectedTileID);
                break;
            case Tool::Eraser:
                m_MapData->SetTile(tileX, tileY, 0);
                break;
            case Tool::Fill:
            {
                uint16_t target = m_MapData->GetTile(tileX, tileY);
                if (target != m_SelectedTileID)
                    FloodFill(tileX, tileY, target, m_SelectedTileID);
                break;
            }
        }
    }

    void TileMapEditorPanel::FloodFill(int startX, int startY, uint16_t target, uint16_t replacement)
    {
        if (!m_MapData)
            return;

        int w = (int)m_MapData->GetWidth();
        int h = (int)m_MapData->GetHeight();

        std::queue<glm::ivec2> queue;
        queue.push({startX, startY});

        int filled = 0;
        const int maxFill = w * h;

        while (!queue.empty() && filled < maxFill)
        {
            glm::ivec2 pos = queue.front();
            queue.pop();
            int x = pos.x, y = pos.y;

            if (x < 0 || x >= w || y < 0 || y >= h)
                continue;
            if (m_MapData->GetTile(x, y) != target)
                continue;

            m_MapData->SetTile(x, y, replacement);
            filled++;

            queue.push({x + 1, y});
            queue.push({x - 1, y});
            queue.push({x, y + 1});
            queue.push({x, y - 1});
        }
    }

    // ======================= UI Sections =======================

    void TileMapEditorPanel::UI_Toolbar()
    {
        bool isBrush = (m_CurrentTool == Tool::Brush);
        bool isEraser = (m_CurrentTool == Tool::Eraser);
        bool isFill = (m_CurrentTool == Tool::Fill);

        if (isBrush) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        if (ImGui::Button("Brush [B]"))
            m_CurrentTool = Tool::Brush;
        if (isBrush) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (isEraser) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("Eraser [E]"))
            m_CurrentTool = Tool::Eraser;
        if (isEraser) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (isFill) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        if (ImGui::Button("Fill [G]"))
            m_CurrentTool = Tool::Fill;
        if (isFill) ImGui::PopStyleColor();

        // Keyboard shortcuts (when canvas is focused)
        if (m_IsCanvasFocused || m_IsCanvasHovered)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_B))
                m_CurrentTool = Tool::Brush;
            if (ImGui::IsKeyPressed(ImGuiKey_E))
                m_CurrentTool = Tool::Eraser;
            if (ImGui::IsKeyPressed(ImGuiKey_G))
                m_CurrentTool = Tool::Fill;
        }

        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        ImGui::Text("Selected Tile: %d", (int)m_SelectedTileID);

        if (m_HoveredTile.x >= 0 && m_HoveredTile.y >= 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("  Cursor: (%d, %d)", m_HoveredTile.x, m_HoveredTile.y);
        }
    }

    void TileMapEditorPanel::UI_TilePalette()
    {
        ImGui::Text("Tile Palette");
        ImGui::Separator();

        if (!m_TileSet)
        {
            ImGui::TextDisabled("No TileSet");
            return;
        }

        const auto &tileDefs = m_TileSet->GetTileDefs();
        if (tileDefs.empty())
        {
            ImGui::TextDisabled("No tiles defined");
            return;
        }

        float btnSize = 32.0f;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int cols = (int)(panelWidth / (btnSize + 4.0f));
        if (cols < 1) cols = 1;

        int col = 0;
        for (const auto &[id, def] : tileDefs)
        {
            if (col > 0)
                ImGui::SameLine(0, 4.0f);

            ImGui::PushID((int)id);

            bool isSelected = (m_SelectedTileID == id);
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));

            ImVec4 tintColor = ImVec4(def.Tint.r, def.Tint.g, def.Tint.b, def.Tint.a);
            ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.3f, 0.6f, 0.9f, 1.0f) : tintColor);

            char label[16];
            snprintf(label, sizeof(label), "%d", (int)id);
            if (ImGui::Button(label, ImVec2(btnSize, btnSize)))
            {
                m_SelectedTileID = id;
                if (m_CurrentTool == Tool::Eraser)
                    m_CurrentTool = Tool::Brush;
            }

            ImGui::PopStyleColor();
            if (isSelected)
                ImGui::PopStyleColor();

            // Tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Tile ID: %d", (int)id);
                ImGui::Text("Source: %s", def.SourceType == TileSourceType::Atlas ? "Atlas" : "Individual");
                ImGui::Text("Tint: (%.1f, %.1f, %.1f, %.1f)", def.Tint.r, def.Tint.g, def.Tint.b, def.Tint.a);
                ImGui::EndTooltip();
            }

            ImGui::PopID();

            col++;
            if (col >= cols)
                col = 0;
        }
    }

    void TileMapEditorPanel::UI_Properties()
    {
        ImGui::Text("Properties");
        ImGui::Separator();

        if (!m_MapData)
        {
            ImGui::TextDisabled("No TileMap");
            return;
        }

        // Half Width / Half Height（总格数 (2*Half+1)，中心在格子中心无 0.5 位移）
        int halfW = (int)m_MapData->GetHalfWidth();
        int halfH = (int)m_MapData->GetHalfHeight();
        float cellSize = m_MapData->GetCellSize();

        ImGui::Text("Half Extent");
        if (ImGui::DragInt("Half W", &halfW, 1.0f, 0, 512))
            m_MapData->Resize((uint32_t)(halfW > 0 ? halfW : 0), m_MapData->GetHalfHeight());
        if (ImGui::DragInt("Half H", &halfH, 1.0f, 0, 512))
            m_MapData->Resize(m_MapData->GetHalfWidth(), (uint32_t)(halfH > 0 ? halfH : 0));
        ImGui::TextDisabled("Grid: %u x %u", m_MapData->GetWidth(), m_MapData->GetHeight());
        if (ImGui::DragFloat("Cell", &cellSize, 0.05f, 0.1f, 10.0f))
            m_MapData->SetCellSize(cellSize);

        ImGui::Separator();

        // TileSet info
        ImGui::Text("TileSet");
        AssetHandle tsHandle = m_MapData->GetTileSetHandle();
        if (tsHandle != 0)
            ImGui::TextDisabled("Handle: %llu", (uint64_t)tsHandle);
        else
            ImGui::TextDisabled("None");

        ImGui::Separator();

        // Statistics
        if (m_MapData)
        {
            uint32_t totalTiles = m_MapData->GetWidth() * m_MapData->GetHeight();
            uint32_t filledTiles = 0;
            for (auto t : m_MapData->GetTiles())
            {
                if (t != 0) filledTiles++;
            }
            ImGui::Text("Stats");
            ImGui::TextDisabled("Total: %d", totalTiles);
            ImGui::TextDisabled("Filled: %d", filledTiles);
            ImGui::TextDisabled("Empty: %d", totalTiles - filledTiles);
        }

        ImGui::Separator();

        // Save
        if (ImGui::Button("Save", ImVec2(-1, 0)))
            SaveTileMap();
    }

    void TileMapEditorPanel::SaveTileMap()
    {
        if (!m_MapData || m_TileMapHandle == 0 || !Project::GetActive())
            return;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;

        auto &registry = assetManager->GetAssetRegistry();
        auto it = registry.find(m_TileMapHandle);
        if (it != registry.end())
        {
            auto fullPath = Project::GetAssetFileSystemPath(it->second.FilePath);
            TileMapDataSerializer::Serialize(fullPath, m_MapData);
            HIMII_CORE_INFO("TileMap saved to: {0}", fullPath.string());
        }
    }

} // namespace Himii

#pragma once

#include "Himii/Asset/Asset.h"

#include <vector>
#include <cstdint>

namespace Himii
{

    // TileMapData 资源：存储一层 Tilemap 的地图数据
    // 引用一个 TileSet 资源（通过 AssetHandle），存储实际的 Tile 放置数组
    class TileMapData : public Asset {
    public:
        TileMapData() = default;
        virtual ~TileMapData() = default;

        virtual AssetType GetType() const override
        {
            return AssetType::TileMap;
        }

        // --- TileSet 引用 ---
        void SetTileSetHandle(AssetHandle handle) { m_TileSetHandle = handle; }
        AssetHandle GetTileSetHandle() const { return m_TileSetHandle; }

        // --- 地图尺寸（半宽/半高：总格数 = (2*HalfWidth+1) x (2*HalfHeight+1)，恒为奇数，中心在格子中心无 0.5 位移）---
        uint32_t GetHalfWidth() const { return m_HalfWidth; }
        uint32_t GetHalfHeight() const { return m_HalfHeight; }
        uint32_t GetWidth() const { return 2 * m_HalfWidth + 1; }
        uint32_t GetHeight() const { return 2 * m_HalfHeight + 1; }
        float GetCellSize() const { return m_CellSize; }

        void SetCellSize(float size) { m_CellSize = size; }

        void Resize(uint32_t newHalfWidth, uint32_t newHalfHeight)
        {
            if (newHalfWidth == m_HalfWidth && newHalfHeight == m_HalfHeight)
                return;

            uint32_t newWidth = 2 * newHalfWidth + 1;
            uint32_t newHeight = 2 * newHalfHeight + 1;
            std::vector<uint16_t> newTiles((size_t)newWidth * newHeight, 0);

            uint32_t oldWidth = 2 * m_HalfWidth + 1;
            uint32_t oldHeight = 2 * m_HalfHeight + 1;
            size_t oldSize = (size_t)oldWidth * oldHeight;

            if (m_Tiles.size() >= oldSize)
            {
                int32_t centerOffsetX = (int32_t)newHalfWidth - (int32_t)m_HalfWidth;
                int32_t centerOffsetY = (int32_t)newHalfHeight - (int32_t)m_HalfHeight;

                for (uint32_t oy = 0; oy < oldHeight; ++oy)
                {
                    for (uint32_t ox = 0; ox < oldWidth; ++ox)
                    {
                        int32_t nx = (int32_t)ox + centerOffsetX;
                        int32_t ny = (int32_t)oy + centerOffsetY;
                        if (nx >= 0 && nx < (int32_t)newWidth && ny >= 0 && ny < (int32_t)newHeight)
                            newTiles[(size_t)nx + (size_t)ny * newWidth] = m_Tiles[ox + oy * oldWidth];
                    }
                }
            }

            m_HalfWidth = newHalfWidth;
            m_HalfHeight = newHalfHeight;
            m_Tiles = std::move(newTiles);
        }

        // --- Tile 数据访问 ---
        const std::vector<uint16_t> &GetTiles() const { return m_Tiles; }
        std::vector<uint16_t> &GetTiles() { return m_Tiles; }

        void SetTile(uint32_t x, uint32_t y, uint16_t tileID)
        {
            if (m_Tiles.empty()) return;
            uint32_t w = GetWidth(), h = GetHeight();
            if (x < w && y < h)
                m_Tiles[x + y * w] = tileID;
        }

        uint16_t GetTile(uint32_t x, uint32_t y) const
        {
            if (m_Tiles.empty()) return 0;
            uint32_t w = GetWidth(), h = GetHeight();
            if (x < w && y < h)
                return m_Tiles[x + y * w];
            return 0;
        }

    private:
        AssetHandle m_TileSetHandle = 0;
        uint32_t m_HalfWidth = 0;
        uint32_t m_HalfHeight = 0;
        float m_CellSize = 1.0f;
        std::vector<uint16_t> m_Tiles;
    };

} // namespace Himii

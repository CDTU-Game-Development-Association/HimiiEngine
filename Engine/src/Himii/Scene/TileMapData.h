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

        // --- 地图尺寸 ---
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        float GetCellSize() const { return m_CellSize; }

        void SetCellSize(float size) { m_CellSize = size; }

        void Resize(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;
            m_Tiles.resize((size_t)m_Width * m_Height, 0);
        }

        // --- Tile 数据访问 ---
        const std::vector<uint16_t> &GetTiles() const { return m_Tiles; }
        std::vector<uint16_t> &GetTiles() { return m_Tiles; }

        void SetTile(uint32_t x, uint32_t y, uint16_t tileID)
        {
            if (x < m_Width && y < m_Height)
                m_Tiles[x + y * m_Width] = tileID;
        }

        uint16_t GetTile(uint32_t x, uint32_t y) const
        {
            if (x < m_Width && y < m_Height)
                return m_Tiles[x + y * m_Width];
            return 0;
        }

    private:
        AssetHandle m_TileSetHandle = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        float m_CellSize = 1.0f;
        std::vector<uint16_t> m_Tiles;
    };

} // namespace Himii

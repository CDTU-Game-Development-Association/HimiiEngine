#include "Terrain.h"
#include "Perlin.h"

Terrain::Terrain()
{
    blocks.resize(worldHeight, std::vector<BlockType>(worldWidth, AIR));
    
}

void Terrain::GenerateTerrain()
{
    std::vector<int> heightMap(worldWidth);
    GenerateHeightMap(heightMap);

    for (int x = 0; x < worldWidth; ++x)
    {
        // ʯͷ��
        int stoneDepth = 10 + rand() % 20;
        for (int y = heightMap[x] + stoneDepth; y < worldHeight - 5; ++y)
        {
            blocks[y][x] = STONE;
        }

        // ������
        int dirtThickness = 3 + rand() % 4;
        for (int y = heightMap[x]; y < heightMap[x] + dirtThickness; ++y)
        {
            blocks[y][x] = DIRT;
        }

        // �ݵر��
        if (heightMap[x] > 0)
        {
            blocks[heightMap[x] - 1][x] = GRASS;
        }
    }
}

void Terrain::GenerateHeightMap(std::vector<int> &heightMap)
{
    PerlinNoise pn;
    double scale = 0.05; // ��������Ƶ��

    for (int x = 0; x < worldWidth; ++x)
    {
        // ʹ�ö���������� (���β����˶�)
        double noise = 0.0;
        double amplitude = 30.0;
        double frequency = scale;
        double persistence = 0.5; // ÿ�����˥��ϵ��
        int octaves = 4;          // ��������

        for (int i = 0; i < octaves; ++i)
        {
            noise += pn.noise(x * frequency, 0) * amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        heightMap[x] =  150+static_cast<int>(noise);

        // ȷ���߶��ں���Χ��
        heightMap[x] = std::max(20, std::min(worldHeight - 30, heightMap[x]));
    }
}

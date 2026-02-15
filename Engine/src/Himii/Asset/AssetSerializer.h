#pragma once
#include "Himii/Core/Core.h"
#include <filesystem>
#include "Himii/Scene/SpriteAnimation.h"
#include "Himii/Scene/TileSet.h"
#include "Himii/Scene/TileMapData.h"

namespace Himii
{

    class SpriteAnimationSerializer {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<SpriteAnimation> &animation);
        static Ref<SpriteAnimation> Deserialize(const std::filesystem::path &filepath);
    };

    class TileSetSerializer {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<TileSet> &tileSet);
        static Ref<TileSet> Deserialize(const std::filesystem::path &filepath);
    };

    class TileMapDataSerializer {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<TileMapData> &tileMapData);
        static Ref<TileMapData> Deserialize(const std::filesystem::path &filepath);
    };

} // namespace Himii

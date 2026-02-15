#include "Himii/Asset/AssetSerializer.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "Himii/Core/Log.h"

namespace Himii
{

    // ==================== SpriteAnimation ====================

    void SpriteAnimationSerializer::Serialize(const std::filesystem::path &filepath,
                                              const Ref<SpriteAnimation> &animation)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "SpriteAnimation";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)animation->Handle;

        out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
        for (const auto &frameHandle: animation->GetFrames())
        {
            out << (uint64_t)frameHandle;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<SpriteAnimation> SpriteAnimationSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        std::ifstream stream(filepath);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["AssetType"] || data["AssetType"].as<std::string>() != "SpriteAnimation")
        {
            HIMII_CORE_ERROR("Invalid SpriteAnimation asset: {0}", filepath.string());
            return nullptr;
        }

        Ref<SpriteAnimation> animation = std::make_shared<SpriteAnimation>();

        if (data["Handle"])
            animation->Handle = data["Handle"].as<uint64_t>();

        if (data["Frames"])
        {
            for (auto frameNode: data["Frames"])
            {
                uint64_t handle = frameNode.as<uint64_t>();
                animation->AddFrame(handle);
            }
        }

        return animation;
    }

    // ==================== TileSet ====================

    void TileSetSerializer::Serialize(const std::filesystem::path &filepath, const Ref<TileSet> &tileSet)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "TileSet";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)tileSet->Handle;

        // Atlas Sources
        out << YAML::Key << "AtlasSources" << YAML::Value << YAML::BeginSeq;
        for (const auto &source : tileSet->GetAtlasSources())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)source.TextureHandle;
            out << YAML::Key << "TileSize" << YAML::Value << source.TileSize;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        // Tile Definitions
        out << YAML::Key << "TileDefs" << YAML::Value << YAML::BeginSeq;
        for (const auto &[id, def] : tileSet->GetTileDefs())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "ID" << YAML::Value << (int)def.ID;
            out << YAML::Key << "SourceType" << YAML::Value << (int)def.SourceType;

            if (def.SourceType == TileSourceType::Atlas)
            {
                out << YAML::Key << "AtlasSourceIndex" << YAML::Value << def.AtlasSourceIndex;
                out << YAML::Key << "AtlasCoordsX" << YAML::Value << def.AtlasCoords.x;
                out << YAML::Key << "AtlasCoordsY" << YAML::Value << def.AtlasCoords.y;
            }
            else
            {
                out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)def.IndividualTextureHandle;
            }

            out << YAML::Key << "Tint" << YAML::Value << YAML::Flow << YAML::BeginSeq
                << def.Tint.r << def.Tint.g << def.Tint.b << def.Tint.a << YAML::EndSeq;
            out << YAML::Key << "Collidable" << YAML::Value << def.Collidable;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<TileSet> TileSetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open TileSet file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream strStream;
            strStream << stream.rdbuf();

            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "TileSet")
            {
                HIMII_CORE_ERROR("Invalid TileSet asset: {0}", filepath.string());
                return nullptr;
            }

            Ref<TileSet> tileSet = std::make_shared<TileSet>();

            if (data["Handle"])
                tileSet->Handle = data["Handle"].as<uint64_t>();

            // Atlas Sources
            if (data["AtlasSources"])
            {
                for (auto sourceNode : data["AtlasSources"])
                {
                    TileAtlasSource source;
                    source.TextureHandle = sourceNode["TextureHandle"].as<uint64_t>();
                    source.TileSize = sourceNode["TileSize"].as<uint32_t>();
                    tileSet->AddAtlasSource(source);
                }
            }

            // Tile Definitions
            if (data["TileDefs"])
            {
                for (auto defNode : data["TileDefs"])
                {
                    TileDef def;
                    def.ID = (uint16_t)defNode["ID"].as<int>();
                    def.SourceType = (TileSourceType)defNode["SourceType"].as<int>();

                    if (def.SourceType == TileSourceType::Atlas)
                    {
                        def.AtlasSourceIndex = defNode["AtlasSourceIndex"].as<uint32_t>();
                        def.AtlasCoords.x = defNode["AtlasCoordsX"].as<int>();
                        def.AtlasCoords.y = defNode["AtlasCoordsY"].as<int>();
                    }
                    else
                    {
                        if (defNode["TextureHandle"])
                            def.IndividualTextureHandle = defNode["TextureHandle"].as<uint64_t>();
                    }

                    if (defNode["Tint"] && defNode["Tint"].IsSequence() && defNode["Tint"].size() == 4)
                    {
                        def.Tint.r = defNode["Tint"][0].as<float>();
                        def.Tint.g = defNode["Tint"][1].as<float>();
                        def.Tint.b = defNode["Tint"][2].as<float>();
                        def.Tint.a = defNode["Tint"][3].as<float>();
                    }

                    if (defNode["Collidable"])
                        def.Collidable = defNode["Collidable"].as<bool>();

                    tileSet->AddTileDef(def);
                }
            }

            return tileSet;
        }
        catch (const YAML::Exception &e)
        {
            HIMII_CORE_ERROR("Failed to deserialize TileSet '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

    // ==================== TileMapData ====================

    void TileMapDataSerializer::Serialize(const std::filesystem::path &filepath, const Ref<TileMapData> &tileMapData)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "TileMap";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)tileMapData->Handle;
        out << YAML::Key << "TileSetHandle" << YAML::Value << (uint64_t)tileMapData->GetTileSetHandle();
        out << YAML::Key << "Width" << YAML::Value << tileMapData->GetWidth();
        out << YAML::Key << "Height" << YAML::Value << tileMapData->GetHeight();
        out << YAML::Key << "CellSize" << YAML::Value << tileMapData->GetCellSize();

        // Tiles 数据 - Flow 模式紧凑存储
        out << YAML::Key << "Tiles" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (auto t : tileMapData->GetTiles())
        {
            out << (int)t;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<TileMapData> TileMapDataSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open TileMapData file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream strStream;
            strStream << stream.rdbuf();

            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "TileMap")
            {
                HIMII_CORE_ERROR("Invalid TileMapData asset: {0}", filepath.string());
                return nullptr;
            }

            Ref<TileMapData> tileMapData = std::make_shared<TileMapData>();

            if (data["Handle"])
                tileMapData->Handle = data["Handle"].as<uint64_t>();

            if (data["TileSetHandle"])
                tileMapData->SetTileSetHandle(data["TileSetHandle"].as<uint64_t>());

            uint32_t width = data["Width"] ? data["Width"].as<uint32_t>() : 0;
            uint32_t height = data["Height"] ? data["Height"].as<uint32_t>() : 0;

            if (data["CellSize"])
                tileMapData->SetCellSize(data["CellSize"].as<float>());

            if (width > 0 && height > 0)
            {
                tileMapData->Resize(width, height);

                if (data["Tiles"] && data["Tiles"].IsSequence())
                {
                    auto &tiles = tileMapData->GetTiles();
                    for (size_t i = 0; i < data["Tiles"].size() && i < tiles.size(); ++i)
                    {
                        tiles[i] = (uint16_t)data["Tiles"][i].as<int>();
                    }
                }
            }

            return tileMapData;
        }
        catch (const YAML::Exception &e)
        {
            HIMII_CORE_ERROR("Failed to deserialize TileMapData '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

} // namespace Himii

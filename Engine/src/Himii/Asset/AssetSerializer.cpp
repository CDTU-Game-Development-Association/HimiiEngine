#include "Himii/Asset/AssetSerializer.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "Himii/Core/Log.h"
#include "Himii/Scene/ParticleEmitterAsset.h"

namespace Himii
{

    // ==================== ParticleEmitterAsset ====================

    static void WriteVec3(YAML::Emitter& out, const char* key, const glm::vec3& v)
    {
        out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    }
    static void WriteVec4(YAML::Emitter& out, const char* key, const glm::vec4& v)
    {
        out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    }
    static glm::vec3 ReadVec3(const YAML::Node& node, const char* key, const glm::vec3& def = {})
    {
        if (!node[key] || !node[key].IsSequence() || node[key].size() < 3) return def;
        return { node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>() };
    }
    static glm::vec4 ReadVec4(const YAML::Node& node, const char* key, const glm::vec4& def = {})
    {
        if (!node[key] || !node[key].IsSequence() || node[key].size() < 4) return def;
        return { node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>(), node[key][3].as<float>() };
    }

    void ParticleEmitterAssetSerializer::Serialize(const std::filesystem::path& filepath, const Ref<ParticleEmitterAsset>& asset)
    {
        if (!asset) return;
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "ParticleEmitter";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)asset->Handle;

        const auto& p = asset->TemplateProps;
        WriteVec3(out, "Position", p.position);
        WriteVec3(out, "Velocity", p.velocity);
        WriteVec3(out, "VelocityVariation", p.velocityVariation);
        out << YAML::Key << "Lifetime" << YAML::Value << p.lifetime;
        WriteVec4(out, "ColorBegin", p.colorBegin);
        WriteVec4(out, "ColorEnd", p.colorEnd);
        out << YAML::Key << "SizeBegin" << YAML::Value << p.sizeBegin;
        out << YAML::Key << "SizeEnd" << YAML::Value << p.sizeEnd;

        out << YAML::Key << "EmissionRate" << YAML::Value << asset->EmissionRate;
        out << YAML::Key << "Looping" << YAML::Value << asset->Looping;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<ParticleEmitterAsset> ParticleEmitterAssetSerializer::Deserialize(const std::filesystem::path& filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open ParticleEmitterAsset file: {0}", filepath.string());
                return nullptr;
            }
            std::stringstream strStream;
            strStream << stream.rdbuf();
            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "ParticleEmitter")
            {
                HIMII_CORE_ERROR("Invalid ParticleEmitterAsset: {0}", filepath.string());
                return nullptr;
            }

            Ref<ParticleEmitterAsset> asset = std::make_shared<ParticleEmitterAsset>();
            if (data["Handle"])
                asset->Handle = data["Handle"].as<uint64_t>();

            asset->TemplateProps.position = ReadVec3(data, "Position");
            asset->TemplateProps.velocity = ReadVec3(data, "Velocity");
            asset->TemplateProps.velocityVariation = ReadVec3(data, "VelocityVariation");
            if (data["Lifetime"]) asset->TemplateProps.lifetime = data["Lifetime"].as<float>();
            asset->TemplateProps.colorBegin = ReadVec4(data, "ColorBegin", { 1, 1, 1, 1 });
            asset->TemplateProps.colorEnd = ReadVec4(data, "ColorEnd", { 0, 0, 0, 0 });
            if (data["SizeBegin"]) asset->TemplateProps.sizeBegin = data["SizeBegin"].as<float>();
            if (data["SizeEnd"]) asset->TemplateProps.sizeEnd = data["SizeEnd"].as<float>();

            if (data["EmissionRate"]) asset->EmissionRate = data["EmissionRate"].as<float>();
            if (data["Looping"]) asset->Looping = data["Looping"].as<bool>();

            return asset;
        }
        catch (const YAML::Exception& e)
        {
            HIMII_CORE_ERROR("Failed to deserialize ParticleEmitterAsset '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

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
        out << YAML::Key << "HalfWidth" << YAML::Value << tileMapData->GetHalfWidth();
        out << YAML::Key << "HalfHeight" << YAML::Value << tileMapData->GetHalfHeight();
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

            uint32_t halfWidth = 0, halfHeight = 0;
            if (data["HalfWidth"] && data["HalfHeight"])
            {
                halfWidth = data["HalfWidth"].as<uint32_t>();
                halfHeight = data["HalfHeight"].as<uint32_t>();
            }
            else if (data["Width"] && data["Height"])
            {
                uint32_t w = data["Width"].as<uint32_t>();
                uint32_t h = data["Height"].as<uint32_t>();
                halfWidth = (w > 0) ? (w - 1) / 2 : 0;
                halfHeight = (h > 0) ? (h - 1) / 2 : 0;
            }

            if (data["CellSize"])
                tileMapData->SetCellSize(data["CellSize"].as<float>());

            {
                tileMapData->Resize(halfWidth, halfHeight);

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

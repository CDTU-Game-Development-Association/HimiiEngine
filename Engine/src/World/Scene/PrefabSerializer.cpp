#include "Hepch.h"
#include "PrefabSerializer.h"
#include "World/Scene/SceneSerializer.h"
#include "Module/Script/ScriptEngine.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    namespace
    {
        struct RootRelationshipRestore
        {
            Entity Root;
            bool HasRelationship = false;
            UUID ParentIdentifier = 0;
            uint32_t SiblingIndex = 0;

            explicit RootRelationshipRestore(Entity root)
                : Root(root)
            {
                if (!root || !root.HasComponent<RelationshipComponent>())
                    return;

                HasRelationship = true;
                const RelationshipComponent& relationship = root.GetComponent<RelationshipComponent>();
                ParentIdentifier = relationship.Parent;
                SiblingIndex = relationship.SiblingIndex;
                RelationshipComponent& mutableRelationship = root.GetComponent<RelationshipComponent>();
                mutableRelationship.Parent = 0;
                mutableRelationship.SiblingIndex = 0;
            }

            ~RootRelationshipRestore()
            {
                if (!HasRelationship || !Root || !Root.HasComponent<RelationshipComponent>())
                    return;

                RelationshipComponent& relationship = Root.GetComponent<RelationshipComponent>();
                relationship.Parent = ParentIdentifier;
                relationship.SiblingIndex = SiblingIndex;
            }
        };

        void AppendSubtree(const Ref<Scene>& scene, Entity entity, std::vector<Entity>& entities)
        {
            if (!entity)
                return;

            entities.push_back(entity);
            const std::vector<UUID> childIdentifiers = scene->GetEntityChildren(entity);
            for (UUID childIdentifier : childIdentifiers)
                AppendSubtree(scene, scene->GetEntityByUUID(childIdentifier), entities);
        }

        bool EntityNodeIsUserInterface(const YAML::Node& entityNode)
        {
            return entityNode["RectTransformComponent"].IsDefined()
                   || entityNode["UITransformComponent"].IsDefined();
        }

        uint64_t ReadParentIdentifier(const YAML::Node& entityNode)
        {
            const YAML::Node relationshipComponent = entityNode["RelationshipComponent"];
            if (!relationshipComponent || !relationshipComponent["Parent"])
                return 0;

            return relationshipComponent["Parent"].as<uint64_t>();
        }

        void RemapScriptEntityFields(YAML::Node entityNode,
                                     const std::unordered_map<uint64_t, uint64_t>& identifierRemap)
        {
            YAML::Node scriptComponent = entityNode["ScriptComponent"];
            if (!scriptComponent)
                return;

            YAML::Node scriptFields = scriptComponent["ScriptFields"];
            if (!scriptFields || !scriptFields.IsSequence())
                return;

            for (YAML::Node scriptField : scriptFields)
            {
                if (!scriptField["Type"] || !scriptField["Data"])
                    continue;

                const int fieldType = scriptField["Type"].as<int>();
                if (fieldType != static_cast<int>(ScriptFieldType::Entity)
                    && fieldType != static_cast<int>(ScriptFieldType::Button))
                    continue;

                const uint64_t referencedIdentifier = scriptField["Data"].as<uint64_t>();
                if (referencedIdentifier == 0)
                    continue;

                const auto remapIterator = identifierRemap.find(referencedIdentifier);
                scriptField["Data"] = remapIterator == identifierRemap.end()
                                              ? static_cast<uint64_t>(0)
                                              : remapIterator->second;
            }
        }

        void DestroyCreatedEntities(const Ref<Scene>& scene, const std::vector<uint64_t>& createdIdentifiers)
        {
            for (auto identifierIterator = createdIdentifiers.rbegin();
                 identifierIterator != createdIdentifiers.rend();
                 ++identifierIterator)
            {
                Entity createdEntity = scene->GetEntityByUUID(*identifierIterator);
                if (createdEntity)
                    scene->DestroyEntity(createdEntity);
            }
        }
    }

    bool PrefabSerializer::Save(const Ref<Scene>& scene, Entity entity, const std::filesystem::path& filepath,
                                const std::string& prefabName)
    {
        if (!scene || !entity)
            return false;

        scene->RebuildHierarchyCache();

        std::vector<Entity> subtreeEntities;
        AppendSubtree(scene, entity, subtreeEntities);
        if (subtreeEntities.empty())
            return false;

        const std::string resolvedName = prefabName.empty() ? entity.GetName() : prefabName;

        YAML::Emitter output;
        output << YAML::BeginMap;
        output << YAML::Key << "AssetType" << YAML::Value << "Prefab";
        output << YAML::Key << "PrefabName" << YAML::Value << resolvedName;
        output << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        {
            // 只在写出期间清掉根的外部父节点，避免把场景里的父实体写进资产。
            RootRelationshipRestore restoreRootRelationship(entity);
            for (Entity subtreeEntity : subtreeEntities)
                SceneSerializer::SerializeEntity(output, subtreeEntity);
        }

        output << YAML::EndSeq;
        output << YAML::EndMap;

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            HIMII_CORE_ERROR("PrefabSerializer::Save: failed to open {0}", filepath.string());
            return false;
        }

        file << output.c_str();
        return true;
    }

    Entity PrefabSerializer::Instantiate(const Ref<Scene>& scene, const std::filesystem::path& filepath)
    {
        if (!scene || !std::filesystem::exists(filepath))
            return {};

        std::vector<uint64_t> createdIdentifiers;
        try
        {
            YAML::Node rootNode = YAML::LoadFile(filepath.string());
            YAML::Node entitiesNode = rootNode["Entities"];
            if (!entitiesNode || !entitiesNode.IsSequence() || entitiesNode.size() == 0)
            {
                HIMII_CORE_ERROR("PrefabSerializer::Instantiate: {0} has no Entities sequence",
                                 filepath.string());
                return {};
            }

            std::vector<YAML::Node> entityNodes;
            std::unordered_map<uint64_t, uint64_t> parentByIdentifier;
            std::unordered_map<uint64_t, bool> userInterfaceByIdentifier;
            std::unordered_set<uint64_t> identifiers;
            bool containsCanvas = false;

            for (YAML::Node entityNode : entitiesNode)
            {
                if (!entityNode["Entity"])
                {
                    HIMII_CORE_ERROR("PrefabSerializer::Instantiate: entity block missing Entity in {0}",
                                     filepath.string());
                    return {};
                }

                const uint64_t entityIdentifier = entityNode["Entity"].as<uint64_t>();
                if (entityIdentifier == 0 || !identifiers.insert(entityIdentifier).second)
                {
                    HIMII_CORE_ERROR("PrefabSerializer::Instantiate: duplicate or empty entity id in {0}",
                                     filepath.string());
                    return {};
                }

                entityNodes.push_back(entityNode);
                parentByIdentifier.emplace(entityIdentifier, ReadParentIdentifier(entityNode));
                userInterfaceByIdentifier.emplace(entityIdentifier, EntityNodeIsUserInterface(entityNode));
                if (entityNode["CanvasComponent"].IsDefined())
                    containsCanvas = true;
            }

            if (containsCanvas && scene->FindCanvasEntity())
            {
                HIMII_CORE_ERROR("PrefabSerializer::Instantiate: scene already has a Canvas, refused {0}",
                                 filepath.string());
                return {};
            }

            uint64_t rootIdentifier = 0;
            int rootCount = 0;
            for (const auto& [entityIdentifier, parentIdentifier] : parentByIdentifier)
            {
                if (parentIdentifier == 0)
                {
                    ++rootCount;
                    rootIdentifier = entityIdentifier;
                    continue;
                }

                if (identifiers.find(parentIdentifier) == identifiers.end())
                {
                    HIMII_CORE_ERROR("PrefabSerializer::Instantiate: parent outside prefab in {0}",
                                     filepath.string());
                    return {};
                }

                if (userInterfaceByIdentifier[entityIdentifier] != userInterfaceByIdentifier[parentIdentifier])
                {
                    HIMII_CORE_ERROR("PrefabSerializer::Instantiate: mixed transform domain in {0}",
                                     filepath.string());
                    return {};
                }
            }

            if (rootCount != 1)
            {
                HIMII_CORE_ERROR("PrefabSerializer::Instantiate: expected one root in {0}", filepath.string());
                return {};
            }

            for (uint64_t entityIdentifier : identifiers)
            {
                std::unordered_set<uint64_t> visitedIdentifiers;
                uint64_t cursor = entityIdentifier;
                while (cursor != 0)
                {
                    if (!visitedIdentifiers.insert(cursor).second)
                    {
                        HIMII_CORE_ERROR("PrefabSerializer::Instantiate: parent cycle in {0}",
                                         filepath.string());
                        return {};
                    }

                    cursor = parentByIdentifier[cursor];
                }
            }

            std::unordered_map<uint64_t, uint64_t> identifierRemap;
            std::unordered_set<uint64_t> allocatedIdentifiers;
            identifierRemap.reserve(identifiers.size());
            for (uint64_t entityIdentifier : identifiers)
            {
                uint64_t allocatedIdentifier = 0;
                do
                {
                    allocatedIdentifier = UUID();
                } while (allocatedIdentifier == 0
                         || scene->GetEntityByUUID(allocatedIdentifier)
                         || allocatedIdentifiers.find(allocatedIdentifier) != allocatedIdentifiers.end());

                allocatedIdentifiers.insert(allocatedIdentifier);
                identifierRemap.emplace(entityIdentifier, allocatedIdentifier);
            }

            const uint64_t instantiatedRootIdentifier = identifierRemap[rootIdentifier];

            for (YAML::Node entityNode : entityNodes)
            {
                const uint64_t sourceIdentifier = entityNode["Entity"].as<uint64_t>();
                const uint64_t parentIdentifier = parentByIdentifier[sourceIdentifier];
                entityNode["Entity"] = identifierRemap[sourceIdentifier];

                if (parentIdentifier != 0)
                {
                    YAML::Node relationshipComponent = entityNode["RelationshipComponent"];
                    relationshipComponent["Parent"] = identifierRemap[parentIdentifier];
                }

                RemapScriptEntityFields(entityNode, identifierRemap);
            }

            for (YAML::Node entityNode : entityNodes)
            {
                const uint64_t instantiatedIdentifier = entityNode["Entity"].as<uint64_t>();
                SceneSerializer::DeserializeEntity(entityNode, scene);
                createdIdentifiers.push_back(instantiatedIdentifier);
            }

            scene->RebuildHierarchyCache();
            return scene->GetEntityByUUID(instantiatedRootIdentifier);
        }
        catch (const YAML::Exception& exception)
        {
            HIMII_CORE_ERROR("PrefabSerializer::Instantiate: YAML error in {0}: {1}",
                             filepath.string(), exception.what());
        }
        catch (const std::exception& exception)
        {
            HIMII_CORE_ERROR("PrefabSerializer::Instantiate: failed {0}: {1}",
                             filepath.string(), exception.what());
        }

        DestroyCreatedEntities(scene, createdIdentifiers);
        return {};
    }
}

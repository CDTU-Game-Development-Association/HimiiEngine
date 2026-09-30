#pragma once

#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"

#include <filesystem>
#include <string>

namespace Himii
{
    class PrefabSerializer
    {
    public:
        /// 把实体及其全部后代写成 `.hprefab`。根在文件里的 Parent 为 0，不记录场景中的外部父节点。
        static bool Save(const Ref<Scene>& scene, Entity entity, const std::filesystem::path& filepath,
                         const std::string& prefabName = {});

        /// 整份成功时返回新的根实体，失败时不留下任何实体。不读取旧的单实体根映射格式。
        static Entity Instantiate(const Ref<Scene>& scene, const std::filesystem::path& filepath);
    };
}

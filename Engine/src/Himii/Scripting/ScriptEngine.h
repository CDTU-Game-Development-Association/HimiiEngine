#pragma once

#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include "glm/glm.hpp"

#include "Himii/Core/Timestep.h"
#include "Himii/Core/UUID.h"

namespace Himii {

    class Scene;
    class Entity;

    enum class ScriptFieldType
    {
        None = 0, Float, Double, Bool, Char, Byte, Short, Int, Long, UByte, UShort, UInt, ULong, Vector2, Vector3, Vector4, Entity
    };

    struct ScriptFieldInstance
    {
        ScriptFieldType Type = ScriptFieldType::None;

        uint8_t m_Buffer[16];

        template<typename T>
        T GetValue() const
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            return *(const T*)m_Buffer;
        }

        template<typename T>
        void SetValue(T value)
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            memcpy(m_Buffer, &value, sizeof(T));
        }
    };

    using ScriptFieldMap = std::unordered_map<std::string, ScriptFieldInstance>;

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& filepath);
        static void CompileAndReloadAppAssembly(const std::filesystem::path &projectPath);
		static void LoadAppAssembly(const std::filesystem::path& filepath);

		// 运行时生命周期
        static void OnCreateEntity(Entity entity);
        static void OnRuntimeStart(Scene* scene);
        static void OnRuntimeStop();

		static void OnUpdateScript(Entity entity, Timestep ts);

		static bool EntityClassExists(const std::string &fullClassName);

        // [NEW] Instantiate C# class and return handle
        static void* InstantiateClass(UUID entityID, const std::string& className);
        
        static ScriptFieldMap& GetScriptFieldMap(Entity entity);

        // [NEW] Reflection helpers
        static std::string GetFields(void *instanceHandle);
        static bool GetFloat(void *instanceHandle, const std::string &fieldName, float &outValue);
        static void SetFloat(void *instanceHandle, const std::string &fieldName, float value);

        static bool GetInt(void *instanceHandle, const std::string &fieldName, int &outValue);
        static void SetInt(void *instanceHandle, const std::string &fieldName, int value);

        static bool GetBool(void *instanceHandle, const std::string &fieldName, bool &outValue);
        static void SetBool(void *instanceHandle, const std::string &fieldName, bool value);

        static bool GetVector2(void *instanceHandle, const std::string &fieldName, glm::vec2 &outValue);
        static void SetVector2(void *instanceHandle, const std::string &fieldName, const glm::vec2 &value);
        static bool GetVector3(void *instanceHandle, const std::string &fieldName, glm::vec3 &outValue);
        static void SetVector3(void *instanceHandle, const std::string &fieldName, const glm::vec3 &value);
        static bool GetVector4(void *instanceHandle, const std::string &fieldName, glm::vec4 &outValue);
        static void SetVector4(void *instanceHandle, const std::string &fieldName, const glm::vec4 &value);

        static void* GetEntityScriptInstance(UUID entityID);

        static std::string Serialize(void* instanceHandle);
        static void Deserialize(void* instanceHandle, const std::string& json);

		static Scene* GetSceneContext();

    private:
        static void InitInterop();

    private:
        static Scene* s_SceneContext;
        // Map Entity UUID to GCHandle (void*)
        static std::unordered_map<UUID, void*> s_EntityInstanceMap;
	};


}

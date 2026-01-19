#pragma once
#include "Himii/Renderer/EditorCamera.h"
#include "Himii/Renderer/Camera.h"
#include "Himii/Renderer/Texture.h"

namespace Himii {

    class Renderer3D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const EditorCamera& camera);
        static void BeginScene(const Camera& camera, const glm::mat4& transform);
        static void EndScene();

        static void Flush();

        // Primitives
        static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID = -1);
        static void DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
        static void DrawPlane(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);

        // Skybox
        static void DrawSkybox(const Ref<TextureCube> &cubemap, const Camera &camera, const glm::mat4 &cameraTransform);
        static void DrawSkybox(const Ref<TextureCube> &cubemap, const EditorCamera &camera);
        
        // Grid
        static void DrawGrid(const EditorCamera& camera);
        static void DrawGrid(const Camera& camera, const glm::mat4& transform);

        // Stats
        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t CubeCount = 0;

            uint32_t GetTotalVertexCount() const { return CubeCount * 24; } // 4 verts * 6 faces
            uint32_t GetTotalIndexCount() const { return CubeCount * 36; }  // 6 indices * 6 faces
        };

        static void ResetStats();
        static Statistics GetStatistics();

    private:
        static void StartBatch();
        static void NextBatch();
    };

}

#include "Hepch.h"

#if defined(_WIN32)
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

#include "Renderer3D.h"
#include "Himii/Scene/SceneCamera.h"

// 需要直接调用 OpenGL 函数修改深度测试模式 (RenderCommand 暂时没封装 SetDepthFunc)
// #include <glad/glad.h>

#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/UniformBuffer.h"
#include "Himii/Renderer/VertexArray.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{

    struct CubeVertex {
        float Color[4];
        float Position[3];
        float Normal[3];
        int EntityID;
    };

    struct Renderer3DData {
        static const uint32_t MaxCubes = 10000;
        static const uint32_t MaxVertices = MaxCubes * 24;
        static const uint32_t MaxIndices = MaxCubes * 36;

        // Cube
        Ref<VertexArray> CubeVertexArray;
        Ref<VertexBuffer> CubeVertexBuffer;
        Ref<Shader> CubeShader;

        // Skybox
        Ref<VertexArray> SkyboxVAO;
        Ref<VertexBuffer> SkyboxVBO;
        Ref<Shader> SkyboxShader;

        struct SkyboxData {
            glm::mat4 View;
            glm::mat4 Projection;
        };
        Ref<UniformBuffer> SkyboxUniformBuffer;

        // Grid
        Ref<VertexArray> GridVAO;
        Ref<VertexBuffer> GridVBO;
        Ref<Shader> GridShader;
        struct GridData {
            glm::mat4 View;
            glm::mat4 Proj;
            float Near;
            float Far;
        }; 
        Ref<UniformBuffer> GridUniformBuffer;

        uint32_t CubeIndexCount = 0;
        CubeVertex *CubeVertexBufferBase = nullptr;
        CubeVertex *CubeVertexBufferPtr = nullptr;

        // Plane
        Ref<VertexArray> PlaneVertexArray;
        Ref<VertexBuffer> PlaneVertexBuffer;
        // Plane Shader reuse CubeShader? Yes, standard lighting.
        // Wait, Plane is just a flat cube face.
        uint32_t PlaneIndexCount = 0;
        CubeVertex *PlaneVertexBufferBase = nullptr;
        CubeVertex *PlaneVertexBufferPtr = nullptr;

        glm::vec3 CubeVertexPositions[24];
        glm::vec3 CubeVertexNormals[24];

        Renderer3D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
        Ref<UniformBuffer> CameraUniformBuffer;
    };

    static Renderer3DData s_Data;

    void Renderer3D::Init()
    {
        s_Data.CubeVertexArray = VertexArray::Create();
        s_Data.CubeVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CubeVertex));

        s_Data.CubeVertexBuffer->SetLayout({{ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float3, "a_Normal"},
                                            {ShaderDataType::Int, "a_EntityID"}});

        s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer);
        s_Data.CubeVertexBufferBase = new CubeVertex[s_Data.MaxVertices];

        uint32_t *cubeIndices = new uint32_t[s_Data.MaxIndices];

        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 36)
        {
            // (索引生成代码保持不变...)
            // Front face
            cubeIndices[i + 0] = offset + 0;
            cubeIndices[i + 1] = offset + 1;
            cubeIndices[i + 2] = offset + 2;
            cubeIndices[i + 3] = offset + 2;
            cubeIndices[i + 4] = offset + 3;
            cubeIndices[i + 5] = offset + 0;
            // Right face
            cubeIndices[i + 6] = offset + 4;
            cubeIndices[i + 7] = offset + 5;
            cubeIndices[i + 8] = offset + 6;
            cubeIndices[i + 9] = offset + 6;
            cubeIndices[i + 10] = offset + 7;
            cubeIndices[i + 11] = offset + 4;
            // Back face
            cubeIndices[i + 12] = offset + 8;
            cubeIndices[i + 13] = offset + 9;
            cubeIndices[i + 14] = offset + 10;
            cubeIndices[i + 15] = offset + 10;
            cubeIndices[i + 16] = offset + 11;
            cubeIndices[i + 17] = offset + 8;
            // Left face
            cubeIndices[i + 18] = offset + 12;
            cubeIndices[i + 19] = offset + 13;
            cubeIndices[i + 20] = offset + 14;
            cubeIndices[i + 21] = offset + 14;
            cubeIndices[i + 22] = offset + 15;
            cubeIndices[i + 23] = offset + 12;
            // Top face
            cubeIndices[i + 24] = offset + 16;
            cubeIndices[i + 25] = offset + 17;
            cubeIndices[i + 26] = offset + 18;
            cubeIndices[i + 27] = offset + 18;
            cubeIndices[i + 28] = offset + 19;
            cubeIndices[i + 29] = offset + 16;
            // Bottom face
            cubeIndices[i + 30] = offset + 20;
            cubeIndices[i + 31] = offset + 21;
            cubeIndices[i + 32] = offset + 22;
            cubeIndices[i + 33] = offset + 22;
            cubeIndices[i + 34] = offset + 23;
            cubeIndices[i + 35] = offset + 20;

            offset += 24;
        }

        Ref<IndexBuffer> cubeIB = IndexBuffer::Create(cubeIndices, s_Data.MaxIndices);
        s_Data.CubeVertexArray->SetIndexBuffer(cubeIB);
        delete[] cubeIndices;

        s_Data.CubeShader = Shader::Create("assets/shaders/Renderer3D_Cube.glsl");
        if (!s_Data.CubeShader)
        {
            HIMII_CORE_ERROR("Failed to load Renderer3D_Cube.glsl!");
        }

        s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);

        // 初始化立方体顶点数据 (保持不变...)
        s_Data.CubeVertexPositions[0] = {-0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[0] = {0.0f, 0.0f, 1.0f};
        s_Data.CubeVertexPositions[1] = {0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[1] = {0.0f, 0.0f, 1.0f};
        s_Data.CubeVertexPositions[2] = {0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[2] = {0.0f, 0.0f, 1.0f};
        s_Data.CubeVertexPositions[3] = {-0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[3] = {0.0f, 0.0f, 1.0f};
        s_Data.CubeVertexPositions[4] = {0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[4] = {1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[5] = {0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[5] = {1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[6] = {0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[6] = {1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[7] = {0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[7] = {1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[8] = {0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[8] = {0.0f, 0.0f, -1.0f};
        s_Data.CubeVertexPositions[9] = {-0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[9] = {0.0f, 0.0f, -1.0f};
        s_Data.CubeVertexPositions[10] = {-0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[10] = {0.0f, 0.0f, -1.0f};
        s_Data.CubeVertexPositions[11] = {0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[11] = {0.0f, 0.0f, -1.0f};
        s_Data.CubeVertexPositions[12] = {-0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[12] = {-1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[13] = {-0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[13] = {-1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[14] = {-0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[14] = {-1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[15] = {-0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[15] = {-1.0f, 0.0f, 0.0f};
        s_Data.CubeVertexPositions[16] = {-0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[16] = {0.0f, 1.0f, 0.0f};
        s_Data.CubeVertexPositions[17] = {0.5f, 0.5f, 0.5f};
        s_Data.CubeVertexNormals[17] = {0.0f, 1.0f, 0.0f};
        s_Data.CubeVertexPositions[18] = {0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[18] = {0.0f, 1.0f, 0.0f};
        s_Data.CubeVertexPositions[19] = {-0.5f, 0.5f, -0.5f};
        s_Data.CubeVertexNormals[19] = {0.0f, 1.0f, 0.0f};
        s_Data.CubeVertexPositions[20] = {-0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[20] = {0.0f, -1.0f, 0.0f};
        s_Data.CubeVertexPositions[21] = {0.5f, -0.5f, -0.5f};
        s_Data.CubeVertexNormals[21] = {0.0f, -1.0f, 0.0f};
        s_Data.CubeVertexPositions[22] = {0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[22] = {0.0f, -1.0f, 0.0f};
        s_Data.CubeVertexPositions[23] = {-0.5f, -0.5f, 0.5f};
        s_Data.CubeVertexNormals[23] = {0.0f, -1.0f, 0.0f};

        // Skybox 顶点数据
        float skyboxVertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                                  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                                  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                                  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                                  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                                  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                                  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                                  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                                  1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

        s_Data.SkyboxVAO = VertexArray::Create();
        s_Data.SkyboxVBO = VertexBuffer::Create(sizeof(skyboxVertices));
        s_Data.SkyboxVBO->SetData(skyboxVertices, sizeof(skyboxVertices));
        s_Data.SkyboxVBO->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        s_Data.SkyboxVAO->AddVertexBuffer(s_Data.SkyboxVBO);

        s_Data.SkyboxShader = Shader::Create("assets/shaders/Skybox.glsl");
        s_Data.SkyboxUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::SkyboxData), 1);

        // Grid Init
        s_Data.GridVAO = VertexArray::Create();
        s_Data.GridVBO = VertexBuffer::Create(sizeof(float) * 6 * 3);
        float gridVertices[] = {
             1.0f,  1.0f,  0.0f,
            -1.0f,  1.0f,  0.0f, 
            -1.0f, -1.0f,  0.0f,
            -1.0f, -1.0f,  0.0f,
             1.0f, -1.0f,  0.0f,
             1.0f,  1.0f,  0.0f
        };
        // Wait, shader unprojects. We need a quad in Clip Space? 
        // Or we pass object space quad and MVP?
        // VS says: gl_Position = vec4(p, 1.0);
        // So p should be in clip space XY [-1, 1]. Z can be 0.
        // The array above is X Y Z.
        
        s_Data.GridVBO->SetData(gridVertices, sizeof(gridVertices));
        s_Data.GridVBO->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        s_Data.GridVAO->AddVertexBuffer(s_Data.GridVBO);
        
        s_Data.GridVAO->AddVertexBuffer(s_Data.GridVBO);
        
        s_Data.GridShader = Shader::Create("assets/shaders/Grid.glsl");
        s_Data.GridUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::GridData), 2); // Binding 2

        // Plane Init
        s_Data.PlaneVertexArray = VertexArray::Create();
        s_Data.PlaneVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CubeVertex));
        s_Data.PlaneVertexBuffer->SetLayout({{ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float3, "a_Normal"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.PlaneVertexArray->AddVertexBuffer(s_Data.PlaneVertexBuffer);
        s_Data.PlaneVertexBufferBase = new CubeVertex[s_Data.MaxVertices];
        
        // Plane Indices (Quad)
        uint32_t* planeIndices = new uint32_t[s_Data.MaxIndices]; // Reusing max indices limit
        offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
        {
            planeIndices[i + 0] = offset + 0;
            planeIndices[i + 1] = offset + 1;
            planeIndices[i + 2] = offset + 2;
            planeIndices[i + 3] = offset + 2;
            planeIndices[i + 4] = offset + 3;
            planeIndices[i + 5] = offset + 0;
            offset += 4;
        }
        Ref<IndexBuffer> planeIB = IndexBuffer::Create(planeIndices, s_Data.MaxIndices);
        s_Data.PlaneVertexArray->SetIndexBuffer(planeIB);
        delete[] planeIndices;
    }

    void Renderer3D::Shutdown()
    {
        delete[] s_Data.CubeVertexBufferBase;
        delete[] s_Data.PlaneVertexBufferBase;
    }

    void Renderer3D::BeginScene(const EditorCamera &camera)
    {
        RenderCommand::SetDepthTest(true);

        Renderer3DData::CameraData cameraData;
        cameraData.ViewProjection = camera.GetViewProjection();
        s_Data.CameraUniformBuffer->SetData(&cameraData, sizeof(Renderer3DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer3D::BeginScene(const Camera &camera, const glm::mat4 &transform)
    {
        RenderCommand::SetDepthTest(true);

        Renderer3DData::CameraData cameraData;
        cameraData.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        s_Data.CameraUniformBuffer->SetData(&cameraData, sizeof(Renderer3DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer3D::EndScene()
    {
        Flush();
    }

    void Renderer3D::StartBatch()
    {
        s_Data.CubeIndexCount = 0;
        s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;

        s_Data.PlaneIndexCount = 0;
        s_Data.PlaneVertexBufferPtr = s_Data.PlaneVertexBufferBase;
    }

    void Renderer3D::Flush()
    {
        if (s_Data.CubeIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.CubeVertexBufferPtr - (uint8_t *)s_Data.CubeVertexBufferBase);
            s_Data.CubeVertexBuffer->SetData(s_Data.CubeVertexBufferBase, dataSize);

            s_Data.CubeShader->Bind();
            RenderCommand::DrawIndexed(s_Data.CubeVertexArray, s_Data.CubeIndexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.PlaneIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.PlaneVertexBufferPtr - (uint8_t *)s_Data.PlaneVertexBufferBase);
            s_Data.PlaneVertexBuffer->SetData(s_Data.PlaneVertexBufferBase, dataSize);

            s_Data.CubeShader->Bind(); // Using same shader
            RenderCommand::DrawIndexed(s_Data.PlaneVertexArray, s_Data.PlaneIndexCount);
            s_Data.Stats.DrawCalls++;
        }
    }

    void Renderer3D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void Renderer3D::DrawCube(const glm::vec3 &position, const glm::vec3 &size, const glm::vec4 &color, int entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), size);
        DrawCube(transform, color, entityID);
    }

    void Renderer3D::DrawCube(const glm::mat4 &transform, const glm::vec4 &color, int entityID)
    {
        if (s_Data.CubeIndexCount >= Renderer3DData::MaxIndices)
            NextBatch();

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

        for (size_t i = 0; i < 24; i++)
        {
            s_Data.CubeVertexBufferPtr->Color[0] = color.r;
            s_Data.CubeVertexBufferPtr->Color[1] = color.g;
            s_Data.CubeVertexBufferPtr->Color[2] = color.b;
            s_Data.CubeVertexBufferPtr->Color[3] = color.a;

            glm::vec4 transformedPos = transform * glm::vec4(s_Data.CubeVertexPositions[i], 1.0f);
            s_Data.CubeVertexBufferPtr->Position[0] = transformedPos.x;
            s_Data.CubeVertexBufferPtr->Position[1] = transformedPos.y;
            s_Data.CubeVertexBufferPtr->Position[2] = transformedPos.z;

            glm::vec3 transformedNormal = normalMatrix * s_Data.CubeVertexNormals[i];
            s_Data.CubeVertexBufferPtr->Normal[0] = transformedNormal.x;
            s_Data.CubeVertexBufferPtr->Normal[1] = transformedNormal.y;
            s_Data.CubeVertexBufferPtr->Normal[2] = transformedNormal.z;

            s_Data.CubeVertexBufferPtr->EntityID = entityID;
            s_Data.CubeVertexBufferPtr++;
        }

        s_Data.CubeIndexCount += 36;
        s_Data.Stats.CubeCount++;
    }

    void Renderer3D::DrawSkybox(const Ref<TextureCube> &cubemap, const Camera &camera, const glm::mat4 &cameraTransform)
    {
        // 1. 修改深度测试为 <= (因为天空盒深度为 1.0)
        RenderCommand::SetDepthFunc(RendererAPI::DepthComp::Lequal);

        // 2. 关键：关闭面剔除！
        // 因为我们在立方体内部，如果不关闭，我们看到的都是“背面”，会被 GPU 剔除掉
        RenderCommand::SetCullMode(RendererAPI::CullMode::None);

        s_Data.SkyboxShader->Bind();

        // 计算矩阵 (移除位移，只保留旋转)
        glm::mat4 view = glm::mat4(glm::mat3(glm::inverse(cameraTransform)));
        glm::mat4 projection = camera.GetProjection();

        // UBO 上传数据
        Renderer3DData::SkyboxData skyboxData;
        skyboxData.View = view;
        skyboxData.Projection = projection;
        s_Data.SkyboxUniformBuffer->SetData(&skyboxData, sizeof(Renderer3DData::SkyboxData));

        cubemap->Bind(0);

        s_Data.SkyboxVAO->Bind();
        RenderCommand::DrawArrays(s_Data.SkyboxVAO, 36);

        s_Data.SkyboxVAO->Unbind();
        s_Data.SkyboxShader->Unbind();

        // 3. 恢复默认渲染状态
        // 3. 恢复默认渲染状态
        RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
        RenderCommand::SetDepthFunc(RendererAPI::DepthComp::Less);
    }

    // 同样修改 EditorCamera 版本
    void Renderer3D::DrawSkybox(const Ref<TextureCube> &cubemap, const EditorCamera &camera)
    {
        RenderCommand::SetDepthFunc(RendererAPI::DepthComp::Lequal);
        RenderCommand::SetCullMode(RendererAPI::CullMode::None); // 关键！

        s_Data.SkyboxShader->Bind();

        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glm::mat4 projection = camera.GetProjection();

        Renderer3DData::SkyboxData skyboxData;
        skyboxData.View = view;
        skyboxData.Projection = projection;
        s_Data.SkyboxUniformBuffer->SetData(&skyboxData, sizeof(Renderer3DData::SkyboxData));

        cubemap->Bind(0);

        s_Data.SkyboxVAO->Bind();
        RenderCommand::DrawArrays(s_Data.SkyboxVAO, 36);
        s_Data.SkyboxVAO->Unbind();
        s_Data.SkyboxShader->Unbind();

        RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
        RenderCommand::SetDepthFunc(RendererAPI::DepthComp::Less);
    }

    void Renderer3D::DrawPlane(const glm::mat4 &transform, const glm::vec4 &color, int entityID)
    {
        if (s_Data.PlaneIndexCount >= Renderer3DData::MaxIndices)
            NextBatch();

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

        // Plane vertices (Quad on XZ centered at origin?)
        // Let's define standard plane as 1x1 on XZ plane.
        // Similar to Cube Top Face but y=0?
        // Or Top Face of unit cube: y=0.5
        // Let's standard plane: -0.5 to 0.5 on X Z, y=0.
        
        constexpr glm::vec3 planePositions[] = {
            {-0.5f, 0.0f,  0.5f},
            { 0.5f, 0.0f,  0.5f},
            { 0.5f, 0.0f, -0.5f},
            {-0.5f, 0.0f, -0.5f}
        };
        constexpr glm::vec3 planeNormal = {0.0f, 1.0f, 0.0f}; // Up

        for (size_t i = 0; i < 4; i++)
        {
            s_Data.PlaneVertexBufferPtr->Color[0] = color.r;
            s_Data.PlaneVertexBufferPtr->Color[1] = color.g;
            s_Data.PlaneVertexBufferPtr->Color[2] = color.b;
            s_Data.PlaneVertexBufferPtr->Color[3] = color.a;

            glm::vec4 transformedPos = transform * glm::vec4(planePositions[i], 1.0f);
            s_Data.PlaneVertexBufferPtr->Position[0] = transformedPos.x;
            s_Data.PlaneVertexBufferPtr->Position[1] = transformedPos.y;
            s_Data.PlaneVertexBufferPtr->Position[2] = transformedPos.z;

            glm::vec3 transformedNormal = normalMatrix * planeNormal;
            s_Data.PlaneVertexBufferPtr->Normal[0] = transformedNormal.x;
            s_Data.PlaneVertexBufferPtr->Normal[1] = transformedNormal.y;
            s_Data.PlaneVertexBufferPtr->Normal[2] = transformedNormal.z;

            s_Data.PlaneVertexBufferPtr->EntityID = entityID;
            s_Data.PlaneVertexBufferPtr++;
        }

        s_Data.PlaneIndexCount += 6;
        s_Data.Stats.CubeCount++; // Reuse cube count for generic quad/mesh count?
    }

    void Renderer3D::DrawGrid(const EditorCamera &camera)
    {
        s_Data.GridShader->Bind();

        // Pass View/Proj separate
        Renderer3DData::GridData gridData;
        gridData.View = camera.GetViewMatrix();
        gridData.Proj = camera.GetProjection();
        gridData.Near = camera.GetNearClip();
        gridData.Far = camera.GetFarClip();
        s_Data.GridUniformBuffer->SetData(&gridData, sizeof(Renderer3DData::GridData));
        
        // Depth test should be enabled so grid hides behind objects?
        // Yes.
        RenderCommand::SetDepthTest(true);
        RenderCommand::SetDepthMask(false); // Can't write to depth, otherwise it occludes things drawn after (transparents)
        
        s_Data.GridVAO->Bind();
        RenderCommand::DrawArrays(s_Data.GridVAO, 6);
        s_Data.GridVAO->Unbind();
        s_Data.GridShader->Unbind();
        
        RenderCommand::SetDepthMask(true);
    }

    void Renderer3D::DrawGrid(const Camera &camera, const glm::mat4 &transform)
    {
        s_Data.GridShader->Bind();

        Renderer3DData::GridData gridData;
        gridData.View = glm::inverse(transform);
        gridData.Proj = camera.GetProjection();

        // Try to cast to SceneCamera to get clip planes
        const SceneCamera* sceneCamera = dynamic_cast<const SceneCamera*>(&camera);
        if(sceneCamera)
        {
            if(sceneCamera->GetProjectionType() == SceneCamera::ProjectionType::Perspective)
            {
                gridData.Near = sceneCamera->GetPerspectiveNearClip();
                gridData.Far = sceneCamera->GetPerspectiveFarClip();
            }
            else
            {
                gridData.Near = sceneCamera->GetOrthographicNearClip();
                gridData.Far = sceneCamera->GetOrthographicFarClip();
            }
        }
        else
        {
             // Fallback default
             gridData.Near = 0.01f;
             gridData.Far = 1000.0f;
        }

        s_Data.GridUniformBuffer->SetData(&gridData, sizeof(Renderer3DData::GridData));

        RenderCommand::SetDepthTest(true);
        RenderCommand::SetDepthMask(false);

        s_Data.GridVAO->Bind();
        RenderCommand::DrawArrays(s_Data.GridVAO, 6);
        s_Data.GridVAO->Unbind();
        s_Data.GridShader->Unbind();

        RenderCommand::SetDepthMask(true);
    }

    void Renderer3D::ResetStats()
    {
        memset(&s_Data.Stats, 0, sizeof(Statistics));
    }

    Renderer3D::Statistics Renderer3D::GetStatistics()
    {
        return s_Data.Stats;
    }

} // namespace Himii

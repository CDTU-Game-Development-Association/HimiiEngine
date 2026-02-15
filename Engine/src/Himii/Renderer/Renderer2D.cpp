#pragma once
#include "Hepch.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/UniformBuffer.h"
#include "Himii/Renderer/VertexArray.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Himii
{

    struct QuadVertex {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        float TilingFactor;

        int EntityID;
    };

    struct CircleVertex
    {
        glm::vec3 WorldPosition;
        glm::vec3 LocalPosition;
        glm::vec4 Color;
        float Thickness;
        float Fade;

        int EntityID;
    };

    struct LineVertex {
        glm::vec3 Position;
        glm::vec4 Color;

        int EntityID;
    };

    struct Renderer2DData {
        static const uint32_t MaxQuads = 20000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<Shader> QuadShader;
        Ref<Texture2D> WhiteTexture;

        Ref<VertexArray> CircleVertexArray;
        Ref<VertexBuffer> CircleVertexBuffer;
        Ref<Shader> CircleShader;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;
        Ref<Shader> LineShader;

        uint32_t QuadIndexCount = 0;
        QuadVertex *QuadVertexBufferBase = nullptr;
        QuadVertex *QuadVertexBufferPtr = nullptr;

        uint32_t CircleIndexCount = 0;
        CircleVertex *CircleVertexBufferBase = nullptr;
        CircleVertex *CircleVertexBufferPtr = nullptr;

        uint32_t LineVertexCount = 0;
        LineVertex *LineVertexBufferBase = nullptr;
        LineVertex *LineVertexBufferPtr = nullptr;

        float LineWidth = 2.0f;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        glm::vec4 QuadVertexPositions[4];


        Renderer2D::Statistics Stats;

        struct CameraData {
            glm::mat4 ViewProjection;
        };
        CameraData CameraBuffer;
        Ref<UniformBuffer> CameraUniformBuffer;
    };

    static Renderer2DData s_Data;

    // 计算当前批次是否需要换批（由调用者在类方法内触发 NextBatch）
    static inline bool NeedsNewBatch(uint32_t verticesNeeded, uint32_t indicesNeeded)
    {
        const uint32_t usedVertices = (uint32_t)(s_Data.QuadVertexBufferPtr - s_Data.QuadVertexBufferBase);
        return (usedVertices + verticesNeeded > Renderer2DData::MaxVertices) ||
               (s_Data.QuadIndexCount + indicesNeeded > Renderer2DData::MaxIndices);
    }

    void Renderer2D::Init()
    {
        HIMII_PROFILE_FUNCTION();

        //Quad
        s_Data.QuadVertexArray = VertexArray::Create();
        s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));

        s_Data.QuadVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float2, "a_TexCoord"},
                                            {ShaderDataType::Float, "a_TexIndex"},
                                            {ShaderDataType::Float, "a_TilingFactor"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);
        
        s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

        uint32_t *quadIndices = new uint32_t[s_Data.MaxIndices];
        
        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;

            offset += 4;
        }

        Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
        s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
        delete[] quadIndices;

        //Circle
        s_Data.CircleVertexArray = VertexArray::Create();
        s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));

        s_Data.CircleVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_WorldPosition"},
                                              {ShaderDataType::Float3, "a_LocalPosition"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Float, "a_Thickness"},
                                            {ShaderDataType::Float, "a_Fade"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
        s_Data.CircleVertexArray->SetIndexBuffer(quadIB);
        s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

        // Line
        s_Data.LineVertexArray = VertexArray::Create();
        s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));

        s_Data.LineVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                            {ShaderDataType::Float4, "a_Color"},
                                            {ShaderDataType::Int, "a_EntityID"}});
        s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
        s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];


        s_Data.WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int32_t samplers[s_Data.MaxTextureSlots];
        for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
            samplers[i] = i;

        //  锟斤拷锟斤拷锟斤拷色锟斤拷锟斤拷锟斤拷
        s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
        s_Data.CircleShader = Shader::Create("assets/shaders/Renderer2D_Circle.glsl");
        s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

        s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
    }
    void Renderer2D::Shutdown()
    {
        HIMII_PROFILE_FUNCTION();


        delete[] s_Data.QuadVertexBufferBase;
    }
    void Renderer2D::BeginScene(const OrthographicCamera &camera)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.QuadShader->Bind();
        s_Data.QuadShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
        s_Data.QuadShader->SetMat4("u_Transform", glm::mat4(1.0f));

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.TextureSlotIndex = 1;
    }

    void Renderer2D::BeginScene(const EditorCamera& camera)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
    {
        HIMII_PROFILE_FUNCTION();

        s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
        s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
        s_Data.CameraUniformBuffer->Bind();

        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        HIMII_PROFILE_FUNCTION();

        uint32_t dataSize = (uint8_t *)s_Data.QuadVertexBufferPtr - (uint8_t *)s_Data.QuadVertexBufferBase;
        s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

        Flush();
    }

    void Renderer2D::StartBatch()
    {

        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

        s_Data.CircleIndexCount = 0;
        s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

        s_Data.LineVertexCount = 0;
        s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

        s_Data.TextureSlotIndex = 1; // 0 reserved for white texture
    }

    void Renderer2D::Flush()
    {
        if (s_Data.QuadIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.QuadVertexBufferPtr - (uint8_t *)s_Data.QuadVertexBufferBase);
            s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

            // Bind textures
            for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
                s_Data.TextureSlots[i]->Bind(i);

            s_Data.QuadShader->Bind();
            RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.CircleIndexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.CircleVertexBufferPtr - (uint8_t *)s_Data.CircleVertexBufferBase);
            s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

            s_Data.CircleShader->Bind();
            RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
            s_Data.Stats.DrawCalls++;
        }

        if (s_Data.LineVertexCount)
        {
            uint32_t dataSize =
                    (uint32_t)((uint8_t *)s_Data.LineVertexBufferPtr - (uint8_t *)s_Data.LineVertexBufferBase);
            s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

            s_Data.LineShader->Bind();
            RenderCommand::SetLineWidth(s_Data.LineWidth);
            RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
            s_Data.Stats.DrawCalls++;
        }

    }

    // Some paths request a new batch when buffers/textures reach capacity
    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    //-----------------------------------锟斤拷锟斤拷锟侥憋拷锟斤拷----------------------------//
    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color);
    }
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, color);
    }
    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                              float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, texture, tilingFactor, tintColor);
    }
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                              float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 color, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        constexpr size_t quadVertexCount = 4;
        const float textureIndex = 0.0f; // White Texture
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        const float tilingFactor = 1.0f;

        if (NeedsNewBatch((uint32_t)quadVertexCount, 6))
            NextBatch();

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = entityID;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const Ref<Texture2D> &texture, float tilingFactor,
                              const glm::vec4 &tintColor, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        if (NeedsNewBatch((uint32_t)quadVertexCount, 6))
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        for (size_t i = 0; i < quadVertexCount; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
            s_Data.QuadVertexBufferPtr->Color = tintColor;
            s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr->EntityID = entityID;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    // Atlas-drawn quads with custom UVs
    void Renderer2D::DrawQuadUV(const glm::vec2 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuadUV(glm::vec3(position, 0.0f), size, texture, uvs, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuadUV(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();
        if (NeedsNewBatch(4, 6))
            NextBatch();

        constexpr glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float textureIndex = 0.0f;

        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            ++s_Data.TextureSlotIndex;
        }

        glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[0];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[1];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[2];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[3];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = -1;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    // Transform-based UV variant
    void Renderer2D::DrawQuadUV(const glm::mat4 &transform, const Ref<Texture2D> &texture,
                                const std::array<glm::vec2, 4> &uvs, float tilingFactor, const glm::vec4 &tintColor,
                                int entityID)
    {
        HIMII_PROFILE_FUNCTION();
        if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            const auto &slotTex = s_Data.TextureSlots[i];
            if (slotTex && texture && *slotTex == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();
            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            ++s_Data.TextureSlotIndex;
        }

        constexpr size_t quadVertexCount = 4;
        const glm::vec4 color = tintColor;

        // Emit the 4 vertices
        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[0];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[0];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[1];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[1];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[2];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[2];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[3];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = uvs[3];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr->EntityID = entityID;
        s_Data.QuadVertexBufferPtr++;

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    //---------------------------------锟斤拷锟斤拷锟斤拷转锟侥憋拷锟斤拷------------------------------//
    void Renderer2D::DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size, float rotation,
                                     const glm::vec4 &color)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color);
    }
    void Renderer2D::DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size, float rotation,
                                     const glm::vec4 &color)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2 &position, const glm::vec2 &size, float rotation,
                                     const Ref<Texture2D> &texture, float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3 &position, const glm::vec2 &size, float rotation,
                                     const Ref<Texture2D> &texture, float tilingFactor, const glm::vec4 &tintColor)
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                              glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                              glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

        DrawQuad(transform, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4 &color, float thickness , float fade , int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        for (size_t i = 0; i < 4; i++)
        {
            s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
            s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i]*2.0f;
            s_Data.CircleVertexBufferPtr->Color = color;
            s_Data.CircleVertexBufferPtr->Thickness = thickness;
            s_Data.CircleVertexBufferPtr->Fade = fade;
            s_Data.CircleVertexBufferPtr->EntityID = entityID;
            s_Data.CircleVertexBufferPtr++;
        }

        s_Data.CircleIndexCount += 6;

        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawLine(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec4 &color, int entityID)
    {
        s_Data.LineVertexBufferPtr->Position = p0;
        s_Data.LineVertexBufferPtr->Color = color;
        s_Data.LineVertexBufferPtr->EntityID = entityID;
        s_Data.LineVertexBufferPtr++;

        s_Data.LineVertexBufferPtr->Position = p1;
        s_Data.LineVertexBufferPtr->Color = color;
        s_Data.LineVertexBufferPtr->EntityID = entityID;
        s_Data.LineVertexBufferPtr++;

        s_Data.LineVertexCount += 2;
    }

    void Renderer2D::DrawRect(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, int entityID)
    {
        glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
        glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
        glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
        glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

        DrawLine(p0, p1, color, entityID);
        DrawLine(p1, p2, color, entityID);
        DrawLine(p2, p3, color, entityID);
        DrawLine(p3, p0, color, entityID);
    }

    void Renderer2D::DrawRect(const glm::mat4 &transform, const glm::vec4 &color, int entityID)
    {
        glm::vec3 lineVertices[4];
        for (size_t i = 0; i < 4; i++)
            lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

        DrawLine(lineVertices[0], lineVertices[1], color, entityID);
        DrawLine(lineVertices[1], lineVertices[2], color, entityID);
        DrawLine(lineVertices[2], lineVertices[3], color, entityID);
        DrawLine(lineVertices[3], lineVertices[0], color, entityID);
    }

    void Renderer2D::DrawTilemap(const glm::mat4 &transform, const Ref<TileMapData>& mapData, const Ref<TileSet>& tileSet, int entityID)
    {
        HIMII_PROFILE_FUNCTION();

        if (!mapData) return;

        uint32_t width = mapData->GetWidth();
        uint32_t height = mapData->GetHeight();
        float cellSize = mapData->GetCellSize();
        const auto &tiles = mapData->GetTiles();

        if (tiles.size() < (size_t)(width * height))
            return;

        // 预缓存 Atlas 纹理槽位：为每个 AtlasSource 分配纹理槽
        // key: atlasSourceIndex, value: textureIndex in batch
        std::unordered_map<uint32_t, float> atlasTextureIndices;
        if (tileSet)
        {
            const auto &sources = tileSet->GetAtlasSources();
            for (uint32_t si = 0; si < sources.size(); ++si)
            {
                Ref<Texture2D> tex = sources[si].CachedTexture;
                if (!tex) continue;

                float texIdx = 0.0f;
                for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
                {
                    if (s_Data.TextureSlots[i] && *s_Data.TextureSlots[i] == *tex)
                    {
                        texIdx = (float)i;
                        break;
                    }
                }
                if (texIdx == 0.0f)
                {
                    if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                        NextBatch();
                    texIdx = (float)s_Data.TextureSlotIndex;
                    s_Data.TextureSlots[s_Data.TextureSlotIndex] = tex;
                    s_Data.TextureSlotIndex++;
                }
                atlasTextureIndices[si] = texIdx;
            }
        }

        // 遍历所有 Tile
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                uint16_t tileID = tiles[x + y * width];
                if (tileID == 0) continue;

                if (NeedsNewBatch(4, 6)) NextBatch();

                // 查找 TileDef 以确定纹理和 UV
                float textureIndex = 0.0f; // 默认：白色纹理
                glm::vec2 texCoords[4] = {
                    {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
                };
                glm::vec4 tint{1, 1, 1, 1};

                if (tileSet)
                {
                    const TileDef *def = tileSet->GetTileDef(tileID);
                    if (def)
                    {
                        tint = def->Tint;

                        if (def->SourceType == TileSourceType::Atlas)
                        {
                            auto it = atlasTextureIndices.find(def->AtlasSourceIndex);
                            if (it != atlasTextureIndices.end())
                            {
                                textureIndex = it->second;

                                const auto &sources = tileSet->GetAtlasSources();
                                if (def->AtlasSourceIndex < sources.size())
                                {
                                    const auto &src = sources[def->AtlasSourceIndex];
                                    if (src.CachedTexture)
                                    {
                                        float texW = (float)src.CachedTexture->GetWidth();
                                        float texH = (float)src.CachedTexture->GetHeight();
                                        float ts = (float)src.TileSize;
                                        if (ts <= 0) ts = 16.0f;

                                        int cols = (int)(texW / ts);
                                        if (cols <= 0) cols = 1;
                                        float uSize = ts / texW;
                                        float vSize = ts / texH;

                                        int col = def->AtlasCoords.x;
                                        int row = def->AtlasCoords.y;

                                        float u0 = col * uSize;
                                        float u1 = (col + 1) * uSize;
                                        float v0 = row * vSize;
                                        float v1 = (row + 1) * vSize;

                                        texCoords[0] = {u0, v0};
                                        texCoords[1] = {u1, v0};
                                        texCoords[2] = {u1, v1};
                                        texCoords[3] = {u0, v1};
                                    }
                                }
                            }
                        }
                        else if (def->SourceType == TileSourceType::Individual)
                        {
                            // 独立纹理模式
                            Ref<Texture2D> indivTex = def->CachedIndividualTexture;
                            if (indivTex)
                            {
                                float texIdx = 0.0f;
                                for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
                                {
                                    if (s_Data.TextureSlots[i] && *s_Data.TextureSlots[i] == *indivTex)
                                    {
                                        texIdx = (float)i;
                                        break;
                                    }
                                }
                                if (texIdx == 0.0f)
                                {
                                    if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                                        NextBatch();
                                    texIdx = (float)s_Data.TextureSlotIndex;
                                    s_Data.TextureSlots[s_Data.TextureSlotIndex] = indivTex;
                                    s_Data.TextureSlotIndex++;
                                }
                                textureIndex = texIdx;
                                // 独立纹理使用全 UV
                            }
                        }
                    }
                }
                // 无 TileSet 或无 TileDef：使用白色纹理 (textureIndex=0, 全 UV)

                // 计算每个 Tile 的四个顶点位置
                float px = (float)x * cellSize;
                float py = (float)y * cellSize;
                glm::vec4 localPos[4] = {
                    {px,            py,            0.0f, 1.0f},
                    {px + cellSize, py,            0.0f, 1.0f},
                    {px + cellSize, py + cellSize, 0.0f, 1.0f},
                    {px,            py + cellSize, 0.0f, 1.0f}
                };

                for (int i = 0; i < 4; i++)
                {
                    s_Data.QuadVertexBufferPtr->Position = transform * localPos[i];
                    s_Data.QuadVertexBufferPtr->Color = tint;
                    s_Data.QuadVertexBufferPtr->TexCoord = texCoords[i];
                    s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
                    s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
                    s_Data.QuadVertexBufferPtr->EntityID = entityID;
                    s_Data.QuadVertexBufferPtr++;
                }
                s_Data.QuadIndexCount += 6;
                s_Data.Stats.QuadCount++;
            }
        }
    }

    void Renderer2D::DrawSprite(const glm::mat4 &transform, SpriteRendererComponent &sprite, int entityID)
    {
        if (sprite.Texture)
            DrawQuad(transform, sprite.Texture, sprite.TilingFactor, sprite.Color, entityID);
        else
            DrawQuad(transform, sprite.Color, entityID);
    }

    float Renderer2D::GetLineWidth()
    {
        return s_Data.LineWidth;
    }

    void Renderer2D::SetLineWidth(float width)
    {
        s_Data.LineWidth = width;
    }

    void Renderer2D::ResetStats()
    {
        memset(&s_Data.Stats, 0, sizeof(Statistics));
    }
    Renderer2D::Statistics Renderer2D::GetStatistics()
    {
        return s_Data.Stats;
    }
} // namespace Himii

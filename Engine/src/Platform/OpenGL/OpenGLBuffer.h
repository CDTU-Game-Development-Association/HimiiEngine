#pragma once
#include "Himii/Renderer/Buffer.h"

namespace Himii
{
    class OpenGLVertexBuffer : public VertexBuffer {
    public:
        OpenGLVertexBuffer(float *vertices, uint32_t size);
        virtual ~OpenGLVertexBuffer();

        virtual void Bind() const;
        virtual void Unbind() const;

        virtual const BufferLayout &GetLayout() const override
        {
            return m_Layout;
        }
        virtual void SetLayout(const BufferLayout &layout) override
        {
            m_Layout = layout;
        }

    private:
        uint32_t m_RendererID;
        BufferLayout m_Layout;
    };

    class OpenGLIndexBuffer : public IndexBuffer {
    public:
        OpenGLIndexBuffer(uint32_t *indices, uint32_t size);
        virtual ~OpenGLIndexBuffer();

        virtual void Bind() const;
        virtual void Unbind() const;
        virtual uint32_t GetCount() const
        {
            return m_Count;
        }

    private:
        uint32_t m_RendererID;
        uint32_t m_Count;
    };
} // namespace Himii

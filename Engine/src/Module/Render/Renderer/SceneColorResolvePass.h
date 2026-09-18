#pragma once

#include "EngineCore/Core/Core.h"

namespace Himii
{
    class Framebuffer;

    /// 全屏 HDR → LDR。未光照 2D 走显示空间直出；3D / 将来的 2D 光照走电影曲线。
    enum class SceneColorResolveEncoding
    {
        DisplayReferred = 0,
        SceneReferredFilmic = 1
    };

    class SceneColorResolvePass
    {
    public:
        static void Init();
        static void Shutdown();

        /// 2D 工程或正交相机：显示空间直出。透视 3D：ACES + sRGB。
        static SceneColorResolveEncoding SelectEncoding(bool projectIsTwoDimensional,
                                                        bool cameraIsOrthographic);

        /// 采样 sourceFramebuffer 的 color0（RGBA16F），写入当前已绑定的 LDR 目标。
        static void Resolve(const Ref<Framebuffer> &sourceFramebuffer, float exposure,
                            SceneColorResolveEncoding encoding =
                                    SceneColorResolveEncoding::SceneReferredFilmic);

        /// 无主相机时使用的默认曝光。
        static constexpr float DefaultExposure = 1.0f;
        static float ClampExposure(float exposure);
    };
}

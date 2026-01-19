#type vertex
#version 450 core

// 顶点数据：通常是一个全屏四边形 (XY: -1 到 1)
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// 输出到 Fragment Shader
layout(location = 0) out vec3 v_NearPoint;
layout(location = 1) out vec3 v_FarPoint;

// Grid 专用 Uniform Block
layout(std140, binding = 2) uniform GridUniforms
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Near;
    float u_Far;
};

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(proj);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    vec3 p = a_Position;
    
    // Unproject grid points at Near and Far planes
    // OpenGL NDC Z range is [-1, 1]
    v_NearPoint = UnprojectPoint(p.x, p.y, -1.0, u_View, u_Proj).xyz; 
    v_FarPoint  = UnprojectPoint(p.x, p.y,  1.0, u_View, u_Proj).xyz;
    
    // 直接绘制到 Clip Space (全屏)
    gl_Position = vec4(p, 1.0); 
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_NearPoint;
layout(location = 1) in vec3 v_FarPoint;

layout(std140, binding = 2) uniform GridUniforms
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Near;
    float u_Far;
};

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = u_Proj * u_View * vec4(pos.xyz, 1.0);
    // clip_space_pos.z / clip_space_pos.w is NDC Z [-1, 1]
    // gl_FragDepth expects window space Z [0, 1]
    return (clip_space_pos.z / clip_space_pos.w) * 0.5 + 0.5;
}

// 原始网格线计算逻辑
float grid(vec3 fragPos3D, float scale, bool drawAxis) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    
    float alpha = 1.0 - min(line, 1.0);
    
    // Axis colors
    if(drawAxis) {
        if(abs(fragPos3D.x) < (minimumx / scale))
            alpha = 1.0;
        if(abs(fragPos3D.z) < (minimumz / scale))
            alpha = 1.0;
    }
    return alpha;
}

float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = u_Proj * u_View * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; 
    float linearDepth = (2.0 * u_Near * u_Far) / (u_Far + u_Near - clip_space_depth * (u_Far - u_Near)); 
    return linearDepth / u_Far; 
}

void main() {
    float t = -v_NearPoint.y / (v_FarPoint.y - v_NearPoint.y);
    if(t < 0.0) discard;
    
    vec3 fragPos3D = v_NearPoint + t * (v_FarPoint - v_NearPoint);
    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    // 淡出远处
     float fading = max(0.0, (0.5 - linearDepth));
    
    // 多级网格
    // Level 0: potentially noisy small grid -> Update to 1.0 (Cube size)
    float g1 = grid(fragPos3D, 1.0, true); 
    // Level 1: larger grid -> Update to 0.1 (10 units)
    float g2 = grid(fragPos3D, 0.1, true);
    
    // 基于视距混合
    // 当 linearDepth 很小时（近处），显示 g1 + g2
    // 当 linearDepth 变大时（远处），淡出 g1，只留 g2
    float lod0_fade = 1.0 - smoothstep(0.0, 0.2, linearDepth * 5.0); // 快速淡出小格子
    
    // 组合颜色
    // base grid
    vec4 color = vec4(0.2, 0.2, 0.2, 0.0);
    
    // 大格子
    float alpha2 = g2;
    // 小格子 (fade out)
    float alpha1 = g1 * lod0_fade;
    
    // 简单叠加：max(alpha1, alpha2) 或者混合
    float alpha = max(alpha1, alpha2);
    
    // 轴线颜色增强 (Hack to ensure axis stays visible/colored)
    // Redo axis check simply
    color.a = alpha;
    
    if(color.a <= 0.0) discard;

    // Axis coloring again for final pixel
    float scale = 1.0; 
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    
    if(abs(fragPos3D.x) < (minimumx / scale))
        color.b = 1.0;
    if(abs(fragPos3D.z) < (minimumz / scale))
        color.r = 1.0;
    
    color.a *= fading;
    
    o_Color = color;
}
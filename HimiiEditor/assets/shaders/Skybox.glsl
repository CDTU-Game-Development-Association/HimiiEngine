#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

// 1. 给输出变量指定 location
layout(location = 0) out vec3 v_TexCoords;

// 2. 将 View 和 Projection 放入 Uniform Block (UBO)
// binding = 1 (避免与 binding 0 的通用 Camera 冲突)
layout(std140, binding = 1) uniform SkyboxUniforms
{
	mat4 u_View;
	mat4 u_Projection;
};

void main()
{
    v_TexCoords = a_Position;
    
    // 注意：这里直接使用 block 里的变量
    vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
    
    // 深度优化技巧：z = w，透视除法后深度为 1.0
    gl_Position = pos.xyww;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

// 3. 给输入变量指定 location (必须与 vertex shader 匹配)
layout(location = 0) in vec3 v_TexCoords;

// 4. 给采样器指定 binding
layout(binding = 0) uniform samplerCube u_Skybox;

void main()
{    
    o_Color = texture(u_Skybox, v_TexCoords);
}
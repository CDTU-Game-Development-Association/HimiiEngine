#type vertex
#version 450 core
layout(location = 0) in vec4 a_Color;
layout(location = 1) in vec3 a_Position;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

// 定义输出结构体
struct VertexOutput
{
	vec4 Color;
    vec3 Normal;
};

layout (location = 0) out VertexOutput v_Output; // 实例名改为 v_Output
layout (location = 3) out flat int v_EntityID;

void main()
{
	v_Output.Color = a_Color;
    v_Output.Normal = a_Normal;
	v_EntityID = a_EntityID;
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
    vec3 Normal;
};

// 这里的输入实例名必须也是 v_Output，与顶点着色器保持一致！
layout (location = 0) in VertexOutput v_Output; 
layout (location = 3) in flat int v_EntityID;

void main()
{
    // Basic Light
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    vec3 normal = normalize(v_Output.Normal); // 使用 v_Output

    // Diffuse
    float diff = max(dot(normal, -lightDir), 0.0);

    // Ambient
    float ambient = 0.3;

    vec3 lighting = (ambient + diff) * vec3(1.0);

	o_Color = vec4(lighting, 1.0) * v_Output.Color; // 使用 v_Output
	o_EntityID = v_EntityID;
}
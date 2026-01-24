#type vertex
#version 450 core
// Per-Vertex Attributes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// Per-Instance Attributes
layout(location = 3) in vec4 a_Color;
layout(location = 4) in vec4 a_CustomData; // .x = TexIndex, .y = EntityID
layout(location = 5) in mat4 a_Transform; // Occupies 5, 6, 7, 8

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
    vec3 Normal;
    vec2 TexCoord;
};

layout (location = 0) out VertexOutput v_Output;
layout (location = 3) out flat float v_TexIndex;
layout (location = 4) out flat int v_EntityID;

void main()
{
	v_Output.Color = a_Color;
    v_Output.TexCoord = a_TexCoord;
    v_TexIndex = a_CustomData.x;
	v_EntityID = int(a_CustomData.y);
    
    // Transform Position
    vec4 worldPosition = a_Transform * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldPosition;
    
    // Transform Normal
    mat3 normalMatrix = transpose(inverse(mat3(a_Transform)));
    v_Output.Normal = normalize(normalMatrix * a_Normal);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
    vec3 Normal;
    vec2 TexCoord;
};

layout (location = 0) in VertexOutput v_Output;
layout (location = 3) in flat float v_TexIndex;
layout (location = 4) in flat int v_EntityID;

layout(binding = 0) uniform sampler2D u_Textures[32];

void main()
{
    vec3 normal = normalize(v_Output.Normal);



    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));

    float diff = max(dot(normal, -lightDir), 0.0);

    float ambient = 0.15;

    float specular = 0.0;
    if(diff > 0.0) {
        vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0));
        vec3 halfwayDir = normalize(-lightDir + viewDir);
        specular = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * 0.5;
    }

    vec3 lighting = (ambient + diff + specular) * vec3(1.0);

    vec4 texColor = v_Output.Color;
    
    if (v_TexIndex > 0.0)
    {
        int index = int(v_TexIndex);
        texColor *= texture(u_Textures[index], v_Output.TexCoord);
    }

	o_Color = vec4(lighting, 1.0) * texColor;
	o_EntityID = v_EntityID;
}
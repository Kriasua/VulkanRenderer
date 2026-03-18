#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out float outTime;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragPos;



layout(binding = 0) uniform UniformBufferObject
{
	mat4 view;
	mat4 proj;
	vec4 lightDir;
    vec4 lightColor;
	mat4 lightMat;
	float intime;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
} entity;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 worldPos = entity.model * vec4(inPosition, 1.0);
	gl_Position = ubo.proj * ubo.view * worldPos;
	fragColor = inColor;
	outTime = ubo.intime;
	fragTexCoord = inTexCoord;

	// 法线也需要跟着模型旋转，否则模型转了，反光面却不转
    mat3 normalMatrix = transpose(inverse(mat3(entity.model)));
    fragNormal = normalMatrix * inNormal;
    
    // 把世界坐标传给片段着色器算光照
    fragPos = worldPos.xyz;
	
}
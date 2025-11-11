#version 450

#define MAX_LIGHTS 10

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

struct PointLight {
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 invViewMatrix;
	vec4 ambientLightColor;
	PointLight pointLights[MAX_LIGHTS];
	int numLights;
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main() {
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionWorld);

	fragPosWorld = positionWorld.xyz;
	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragColor = color;
}
#pragma once
#include <app_framework/common.h>
#include <app_framework/registry.h>
#include <app_framework/render/material.h>
#include <app_framework/shader/solid_color_vs_program.h>

namespace ml {
namespace app_framework {

static const char *kMarkerOverlayVertexShader = R"GLSL(
#version 410 core

layout(std140) uniform Camera {
  mat4 view_proj;
  vec4 world_position;
} camera;

layout(std140) uniform Model {
  mat4 transform;
} model;

layout (location = 0) in vec3 position;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
  gl_Position = camera.view_proj * model.transform * vec4(position, 1.0);
}
)GLSL";

static const char *kMarkerOverlayFragmentShader = R"GLSL(
#version 410 core
layout(std140) uniform Material {
  vec3 MarkerColor;
  float MarkerAlpha;
} material;

layout (location = 0) out vec4 out_color;

void main() {
  out_color = vec4(material.MarkerColor, material.MarkerAlpha);
}
)GLSL";

class MarkerOverlayMaterial final : public Material {
public:
  MarkerOverlayMaterial(glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f),
                        float alpha = 0.9f) {
    SetVertexProgram(
        Registry::GetInstance()
            ->GetResourcePool()
            ->LoadShaderFromCode<VertexProgram>(kMarkerOverlayVertexShader));
    SetFragmentProgram(Registry::GetInstance()
                           ->GetResourcePool()
                           ->LoadShaderFromCode<FragmentProgram>(
                               kMarkerOverlayFragmentShader));
    SetMarkerColor(color);
    SetMarkerAlpha(alpha);
    EnableAlphaBlending(true);
  }
  ~MarkerOverlayMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(glm::vec3, MarkerColor);
  MATERIAL_VARIABLE_DECLARE(float, MarkerAlpha);
};

} // namespace app_framework
} // namespace ml

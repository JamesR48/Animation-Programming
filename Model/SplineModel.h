/* generate spline plus */
#if !defined SPLINE_MODEL_HPP
#define SPLINE_MODEL_HPP

#include <vector>
#include <glm/glm.hpp>

#include "../Render/OpenGL/OGLRenderData.h"

class SplineModel {
  public:
  OGLMesh CreateVertexData(int NumSplinePoints, const glm::vec3& StartVertex, const glm::vec3& StartTangent, const glm::vec3& EndVertex, const glm::vec3& EndTangent);
  glm::vec3 Bezier(const glm::vec3& StartVertex, const glm::vec3& StartTangent, const glm::vec3& EndVertex, const glm::vec3& EndTangent, float t);
};

#endif
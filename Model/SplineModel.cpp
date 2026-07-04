#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/spline.hpp>

#include "SplineModel.h"
#include "../Tools/Logger.h"

OGLMesh SplineModel::CreateVertexData(int NumSplinePoints, const glm::vec3& StartVertex, const glm::vec3& StartTangent, const glm::vec3& EndVertex, const glm::vec3& EndTangent)
{
  OGLMesh mVertexData;

  const int TotalVertices = (NumSplinePoints * 2) + 4;
  mVertexData.Vertices.resize(TotalVertices);

  /* draw the tangents as lines */
  mVertexData.Vertices[0].Color = glm::vec3(1.0f, 0.0f, 0.0f);
  mVertexData.Vertices[0].Position = StartVertex;
  mVertexData.Vertices[1].Color = glm::vec3(0.5f, 0.0, 0.0f);
  mVertexData.Vertices[1].Position = StartVertex + StartTangent;

  mVertexData.Vertices[2].Color = glm::vec3(0.0f, 0.0, 1.0f);
  mVertexData.Vertices[2].Position = EndVertex;
  mVertexData.Vertices[3].Color = glm::vec3(0.0f, 0.0f,0.5f);
  mVertexData.Vertices[3].Position = EndVertex + EndTangent;

  /* draw tangent as line segments */
  float Offset = 1.0f / static_cast<float>(NumSplinePoints);
  float Value = 0.0f;

  for (int i = 5; i < TotalVertices; i += 2)
  {
    mVertexData.Vertices[i - 1].Position = glm::hermite(StartVertex, StartTangent, EndVertex,EndTangent, Value);
    mVertexData.Vertices[i - 1].Color = glm::vec3(Value);

    /* keep Color of line segment */
    mVertexData.Vertices[i].Color = glm::vec3(Value);

    Value += Offset;
    mVertexData.Vertices[i].Position = glm::hermite(StartVertex, StartTangent, EndVertex,EndTangent, Value);
  }
  mVertexData.Vertices[TotalVertices - 1].Position = EndVertex;
  mVertexData.Vertices[TotalVertices - 1].Color = glm::vec3(Value);

  return mVertexData;
}

glm::vec3 SplineModel::Bezier(const glm::vec3 &StartVertex, const glm::vec3 &StartTangent, const glm::vec3 &EndVertex, const glm::vec3 &EndTangent, float t)
{
    const float CubicDerivativeScale = 1.0f/3.0f;
    // Hermite tangents to Bézier handles
    glm::vec3 P0 = StartVertex;
    glm::vec3 P1 = StartVertex + (StartTangent * CubicDerivativeScale);
    glm::vec3 P2 = EndVertex - (EndTangent * CubicDerivativeScale);
    glm::vec3 P3 = EndVertex;

    //  De Casteljau division 1
    // interp between control points
    glm::vec3 R0 = glm::mix(P0, P1, t);
    glm::vec3 R1 = glm::mix(P1, P2, t);
    glm::vec3 R2 = glm::mix(P2, P3, t);

    //  De Casteljau division 2
    // interp between the found intersections
    glm::vec3 S0 = glm::mix(R0, R1, t);
    glm::vec3 S1 = glm::mix(R1, R2, t);

    //  De Casteljau division 3
    // exact coordinate on the curve.
    glm::vec3 FinalCurvePoint = glm::mix(S0, S1, t);

    return FinalCurvePoint;
}


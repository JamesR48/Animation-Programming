#include "ArrowModel.h"
#include <vector>
#include <glm/glm.hpp>
#include "../Tools/Logger.h"

OGLMesh ArrowModel::GetVertexData() {
  if (mVertexData.Vertices.size() == 0)
  {
    Init();
  }
  return mVertexData;
}

void ArrowModel::Init() {
  mVertexData.Vertices.resize(6);

  /* simple arrow, pointing to X axis */
  mVertexData.Vertices[0].Position = glm::vec3(0.0f, 0.0f,  0.0f);
  mVertexData.Vertices[1].Position = glm::vec3(1.0f, 0.0f,  0.0f);
  mVertexData.Vertices[2].Position = glm::vec3(1.0f, 0.0f,  0.0f);
  mVertexData.Vertices[3].Position = glm::vec3(0.8f, 0.0f,  0.075f);
  mVertexData.Vertices[4].Position = glm::vec3(1.0f, 0.0f,  0.0f);
  mVertexData.Vertices[5].Position = glm::vec3(0.8f, 0.0f, -0.075f);

  mVertexData.Vertices[0].Color = glm::vec3(0.8f, 0.0f, 0.0f);
  mVertexData.Vertices[1].Color = glm::vec3(0.8f, 0.0f, 0.0f);
  mVertexData.Vertices[2].Color = glm::vec3(0.8f, 0.0f, 0.0f);
  mVertexData.Vertices[3].Color = glm::vec3(0.8f, 0.0f, 0.0f);
  mVertexData.Vertices[4].Color = glm::vec3(0.8f, 0.0f, 0.0f);
  mVertexData.Vertices[5].Color = glm::vec3(0.8f, 0.0f, 0.0f);

  Logger::Log(1, "%s: ArrowModel - loaded %d Vertices\n", __FUNCTION__, mVertexData.Vertices.size());
}

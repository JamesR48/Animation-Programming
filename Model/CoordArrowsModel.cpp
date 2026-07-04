#include "CoordArrowsModel.h"
#include "../Tools/Logger.h"

OGLMesh CoordArrowsModel::GetVertexData()
{
  if (mVertexData.Vertices.size() == 0)
  {
    Init();
  }
  return mVertexData;
}

void CoordArrowsModel::Init()
{
  mVertexData.Vertices.resize(18);

  /*  X axis - red */
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

  /*  Y axis - green */
  mVertexData.Vertices[6].Position = glm::vec3(0.0f, 0.0f,  0.0f);
  mVertexData.Vertices[7].Position = glm::vec3(0.0f, 1.0f,  0.0f);
  mVertexData.Vertices[8].Position = glm::vec3(0.0f, 1.0f,  0.0f);
  mVertexData.Vertices[9].Position = glm::vec3(0.0f, 0.8f,  0.075f);
  mVertexData.Vertices[10].Position = glm::vec3(0.0f, 1.0f,  0.0f);
  mVertexData.Vertices[11].Position = glm::vec3(0.0f, 0.8f, -0.075f);

  mVertexData.Vertices[6].Color = glm::vec3(0.0f, 0.8f, 0.0f);
  mVertexData.Vertices[7].Color = glm::vec3(0.0f, 0.8f, 0.0f);
  mVertexData.Vertices[8].Color = glm::vec3(0.0f, 0.8f, 0.0f);
  mVertexData.Vertices[9].Color = glm::vec3(0.0f, 0.8f, 0.0f);
  mVertexData.Vertices[10].Color = glm::vec3(0.0f, 0.8f, 0.0f);
  mVertexData.Vertices[11].Color = glm::vec3(0.0f, 0.8f, 0.0f);

  /*  Z axis - blue */
  mVertexData.Vertices[12].Position = glm::vec3( 0.0f,   0.0f, 0.0f);
  mVertexData.Vertices[13].Position = glm::vec3( 0.0f,   0.0f, 1.0f);
  mVertexData.Vertices[14].Position = glm::vec3( 0.0f,   0.0f, 1.0f);
  mVertexData.Vertices[15].Position = glm::vec3( 0.075f, 0.0f, 0.8f);
  mVertexData.Vertices[16].Position = glm::vec3( 0.0f,   0.0f, 1.0f);
  mVertexData.Vertices[17].Position = glm::vec3(-0.075f, 0.0f, 0.8f);

  mVertexData.Vertices[12].Color = glm::vec3(0.0f, 0.0f, 0.8f);
  mVertexData.Vertices[13].Color = glm::vec3(0.0f, 0.0f, 0.8f);
  mVertexData.Vertices[14].Color = glm::vec3(0.0f, 0.0f, 0.8f);
  mVertexData.Vertices[15].Color = glm::vec3(0.0f, 0.0f, 0.8f);
  mVertexData.Vertices[16].Color = glm::vec3(0.0f, 0.0f, 0.8f);
  mVertexData.Vertices[17].Color = glm::vec3(0.0f, 0.0f, 0.8f);

  Logger::Log(1, "%s: CoordArrowsModel - loaded %d Vertices\n", __FUNCTION__, mVertexData.Vertices.size());
}


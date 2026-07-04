#include "Model_OpenGL.h"
#include "../Tools/Logger.h"

void Model_OpenGL::Init()
{
    mVertexData.Vertices.resize(36);

  /* front */
  mVertexData.Vertices[0].Position = glm::vec3(-0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[1].Position = glm::vec3( 0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[2].Position = glm::vec3(-0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[3].Position = glm::vec3(-0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[4].Position = glm::vec3( 0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[5].Position = glm::vec3( 0.5f,  0.5f,  0.5f);

  mVertexData.Vertices[0].Color = glm::vec3(1.0f, 0.5f, 0.5f);
  mVertexData.Vertices[1].Color = glm::vec3(1.0f, 0.5f, 0.5f);
  mVertexData.Vertices[2].Color = glm::vec3(1.0f, 0.5f, 0.5f);
  mVertexData.Vertices[3].Color = glm::vec3(1.0f, 0.5f, 0.5f);
  mVertexData.Vertices[4].Color = glm::vec3(1.0f, 0.5f, 0.5f);
  mVertexData.Vertices[5].Color = glm::vec3(1.0f, 0.5f, 0.5f);

  mVertexData.Vertices[0].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[1].UV = glm::vec2(1.0, 1.0);
  mVertexData.Vertices[2].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[3].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[4].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[5].UV = glm::vec2(1.0, 1.0);

  /* back */
  mVertexData.Vertices[6].Position = glm::vec3(-0.5f, -0.5f,  -0.5f);
  mVertexData.Vertices[7].Position = glm::vec3(-0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[8].Position = glm::vec3( 0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[9].Position = glm::vec3(-0.5f, -0.5f,  -0.5f);
  mVertexData.Vertices[10].Position = glm::vec3( 0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[11].Position = glm::vec3( 0.5f, -0.5f,  -0.5f);

  mVertexData.Vertices[6].Color = glm::vec3(0.5f, 1.0f, 0.5f);
  mVertexData.Vertices[7].Color = glm::vec3(0.5f, 1.0f, 0.5f);
  mVertexData.Vertices[8].Color = glm::vec3(0.5f, 1.0f, 0.5f);
  mVertexData.Vertices[9].Color = glm::vec3(0.5f, 1.0f, 0.5f);
  mVertexData.Vertices[10].Color = glm::vec3(0.5f, 1.0f, 0.5f);
  mVertexData.Vertices[11].Color = glm::vec3(0.5f, 1.0f, 0.5f);

  mVertexData.Vertices[6].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[7].UV = glm::vec2(1.0, 1.0);
  mVertexData.Vertices[8].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[9].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[10].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[11].UV = glm::vec2(0.0, 0.0);

  /* left */
  mVertexData.Vertices[12].Position = glm::vec3(-0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[13].Position = glm::vec3(-0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[14].Position = glm::vec3(-0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[15].Position = glm::vec3(-0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[16].Position = glm::vec3(-0.5f,  0.5f, -0.5f);
  mVertexData.Vertices[17].Position = glm::vec3(-0.5f, -0.5f, -0.5f);

  mVertexData.Vertices[12].Color = glm::vec3(0.5f, 0.5f, 1.0f);
  mVertexData.Vertices[13].Color = glm::vec3(0.5f, 0.5f, 1.0f);
  mVertexData.Vertices[14].Color = glm::vec3(0.5f, 0.5f, 1.0f);
  mVertexData.Vertices[15].Color = glm::vec3(0.5f, 0.5f, 1.0f);
  mVertexData.Vertices[16].Color = glm::vec3(0.5f, 0.5f, 1.0f);
  mVertexData.Vertices[17].Color = glm::vec3(0.5f, 0.5f, 1.0f);

  mVertexData.Vertices[12].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[13].UV = glm::vec2(1.0, 1.0);
  mVertexData.Vertices[14].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[15].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[16].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[17].UV = glm::vec2(0.0, 0.0);

  /* right */
  mVertexData.Vertices[18].Position = glm::vec3(0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[19].Position = glm::vec3(0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[20].Position = glm::vec3(0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[21].Position = glm::vec3(0.5f, -0.5f,  0.5f);
  mVertexData.Vertices[22].Position = glm::vec3(0.5f, -0.5f, -0.5f);
  mVertexData.Vertices[23].Position = glm::vec3(0.5f,  0.5f, -0.5f);

  mVertexData.Vertices[18].Color = glm::vec3(0.0f, 0.5f, 0.5f);
  mVertexData.Vertices[19].Color = glm::vec3(0.0f, 0.5f, 0.5f);
  mVertexData.Vertices[20].Color = glm::vec3(0.0f, 0.5f, 0.5f);
  mVertexData.Vertices[21].Color = glm::vec3(0.0f, 0.5f, 0.5f);
  mVertexData.Vertices[22].Color = glm::vec3(0.0f, 0.5f, 0.5f);
  mVertexData.Vertices[23].Color = glm::vec3(0.0f, 0.5f, 0.5f);

  mVertexData.Vertices[18].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[19].UV = glm::vec2(1.0, 1.0);
  mVertexData.Vertices[20].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[21].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[22].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[23].UV = glm::vec2(1.0, 1.0);

  /* top */
  mVertexData.Vertices[24].Position = glm::vec3( 0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[25].Position = glm::vec3(-0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[26].Position = glm::vec3(-0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[27].Position = glm::vec3( 0.5f,  0.5f,  0.5f);
  mVertexData.Vertices[28].Position = glm::vec3( 0.5f,  0.5f,  -0.5f);
  mVertexData.Vertices[29].Position = glm::vec3(-0.5f,  0.5f,  -0.5f);

  mVertexData.Vertices[24].Color = glm::vec3(0.5f, 0.0f, 0.5f);
  mVertexData.Vertices[25].Color = glm::vec3(0.5f, 0.0f, 0.5f);
  mVertexData.Vertices[26].Color = glm::vec3(0.5f, 0.0f, 0.5f);
  mVertexData.Vertices[27].Color = glm::vec3(0.5f, 0.0f, 0.5f);
  mVertexData.Vertices[28].Color = glm::vec3(0.5f, 0.0f, 0.5f);
  mVertexData.Vertices[29].Color = glm::vec3(0.5f, 0.0f, 0.5f);

  mVertexData.Vertices[24].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[25].UV = glm::vec2(1.0, 1.0);
  mVertexData.Vertices[26].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[27].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[28].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[29].UV = glm::vec2(1.0, 1.0);

  /* bottom */
  mVertexData.Vertices[30].Position = glm::vec3( 0.5f,  -0.5f,  0.5f);
  mVertexData.Vertices[31].Position = glm::vec3(-0.5f,  -0.5f,  0.5f);
  mVertexData.Vertices[32].Position = glm::vec3(-0.5f,  -0.5f,  -0.5f);
  mVertexData.Vertices[33].Position = glm::vec3( 0.5f,  -0.5f,  0.5f);
  mVertexData.Vertices[34].Position = glm::vec3(-0.5f,  -0.5f,  -0.5f);
  mVertexData.Vertices[35].Position = glm::vec3( 0.5f,  -0.5f,  -0.5f);

  mVertexData.Vertices[30].Color = glm::vec3(0.5f, 0.5f, 0.0f);
  mVertexData.Vertices[31].Color = glm::vec3(0.5f, 0.5f, 0.0f);
  mVertexData.Vertices[32].Color = glm::vec3(0.5f, 0.5f, 0.0f);
  mVertexData.Vertices[33].Color = glm::vec3(0.5f, 0.5f, 0.0f);
  mVertexData.Vertices[34].Color = glm::vec3(0.5f, 0.5f, 0.0f);
  mVertexData.Vertices[35].Color = glm::vec3(0.5f, 0.5f, 0.0f);

  mVertexData.Vertices[30].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[31].UV = glm::vec2(0.0, 0.0);
  mVertexData.Vertices[32].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[33].UV = glm::vec2(0.0, 1.0);
  mVertexData.Vertices[34].UV = glm::vec2(1.0, 0.0);
  mVertexData.Vertices[35].UV = glm::vec2(1.0, 1.0);

  Logger::Log(1, "%s: loaded %d Vertices\n", __FUNCTION__, mVertexData.Vertices.size());
}

OGLMesh Model_OpenGL::GetVertexData()
{
    if (mVertexData.Vertices.size() == 0)
    {
        Init();
    }

    return mVertexData;
}

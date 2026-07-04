#ifndef CPPANIMPROGRAMMING_CAMERA_H
#define CPPANIMPROGRAMMING_CAMERA_H
#include <glm/glm.hpp>

struct OGLRenderData;

class Camera
{
public:
    glm::mat4 GetViewMatrix(OGLRenderData& RenderData);

private:
    glm::vec3 mViewDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mRightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mUpDirection = glm::vec3(0.0f, 0.0f, 0.0f);

    /* world up is positive Y */
    glm::vec3 mWorldUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
};

#endif //CPPANIMPROGRAMMING_CAMERA_H

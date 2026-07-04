#include "Camera.h"
#include "../Render/OpenGL/OGLRenderData.h"

#include <glm/ext/matrix_transform.hpp>

glm::mat4 Camera::GetViewMatrix(OGLRenderData& RenderData)
{
    float AzimRad = glm::radians(RenderData.rdViewAzimuth);
    float ElevRad = glm::radians(RenderData.rdViewElevation);

    float sinAzim = glm::sin(AzimRad);
    float cosAzim = glm::cos(AzimRad);
    float sinElev = glm::sin(ElevRad);
    float cosElev = glm::cos(ElevRad);

    /* update view direction */
    mViewDirection = glm::normalize(glm::vec3( sinAzim * cosElev, sinElev, -cosAzim * cosElev));

    /* calculate right and up direction */
    mRightDirection = glm::normalize(glm::cross(mViewDirection, mWorldUpVector));
    mUpDirection = glm::normalize(glm::cross(mRightDirection, mViewDirection));

    /* update camera position depending on desired movement */
    RenderData.rdCameraWorldPosition +=
      (RenderData.rdMoveForward * RenderData.rdDeltaTime * mViewDirection)
      + (RenderData.rdMoveRight * RenderData.rdDeltaTime * mRightDirection)
      + (RenderData.rdMoveUp * RenderData.rdDeltaTime * mUpDirection);

    return glm::lookAt(RenderData.rdCameraWorldPosition, RenderData.rdCameraWorldPosition + mViewDirection, mUpDirection);
}

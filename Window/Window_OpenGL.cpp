#include "Window.h"
#include "../Tools/Logger.h"
#include <memory>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include "../Render/OpenGL/OGLRenderer.h"
#include "../Model/Model_OpenGL.h"

GLFWwindow *mWindow = nullptr;
// Track if pressed or released
std::vector<bool> KeyStates(GLFW_KEY_LAST + 1, false);
std::unique_ptr<OGLRenderer> mRenderer;
std::unique_ptr<Model_OpenGL> mModel;

bool Window::Init(unsigned int Width, unsigned int Height, std::string Title)
{
    if (!glfwInit())
    {
        Logger::Log(1, "%s: glfwInit() failed\n", __FUNCTION__);
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(Width, Height, Title.c_str(), nullptr, nullptr);
    if (!mWindow)
    {
        Logger::Log(1, "%s: glfwCreateWindow() failed\n", __FUNCTION__);
        return false;
    }

    glfwMakeContextCurrent(mWindow);

    mRenderer = std::make_unique<OGLRenderer>(mWindow);
    if (!mRenderer->Init(Width, Height))
    {
        glfwTerminate();
        Logger::Log(1, "%s error: Could not init OpenGL\n", __FUNCTION__);
        return false;
    }

    /* the C handlers needs a little 'stunt' here */
    /* save the pointer to the instance as user pointer */
    glfwSetWindowUserPointer(mWindow, mRenderer.get());

    // Window Events
    glfwSetWindowSizeCallback(mWindow, [](GLFWwindow *Win, int Width, int Height)
    {
        const auto Renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(Win));
        Renderer->SetSize(Width, Height);

        // Prevent division by zero if minimized
        if (Width == 0 || Height == 0) return;

        const auto ThisWindow = static_cast<Window*>(glfwGetWindowUserPointer(Win));
        ThisWindow->HandleWindowResizeEvents(Width, Height);
    });

    //Input Events
    glfwSetKeyCallback(mWindow, [](GLFWwindow *Win, int Key, int Scancode, int Action, int Mods)
    {
        const auto Renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(Win));
        Renderer->HandleKeyEvents(Key, Scancode, Action, Mods);
    });

    glfwSetMouseButtonCallback(mWindow, [](GLFWwindow *Win, int Button, int Action, int Mods)
    {
        const auto Renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(Win));
        Renderer->HandleMouseButtonEvents(Button, Action, Mods);
    });

    glfwSetCursorPosCallback(mWindow, [](GLFWwindow *Win, double XPos, double YPos)
    {
        const auto Renderer = static_cast<OGLRenderer*>(glfwGetWindowUserPointer(Win));
        Renderer->HandleMousePositionEvents(XPos, YPos);
    });

    mModel = std::make_unique<Model_OpenGL>();
    mModel->Init();
    Logger::Log(1, "%s: mockup model data loaded\n", __FUNCTION__);

    Logger::Log(1, "%s: OpenGL 4.6 Window creation succeeded\n", __FUNCTION__);
    return true;
}

void Window::InitRenderer()
{

}

void Window::MainLoop()
{
    /* force VSYNC */
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(mWindow))
    {
        mRenderer->Draw();

        /* needed BEFORE event polling for Wayland */
        glfwSwapBuffers(mWindow);

        /* poll events in a loop */
        glfwPollEvents();
    }
}

void Window::ProcessInput()
{
    if (!mWindow) return;

    int State = glfwGetKey(mWindow, GLFW_KEY_W);
    if (State == GLFW_PRESS)
    {
        // Key is currently pressed
    }

#if 0
    for (int Idx = 0; Idx <= GLFW_KEY_LAST; Idx++)
    {
        if (KeyStates[Idx] == true)
        {
            if (Idx == GLFW_KEY_W)
            {
                const char *keyName = glfwGetKeyName(Idx, 0);
                Logger::Log(1, "key %s (key %i) PRESSED\n", keyName, Idx);
            }
        }
    }
#endif
}

void Window::Cleanup()
{
    Logger::Log(1, "%s: Terminating Window\n", __FUNCTION__);
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

// Window Events

void Window::HandleWindowMoveEvents(int XPos, int YPos)
{
    Logger::Log(1, "%s: Window has been moved to %i/%i\n", __FUNCTION__, XPos, YPos);
}

void Window::HandleWindowMinimizedEvents(int Minimized)
{
    if (Minimized)
    {
        Logger::Log(1, "%s: Window has been minimized\n", __FUNCTION__);
    }
    else
    {
        Logger::Log(1, "%s: Window has been restored\n", __FUNCTION__);
    }
}

void Window::HandleWindowMaximizedEvents(int Maximized)
{
    if (Maximized)
    {
        Logger::Log(1, "%s: Window has been maximized\n", __FUNCTION__);
    }
    else
    {
        Logger::Log(1, "%s: Window has been restored\n", __FUNCTION__);
    }
}

void Window::HandleWindowResizeEvents(int Width, int Height)
{
    // Update the OpenGL viewport layout to match the new size
    glViewport(0, 0, Width, Height);

    Logger::Log(1, "%s: Window has been resized to %i/%i\n", __FUNCTION__, Width, Height);
}

void Window::HandleWindowCloseEvents()
{
    Logger::Log(1, "%s: Window got close event\n", __FUNCTION__);
}

// Input Events

void Window::HandleKeyEvents(int Key, int Scancode, int Action, int Mods)
{
    std::string ActionName;
    switch (Action)
    {
        case GLFW_PRESS:
            //ActionName = "pressed";
            KeyStates[Key] = true;
            break;
        case GLFW_RELEASE:
            //ActionName = "released";
            KeyStates[Key] = false;
            break;
        case GLFW_REPEAT:
            //ActionName = "repeated";
            break;
        default:
            ActionName = "invalid";
            break;
    }

    //const char *keyName = glfwGetKeyName(Key, 0);
    //Logger::Log(1, "%s: key %s (key %i, scancode %i) %s\n", __FUNCTION__, keyName, Key, Scancode, ActionName.c_str());
}

void Window::HandleMouseButtonEvents(int Button, int Action, int Mods)
{
    std::string ActionName;
    switch (Action)
    {
        case GLFW_PRESS:
            ActionName = "pressed";
            break;
        case GLFW_RELEASE:
            ActionName = "released";
            break;
        case GLFW_REPEAT:
            ActionName = "repeated";
            break;
        default:
            ActionName = "invalid";
            break;
    }

    std::string MouseButtonName;
    switch(Button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            MouseButtonName = "left";
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            MouseButtonName = "middle";
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            MouseButtonName = "right";
            break;
        default:
            MouseButtonName = "other";
            break;
    }

    Logger::Log(1, "%s: %s mouse button (%i) %s\n", __FUNCTION__, MouseButtonName.c_str(), Button, ActionName.c_str());
}

void Window::HandleMousePositionEvents(double XPos, double YPos)
{
    Logger::Log(1, "%s: Mouse is at position %lf/%lf\n", __FUNCTION__, XPos, YPos);
}

void Window::HandleMouseEnterLeaveEvents(int Enter)
{
    if (Enter)
    {
        Logger::Log(1, "%s: Mouse entered window\n", __FUNCTION__);
    }
    else
    {
        Logger::Log(1, "%s: Mouse left window\n", __FUNCTION__);
    }
}

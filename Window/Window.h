#ifndef CPPANIMPROGRAMMING_WINDOW_H
#define CPPANIMPROGRAMMING_WINDOW_H

#include <string>

class Window
{
public:
    bool Init(unsigned int Width, unsigned int Height, std::string Title);
    // To initialize selected renderer (i.e. Vulkan, OpenGL, SDL...)
    void InitRenderer();
    void MainLoop();
    void ProcessInput();
    void Cleanup();

private:
    // Window Events
    void HandleWindowMoveEvents(int XPos, int YPos);
    void HandleWindowMinimizedEvents(int Minimized);
    void HandleWindowMaximizedEvents(int Maximized);
    void HandleWindowResizeEvents(int Width, int Height);
    void HandleWindowCloseEvents();

    // Input Events
    void HandleKeyEvents(int Key, int Scancode, int Action, int Mods);
    void HandleMouseButtonEvents(int Button, int Action, int Mods);
    void HandleMousePositionEvents(double XPos, double YPos);
    void HandleMouseEnterLeaveEvents(int Enter);
};

#endif //CPPANIMPROGRAMMING_WINDOW_H

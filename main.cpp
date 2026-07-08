#include <memory>
#include "Tools/Logger.h"
#include "Window/Window.h"

int main(int argc, char* argv[])
{
    std::unique_ptr<Window> WindowPtr = std::make_unique<Window>();

    if (!WindowPtr->Init(1280, 720, "CPP Animation Programming"))
    {
        Logger::Log(1, "%s Error: Window init error\n", __FUNCTION__);
        return -1;
    }

    WindowPtr->MainLoop();

    WindowPtr->Cleanup();
    return 0;
}
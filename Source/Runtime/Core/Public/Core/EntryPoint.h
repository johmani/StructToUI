#pragma once

int Core::Main(int argc, char** argv)
{
    while (Application::IsApplicationRunning())
    {
        auto app = Application::CreateApplication({ argv, argc });

        if (app)
        {
            app->Run();
            delete app;
            app = nullptr;
        }
        else break;

#ifdef CORE_ENABLE_LOGGING
        Log::Shutdown();
#endif 
    }

    return 0;
}

#if defined(_WIN64) && defined(CORE_DIST)
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    return Core::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
    return Core::Main(argc, argv);
}

#endif
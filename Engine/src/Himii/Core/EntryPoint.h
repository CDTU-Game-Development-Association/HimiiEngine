#pragma once
#include "Himii/Core/Application.h"
#include "Himii/Core/Core.h"

extern Himii::Application* Himii::CreateApplication(ApplicationCommandLineArgs args);

// -------------------------------------------------------------------------
// 现有的 main 函数（控制台入口）
// -------------------------------------------------------------------------
int main(int argc, char** argv)
{
    Himii::Log::Init();

    HIMII_PROFILE_BEGIN_SESSION("Startup", "HimiiProfile-Startup.json");
    auto app = Himii::CreateApplication({ argc, argv });
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Runtime", "HimiiProfile-Runtime.json");
    app->Run();
    HIMII_PROFILE_END_SESSION();

    HIMII_PROFILE_BEGIN_SESSION("Shutdown", "HimiiProfile-Shutdown.json");
    delete app;
    HIMII_PROFILE_END_SESSION();

    return 0;
}



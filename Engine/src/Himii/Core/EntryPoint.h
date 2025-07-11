﻿#pragma once
#include "Log.h"

extern Himii::Application *Himii::CreateApplication();

int main(int argc, char *argv[])
{
    Himii::Log::Init();
    LOG_CORE_INFO("Application starting...");
    /*LOG_WARNING("This is a warning message.");
    LOG_ERROR("This is an error message.");*/
    // 创建应用程序实例
    auto *app = Himii::CreateApplication();
   
    // 初始化应用程序
    //app->Initialize();

    // 运行应用程序
    app->Run();

    // 清理资源
    delete app;

    return 0;
}

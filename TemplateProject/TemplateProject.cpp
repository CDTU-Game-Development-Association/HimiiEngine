﻿#include "Engine.h"
#include "Himii/Core/EntryPoint.h"
#include "ExampleLayer.h"
#include <iostream>


class TemplateProject : public Himii::Application {
public:
    TemplateProject()
    {
        // 初始化代码
        PushLayer(new ExampleLayer());
    }

    virtual ~TemplateProject()
    {
        // 清理代码
    }
};

Himii::Application *Himii::CreateApplication()
{
    return new TemplateProject();
}

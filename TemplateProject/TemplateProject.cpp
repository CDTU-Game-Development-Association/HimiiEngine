#include "Engine.h"

class TemplateProject : public Engine::Application {
public:
    TemplateProject()
    {
        // ��ʼ������
    }

    virtual ~TemplateProject()
    {
        // �������
    }
};

Engine::Application *Engine::CreateApplication()
{
    return new TemplateProject();
}

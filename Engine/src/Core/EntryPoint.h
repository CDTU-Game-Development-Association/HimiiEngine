#pragma once

int main(int argc, char *argv[])
{
    // ����Ӧ�ó���ʵ��
    auto *app = Core::CreateApplication();

    // ��ʼ��Ӧ�ó���
    app->Initialize();

    // ����Ӧ�ó���
    app->Run();

    // ������Դ
    delete app;

    return 0;
}

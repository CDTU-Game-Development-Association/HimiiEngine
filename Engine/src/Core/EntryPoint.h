#pragma once
#include <iostream>

using namespace std;
int main(int argc, char *argv[])
{
    // ����Ӧ�ó���ʵ��
    auto *app = Core::CreateApplication();
    cout << "Application created successfully." << endl;
    // ��ʼ��Ӧ�ó���
    //app->Initialize();

    // ����Ӧ�ó���
    app->Run();

    // ������Դ
    delete app;

    return 0;
}

#include "test-python.h"
#include <iostream>
#include "Python.h"

TestPython ::TestPython(/* args */)
{
}

TestPython ::~TestPython()
{
}

void TestPython::CallPython()
{
    Py_Initialize();
    if (!Py_IsInitialized())
    {
        std::cout << "初始化失败！" << std::endl;
    }
    else
    {
        std::cout << "初始化成功！" << std::endl;
    }
}

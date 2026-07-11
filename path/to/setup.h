# complete code
// setup.h
#ifndef SETUP_H
#define SETUP_H

#include <Windows.h>

class Setup
{
public:
    static void InitializeSetup();
    static void InstallDependencies();
    static void InstallApplication();
    static void CleanUp();
};

#endif // SETUP_H
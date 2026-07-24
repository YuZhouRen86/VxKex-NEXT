@echo off

where msbuild.exe >nul 2>nul
if %errorlevel% equ 0 (
    msbuild "%~dp0\..\VxKex.sln" /p:Configuration=Release /p:Platform="x64"
    msbuild "%~dp0\..\VxKex.sln" /p:Configuration=Release /p:Platform="Win32"
) else (
    echo Please drag this file into the Visual Studio Command Prompt and press Enter to execute.
    pause
)
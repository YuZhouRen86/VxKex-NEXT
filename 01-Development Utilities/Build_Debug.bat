@echo off

where msbuild.exe >nul 2>nul
if %errorlevel% equ 0 (
    msbuild.exe "%~dp0\..\VxKex.sln" /p:Configuration=Debug /p:Platform="x64"
    msbuild.exe "%~dp0\..\VxKex.sln" /p:Configuration=Debug /p:Platform="Win32"
) else (
    echo Please drag this file into the Visual Studio Command Prompt and press Enter to execute.
    pause
)
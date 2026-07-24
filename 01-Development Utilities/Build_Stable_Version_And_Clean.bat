@echo off

where msbuild.exe >nul 2>nul
if %errorlevel% equ 0 (
    echo(
    echo ========================================
    echo Backing up vautogen files ......
    echo ========================================
    timeout /t 1 /nobreak
    mkdir "%~dp0\..\VAUTOGEN_BACKUP"
    copy /Y "%~dp0\..\00-Common Headers\vautogen.h" "%~dp0\..\VAUTOGEN_BACKUP\"
    copy /Y "%~dp0\..\01-Development Utilities\vautogen\vautogen.ini" "%~dp0\..\VAUTOGEN_BACKUP\"
    echo(
    echo ========================================
    echo Building VxKex NEXT ^(Debug^) ......
    echo ========================================
    timeout /t 1 /nobreak
    call "%~dp0\Build_Debug.bat"
    echo(
    echo ========================================
    echo Restoring vautogen files ......
    echo ========================================
    timeout /t 1 /nobreak
    copy /Y "%~dp0\..\VAUTOGEN_BACKUP\vautogen.h" "%~dp0\..\00-Common Headers\"
    copy /Y "%~dp0\..\VAUTOGEN_BACKUP\vautogen.ini" "%~dp0\..\01-Development Utilities\vautogen\"
    rmdir /S /Q "%~dp0\..\VAUTOGEN_BACKUP"
    echo(
    echo ========================================
    echo Building VxKex NEXT ^(Release^) ......
    echo ========================================
    timeout /t 1 /nobreak
    call "%~dp0\Build_Release.bat"
    echo(
    echo ========================================
    echo Cleaning VxKex NEXT ......
    echo ========================================
    call "%~dp0\Clean_VxKex_NEXT.bat"
) else (
    echo Please drag this file into the Visual Studio Command Prompt and press Enter to execute.
    pause
)
@echo off
setlocal enabledelayedexpansion

where msbuild.exe >nul 2>nul
if %errorlevel% equ 0 (
    if defined VxKex_NEXT_Platform_Toolset (
        echo Using platform toolset from environment: !VxKex_NEXT_Platform_Toolset!
    ) else (
        echo Select platform toolset:
        echo 1 - Windows SDK 7.1 ^(Windows7.1SDK^)
        echo 2 - Visual Studio 2010 ^(v100^)
        echo 3 - Visual Studio 2012 Windows XP ^(v110_xp^)
        echo 4 - Visual Studio 2013 Windows XP ^(v120_xp^)
        echo 5 - Visual Studio 2015 Windows XP ^(v140_xp^)
        set /p "choice=Enter a number and press Enter: "
        if "!choice!"=="1" set "VxKex_NEXT_Platform_Toolset=Windows7.1SDK"
        if "!choice!"=="2" set "VxKex_NEXT_Platform_Toolset=v100"
        if "!choice!"=="3" set "VxKex_NEXT_Platform_Toolset=v110_xp"
        if "!choice!"=="4" set "VxKex_NEXT_Platform_Toolset=v120_xp"
        if "!choice!"=="5" set "VxKex_NEXT_Platform_Toolset=v140_xp"
        if not defined VxKex_NEXT_Platform_Toolset (
            echo Invalid choice, defaulting to v100.
            set "VxKex_NEXT_Platform_Toolset=v100"
        )
    )
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
    echo Please drag this file into Visual Studio Command Prompt or Windows SDK Command Prompt and press Enter to execute.
    pause
)

endlocal
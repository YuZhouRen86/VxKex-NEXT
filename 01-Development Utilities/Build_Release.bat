@echo off
setlocal enabledelayedexpansion

where MSBuild.exe >nul 2>nul
if errorlevel 1 goto :no_msbuild

if defined VxKex_NEXT_Platform_Toolset (
    set "TOOLSET=!VxKex_NEXT_Platform_Toolset!"
    echo Using platform toolset from environment: !TOOLSET!
) else (
    echo Select platform toolset:
    echo 1 - Windows SDK 7.1 ^(Windows7.1SDK^)
    echo 2 - Visual Studio 2010 ^(v100^)
    echo 3 - Visual Studio 2012 Windows XP ^(v110_xp^)
    echo 4 - Visual Studio 2013 Windows XP ^(v120_xp^)
    echo 5 - Visual Studio 2015 Windows XP ^(v140_xp^)
    set /p "choice=Enter a number and press Enter: "
    if "!choice!"=="1" set "TOOLSET=Windows7.1SDK"
    if "!choice!"=="2" set "TOOLSET=v100"
    if "!choice!"=="3" set "TOOLSET=v110_xp"
    if "!choice!"=="4" set "TOOLSET=v120_xp"
    if "!choice!"=="5" set "TOOLSET=v140_xp"
    if not defined TOOLSET (
        echo Invalid choice, defaulting to v100.
        set "TOOLSET=v100"
    )
)

timeout /t 1 /nobreak
call SetEnv.Cmd /x64
MSBuild.exe "%~dp0\..\VxKex.sln" /p:Configuration=Release /p:Platform="x64" /p:PlatformToolset=!TOOLSET!
timeout /t 1 /nobreak
call SetEnv.Cmd /x86
MSBuild.exe "%~dp0\..\VxKex.sln" /p:Configuration=Release /p:Platform="Win32" /p:PlatformToolset=!TOOLSET!

goto :end

:no_msbuild
echo Please drag this file into Visual Studio Command Prompt or Windows SDK Command Prompt and press Enter to execute.
pause

:end
endlocal
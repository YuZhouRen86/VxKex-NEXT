@echo off
cd /D %~dp0
rmdir /S /Q ipch
del /A /F /S /Q *.user *.sdf *.suo *.aps

for /R /D %%f in (x64) do (
    if not "%%f"=="%cd%\02-Prebuilt DLLs\x64" (
        rmdir /S /Q "%%f"
    )
)

for /R /D %%f in (Debug) do (
    rmdir /S /Q "%%f"
)

for /R /D %%f in (Release) do (
    rmdir /S /Q "%%f"
)

pause
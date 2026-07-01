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

del /A /F /S /Q "02-Prebuilt Data\*.pdb"
del /A /F /S /Q "02-Prebuilt Data\KexDir\Globalization\Dictionaries\*"
del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.exe"
del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.pdb"
del /A /F /Q "01-Development Utilities\moz2sst\ROOT.sst"

pause
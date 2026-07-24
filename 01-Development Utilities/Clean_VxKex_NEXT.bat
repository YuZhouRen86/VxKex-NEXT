@echo off
pushd %~dp0\..

for /R /D %%d in (ipch) do if exist "%%d" echo Deleting directory "%%d". && rmdir /S /Q "%%d"
for /R %%f in (*.user *.sdf *.suo *.aps) do echo Deleting file "%%f". && del /A /F /Q "%%f"

for /R /D %%d in (x64) do (
    if not "%%d"=="%cd%\02-Prebuilt DLLs\x64" (
        if exist "%%d" echo Deleting directory "%%d". && rmdir /S /Q "%%d"
    )
)

for /R /D %%d in (Debug) do (
    if exist "%%d" echo Deleting directory "%%d". && rmdir /S /Q "%%d"
)

for /R /D %%d in (Release) do (
    if exist "%%d" echo Deleting directory "%%d". && rmdir /S /Q "%%d"
)

for /R "02-Prebuilt Data" %%f in (*.pdb) do echo Deleting file "%%f". && del /A /F /Q "%%f"
for %%f in ("02-Prebuilt Data\KexDir\Globalization\Dictionaries\*") do echo Deleting file "%%f". && del /A /F /Q "%%f"
if exist "01-Development Utilities\moz2sst\moz2sst.exe" del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.exe"
if exist "01-Development Utilities\moz2sst\moz2sst.pdb" del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.pdb"
if exist "01-Development Utilities\moz2sst\ROOT.sst" del /A /F /Q "01-Development Utilities\moz2sst\ROOT.sst"

popd

pause
@echo off
pushd %~dp0\..

for /F "delims=" %%d in ('dir /AD /B /S 2^>nul') do (
    for %%a in (ipch .vs x64 Debug Release) do (
        if /I "%%~nxd"=="%%a" (
            if not "%%d"=="%cd%\02-Prebuilt DLLs\x64" (
                if exist "%%d" echo Deleting directory "%%d". && rmdir /S /Q "%%d"
            )
        )
    )
)

for /F "delims=" %%f in ('dir /A-D /B /S *.aps *.cache *.sdf *.suo *.user *.VC.db *.VC.opendb 2^>nul') do if exist "%%f" echo Deleting file "%%f" && del /A /F /Q "%%f"
for /F "delims=" %%f in ('dir /A-D /B /S "02-Prebuilt Data\*.pdb" 2^>nul') do if exist "%%f" echo Deleting file "%%f". && del /A /F /Q "%%f"
for /F "delims=" %%f in ('dir /A-D /B /S "02-Prebuilt Data\KexDir\Globalization\Dictionaries\*" 2^>nul') do if exist "%%f" echo Deleting file "%%f". && del /A /F /Q "%%f"
if exist "01-Development Utilities\moz2sst\moz2sst.exe" del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.exe"
if exist "01-Development Utilities\moz2sst\moz2sst.pdb" del /A /F /Q "01-Development Utilities\moz2sst\moz2sst.pdb"
if exist "01-Development Utilities\moz2sst\ROOT.sst" del /A /F /Q "01-Development Utilities\moz2sst\ROOT.sst"

popd

if not defined VxKex_NEXT_NOPAUSE (pause)
@echo off

REM Keep the PUSHD/POPD intact. Otherwise the MAKESFX.bat will break.
PUSHD %~dp0

REM
REM Regenerate KexMLS .bdi files.
REM This should be run automatically while building the KexSetup SFX.
REM

for /F "delims=" %%f in ('dir /A-D /B /S "KexDir\Globalization\Dictionaries\*" 2^>nul') do if exist "%%f" del /A /F /Q "%%f"
MLSBDIC.EXE /IN:Dictionaries /OUT:KexDir\Globalization\Dictionaries

POPD
EXIT /B %ErrorLevel%
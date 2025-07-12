@echo off

echo Setup command line

:: Use vswhere to find the latest Visual Studio installation
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VSINSTALLDIR=%%i
)

:: Check if the installation path was found
if not defined VSINSTALLDIR (
    echo Visual Studio installation not found, exiting
    exit /b 1
)

:: Log which edition is being used
echo Using Visual Studio at %VSINSTALLDIR%

:: Call VsDevCmd.bat to set environment variables (includes signtool and msbuild in PATH)
call "%VSINSTALLDIR%\Common7\Tools\VsDevCmd.bat"

:: Verify signtool is available
where signtool >nul 2>nul
if %errorlevel% neq 0 (
    echo Unable to find signtool, exiting
    exit /b 1
)

:: Verify msbuild is available
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    echo Unable to find msbuild, exiting
    exit /b 1
)

echo Environment setup complete.
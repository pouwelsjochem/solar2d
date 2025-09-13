@echo off
setlocal

echo Setup command line

:: Use vswhere to find the latest Visual Studio installation
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set "VSINSTALLDIR=%%i"
)

:: Check if the installation path was found
if not defined VSINSTALLDIR (
    echo Visual Studio installation not found, exiting
    exit /b 1
)

:: Extract edition name from path (Enterprise, Professional, or Community)
for %%E in (Enterprise Professional Community) do (
    echo %VSINSTALLDIR% | findstr /i "%%E" >nul
    if %errorlevel%==0 (
        set "VSEDITION=%%E"
    )
)

:: Log which edition is being used
if defined VSEDITION (
    echo Using Visual Studio 2022 %VSEDITION% at %VSINSTALLDIR%
) else (
    echo Using Visual Studio at %VSINSTALLDIR%
)

:: Call VsDevCmd.bat to set environment variables
call "%VSINSTALLDIR%\Common7\Tools\VsDevCmd.bat"

:: Verify signtool is available
where signtool >nul 2>nul
if %errorlevel% neq 0 (
    echo Unable to find signtool, exiting
    exit /b 1
)

:: Verify devenv is available
where devenv >nul 2>nul
if %errorlevel% neq 0 (
    echo Warning: 'devenv' not found in PATH. You can use MSBuild instead.
)

echo Environment setup complete.
endlocal
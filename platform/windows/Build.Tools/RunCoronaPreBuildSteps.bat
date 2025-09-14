@echo off
setlocal enabledelayedexpansion

:: ===== EXIT CODE LEGEND =====
:: 0  = Success
:: 1  = STEP 1: Moving redist files failed
:: 2  = STEP 2a: Corona.Shell rebuild failed
:: 3  = STEP 2b: Corona.Simulator build failed
:: 4  = STEP 2c: CoronaBuilder build failed
:: 5  = STEP 3: Copying Simulator.pdb failed
:: 6  = STEP 7: Restoring .pdb failed
:: 7  = STEP 8a: heat.exe not found
:: 8  = STEP 8b: heat dir command failed
:: 9  = STEP 8c: heat file command failed

echo === STEP 1: Moving redist files ===
move /y "%~dp0..\Bin\redist\*" "%~dp0..\Bin\Corona\" || exit /b 3

echo === STEP 2: Building Corona projects ===
call "%DevEnvDir%devenv" "%~dp0..\Corona.Shell.sln" /rebuild "Release|Win32" || exit /b 2
call "%DevEnvDir%devenv" "%~dp0..\Corona.Simulator.sln" /build "Release|Win32" || exit /b 3
call "%DevEnvDir%devenv" "%~dp0..\CoronaBuilder.sln" /build "Release|Win32" || exit /b 4

echo === STEP 3: Saving Corona Simulator.pdb ===
copy "%~dp0..\Bin\Corona\Corona Simulator.pdb" "%TEMP%" || exit /b 5

echo === STEP 4: Cleaning up intermediate files ===
del /q /s "%~dp0..\Bin\Corona\*.exp"
del /q /s "%~dp0..\Bin\Corona\*.ilk"
del /q /s "%~dp0..\Bin\Corona\*.lib"
del /q /s "%~dp0..\Bin\Corona\*.pdb"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.exp"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.ilk"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.lib"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.pdb"

echo === STEP 5: Copying native libs ===
xcopy /y "%~dp0..\Bin\Corona.Enterprise\lib" "%~dp0..\Bin\Corona\Native\Corona\win\lib\" >nul

echo === STEP 6: Copying CoronaBuilder files ===
xcopy /y "%~dp0..\..\bin\win\CopyResources.bat" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul
move /y "%~dp0..\Bin\Corona\lfs.dll" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul
move /y "%~dp0..\Bin\Corona\CoronaBuilder.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul
xcopy /y "%~dp0..\..\bin\shared\Compile.lua" "%~dp0..\Bin\Corona\Native\Corona\shared\bin" >nul
xcopy /y "%~dp0..\Bin\Lua\lua.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul
xcopy /y "%~dp0..\Bin\Lua\lua.dll" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul
xcopy /y "%~dp0..\Bin\Lua\luac.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" >nul

echo === STEP 7: Restoring Corona Simulator.pdb ===
copy "%TEMP%\Corona Simulator.pdb" "%~dp0..\Bin\Corona\" || exit /b 6

echo === STEP 8: Generating WIX files ===
if not exist "%WIX%\bin\heat.exe" (
  echo ERROR: heat.exe not found in %WIX%\bin\
  exit /b 7
)

"%WIX%\bin\heat" dir "%~dp0..\Bin\Corona" -t "%~dp0ExcludePDB.xsl" -dr APP_INSTALL_FOLDER -cg ApplicationFiles -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "%~dp0Generated Files\ApplicationFiles.wxs"
if errorlevel 1 exit /b 8

"%WIX%\bin\heat" file "%~dp0..\Bin\Corona\Corona Simulator.pdb" -dr APP_INSTALL_FOLDER -cg PDBFile -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "%~dp0Generated Files\PDBFile.wxs"
if errorlevel 1 exit /b 9

echo === All pre-build steps completed successfully ===
exit /b 0
@echo on
setlocal enabledelayedexpansion

echo === Starting Corona Pre-Build Script ===

:: Step 1
echo === STEP 1: Moving redist files ===
move /y "%~dp0..\Bin\redist\*" "%~dp0..\Bin\Corona\" || goto :error

:: Step 2
echo === STEP 2: Building Corona projects ===
call "%DevEnvDir%devenv" "%~dp0..\Corona.Shell.sln" /rebuild "Release|Win32" || goto :error
call "%DevEnvDir%devenv" "%~dp0..\Corona.Simulator.sln" /build "Release|Win32" || goto :error
call "%DevEnvDir%devenv" "%~dp0..\CoronaBuilder.sln" /build "Release|Win32" || goto :error

:: Step 3
echo === STEP 3: Saving Corona Simulator.pdb ===
copy "%~dp0..\Bin\Corona\Corona Simulator.pdb" "%TEMP%" || goto :error

:: Step 4
echo === STEP 4: Cleaning up intermediate files ===
del /q /s "%~dp0..\Bin\Corona\*.exp"
del /q /s "%~dp0..\Bin\Corona\*.ilk"
del /q /s "%~dp0..\Bin\Corona\*.lib"
del /q /s "%~dp0..\Bin\Corona\*.pdb"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.exp"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.ilk"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.lib"
del /q /s "%~dp0..\Bin\AppTemplates\Win32\*.pdb"

:: Step 5
echo === STEP 5: Copying native libs ===
xcopy /y "%~dp0..\Bin\Corona.Enterprise\lib" "%~dp0..\Bin\Corona\Native\Corona\win\lib\" || goto :error

:: Step 6
echo === STEP 6: Copying CoronaBuilder files ===
xcopy /y "%~dp0..\..\bin\win\CopyResources.bat" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error
move /y "%~dp0..\Bin\Corona\lfs.dll" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error
move /y "%~dp0..\Bin\Corona\CoronaBuilder.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error
xcopy /y "%~dp0..\..\bin\shared\Compile.lua" "%~dp0..\Bin\Corona\Native\Corona\shared\bin" || goto :error
xcopy /y "%~dp0..\Bin\Lua\lua.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error
xcopy /y "%~dp0..\Bin\Lua\lua.dll" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error
xcopy /y "%~dp0..\Bin\Lua\luac.exe" "%~dp0..\Bin\Corona\Native\Corona\win\bin\" || goto :error

:: Step 7
echo === STEP 7: Restoring Corona Simulator.pdb ===
copy "%TEMP%\Corona Simulator.pdb" "%~dp0..\Bin\Corona\" || goto :error

:: Step 8
echo === STEP 8: Generating WIX files ===
if exist "%WIX%\bin\heat.exe" (
  echo heat.exe found in %WIX%\bin\
) else (
  echo ERROR: heat.exe not found in %WIX%\bin\
  goto :error
)

"%WIX%\bin\heat" dir "%~dp0..\Bin\Corona" -t "%~dp0ExcludePDB.xsl" -dr APP_INSTALL_FOLDER -cg ApplicationFiles -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "%~dp0Generated Files\ApplicationFiles.wxs" || goto :error
"%WIX%\bin\heat" file "%~dp0..\Bin\Corona\Corona Simulator.pdb" -dr APP_INSTALL_FOLDER -cg PDBFile -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "%~dp0Generated Files\PDBFile.wxs" || goto :error

echo === All pre-build steps completed successfully ===
exit /b 0

:error
echo === Pre-build step failed at step: %errorlevel% ===
exit /b 1
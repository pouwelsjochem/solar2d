@echo on
setlocal enabledelayedexpansion

:: === Only continue if this is a Release build ===
if /I "$(Configuration)"=="Release" (

    echo === STEP 1: Clean target directories ===
    rmdir /q /s "$(SolutionDir)Bin\$(ProjectName)" 2>nul
    rmdir /q /s "$(SolutionDir)Bin\Corona" 2>nul
    rmdir /q /s "$(SolutionDir)Bin\Corona.Enterprise" 2>nul
    rmdir /q /s "$(SolutionDir)Bin\AppTemplates\Win32" 2>nul

    echo === STEP 2: Create base directories ===
    mkdir "$(SolutionDir)Bin\Corona"

    echo === STEP 3: Move Native, JRE, and redist ===
    move /y "$(SolutionDir)Bin\Native" "$(SolutionDir)Bin\Corona\" || exit /b 2
    move /y "$(SolutionDir)Bin\jre" "$(SolutionDir)Bin\Corona\" || exit /b 2
    move /y "$(SolutionDir)Bin\redist\*" "$(SolutionDir)Bin\Corona\" || exit /b 2

    echo === STEP 4: Build Corona projects ===
    "$(DevEnvDir)devenv" "$(SolutionDir)Corona.Shell.sln" /rebuild "Release|Win32" || exit /b 3
    "$(DevEnvDir)devenv" "$(SolutionDir)Corona.Simulator.sln" /build "Release|Win32" || exit /b 4
    "$(DevEnvDir)devenv" "$(SolutionDir)CoronaBuilder.sln" /build "Release|Win32" || exit /b 5

    echo === STEP 5: Save Corona Simulator.pdb ===
    copy "$(SolutionDir)Bin\Corona\Corona Simulator.pdb" "%TEMP%" || exit /b 6

    echo === STEP 6: Clean intermediate files ===
    del /q /s "$(SolutionDir)Bin\Corona\*.exp"
    del /q /s "$(SolutionDir)Bin\Corona\*.ilk"
    del /q /s "$(SolutionDir)Bin\Corona\*.lib"
    del /q /s "$(SolutionDir)Bin\Corona\*.pdb"
    del /q /s "$(SolutionDir)Bin\AppTemplates\Win32\*.exp"
    del /q /s "$(SolutionDir)Bin\AppTemplates\Win32\*.ilk"
    del /q /s "$(SolutionDir)Bin\AppTemplates\Win32\*.lib"
    del /q /s "$(SolutionDir)Bin\AppTemplates\Win32\*.pdb"

    echo === STEP 7: Copy native libs ===
    xcopy /y "$(SolutionDir)Bin\Corona.Enterprise\lib" "$(SolutionDir)Bin\Corona\Native\Corona\win\lib\" >nul

    echo === STEP 8: Copy CoronaBuilder files ===
    xcopy /y "$(SolutionDir)..\..\bin\win\CopyResources.bat" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul
    move /y "$(SolutionDir)Bin\Corona\lfs.dll" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul
    move /y "$(SolutionDir)Bin\Corona\CoronaBuilder.exe" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul
    xcopy /y "$(SolutionDir)..\..\bin\shared\Compile.lua" "$(SolutionDir)Bin\Corona\Native\Corona\shared\bin" >nul
    xcopy /y "$(SolutionDir)Bin\Lua\lua.exe" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul
    xcopy /y "$(SolutionDir)Bin\Lua\lua.dll" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul
    xcopy /y "$(SolutionDir)Bin\Lua\luac.exe" "$(SolutionDir)Bin\Corona\Native\Corona\win\bin\" >nul

    echo === STEP 9: Restore Corona Simulator.pdb ===
    copy "%TEMP%\Corona Simulator.pdb" "$(SolutionDir)Bin\Corona\" || exit /b 7
)

echo === STEP 10: Generate WIX files ===
if not exist "$(WIX)\bin\heat.exe" (
  echo ERROR: heat.exe not found in $(WIX)\bin\
  exit /b 8
)

"$(WIX)\bin\heat" dir "$(SolutionDir)Bin\Corona" -t "$(ProjectDir)ExcludePDB.xsl" -dr APP_INSTALL_FOLDER -cg ApplicationFiles -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "$(ProjectDir)Generated Files\ApplicationFiles.wxs"
if errorlevel 1 exit /b 9

"$(WIX)\bin\heat" file "$(SolutionDir)Bin\Corona\Corona Simulator.pdb" -dr APP_INSTALL_FOLDER -cg PDBFile -ag -scom -sreg -sfrag -srd -var "var.CoronaSdkDir" -out "$(ProjectDir)Generated Files\PDBFile.wxs"
if errorlevel 1 exit /b 10

echo === All pre-build steps completed successfully ===
exit /b 0
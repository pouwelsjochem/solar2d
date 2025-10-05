# Building for Windows Desktop

## Prerequisites:

You must install the following before building Corona Labs' Win32 software.

- [Visual Studio 2019 Community Edition](https://aka.ms/vs/16/release/vs_community.exe)
  - Select Native Development workload
  - Add individual components:
    - C++ Windows XP Support for VS 2017 (v141) tools [Deprecated]
    - C++ MFC for v141 build tools (x86 & x64)

## How to build "Corona Simulator":

- Double click on "Corona.Simulator.sln" file.

  - Solution file is used to build and debug Corona Simulator software.
  - Outputs to:

        .\platform\windows\bin\Corona

## Automating local installs

- The Windows installer honours optional environment overrides when it launches:
  - `SOLAR2D_INSTALL_PATH` pre-populates the destination folder.
  - `SOLAR2D_VERSION` replaces the version registered under `Software\Corona Labs\Corona SDK\Install`.
  Export these variables (or pass them to `msiexec`) to drive unattended installs in CI/CD flows.
- To activate a locally built simulator without running the MSI, use `tools/activate-solar2d.ps1`:
  - `pwsh tools/activate-solar2d.ps1 -InstallPath C:\Solar2D\nightly -CopyContent` mirrors `platform/windows/bin/Corona` to the target, updates `CORONA_PATH`, and refreshes the registry version (add `-Version 2024.9999` for a custom identifier).
  - Append `-Scope Machine` when you need machine-wide environment variables; administrator rights are required for that option.

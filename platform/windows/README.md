# Building for Windows Desktop

## Prerequisites

Install Visual Studio with the Desktop development with C++ workload, the v142
x86 build tools, and MFC for v142. Building the MSI also requires WiX Toolset
v3.

## Build ownership

Open `Corona.Simulator.sln` to build and debug the Simulator. Its output is
written to `Bin\Corona`.

Open `CoronaBuilder.sln` to build the command-line packager and the Win32 app
template. CoronaBuilder owns its templates and packaging resources; building the
Simulator no longer invokes a nested app-template build.

Open `Corona.SDK.sln` to build the complete MSI. The installer builds the
Simulator and CoronaBuilder and stages only the Windows-native packaging tools.
It does not require the macOS `Native.tar.gz` artifact or a bundled JRE.

## Agent mode

Agent mode launches a project in a borderless, menu-free Simulator window and
keeps logs on standard output and error. It requires an explicit project
directory containing `main.lua`:

```powershell
Start-Process `
  -FilePath ".\Bin\Corona\Corona Simulator.exe" `
  -ArgumentList "-agent-mode", "YES", "-project", "C:\path\to\project" `
  -NoNewWindow -Wait
```

Agent sessions do not read or write recent-project, device, working-directory,
window-position, or previous-crash preferences. They never close another
running Simulator instance.

The Simulator exposes the same programmatic device, relaunch, input, simulated
event, fullscreen, state, and quit API used on macOS. Windows does not draw
rounded device corners or a safe-area guide overlay; requesting rounded corners
or enabling that overlay returns an error. Custom safe-area insets still affect
the simulated runtime.

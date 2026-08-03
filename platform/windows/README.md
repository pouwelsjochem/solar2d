# Building for Windows Desktop

## Prerequisites

Install Visual Studio with the Desktop development with C++ workload, the v142
x86 build tools, and MFC for v142. Building the MSI also requires WiX Toolset
v3.

The Windows projects restore the Microsoft WebView2 SDK through NuGet. Enable
automatic NuGet package restore in Visual Studio, or restore the solution before
building. The loader is linked statically, but running a native web view still
requires the [Evergreen WebView2 Runtime](https://learn.microsoft.com/microsoft-edge/webview2/concepts/distribution)
to be installed on the test machine.

## Build ownership

Open `Corona.Simulator.sln` to build and debug the Simulator. Its output is
written to `Bin\Corona`.

Open `CoronaBuilder.sln` to build the command-line packager and the Win32 app
template. CoronaBuilder owns its templates and packaging resources; building the
Simulator no longer invokes a nested app-template build.

Open `Corona.SDK.sln` to build the complete MSI. The installer builds the
Simulator and CoronaBuilder and stages only the Windows-native packaging tools.
It does not require the macOS `Native.tar.gz` artifact or a bundled JRE.

## Packaged app diagnostics

Packaged Win32 apps keep their five most recent launch logs in:

```text
%LOCALAPPDATA%\<company>\<product>\Logs
```

Each log uses its UTC launch time and process ID, for example
`launch-2026-08-03T14-58-12-345Z-1234.log`. Logs include startup milestones,
app and Solar2D versions, Windows and display-adapter information, and engine
messages. Lua `print()` output is not captured. Each log is capped at 5 MB.
The five newest files are retained according to their Windows last-modified
time. A log still open by another app instance is left in place and retried on
a later launch. Recognized startup errors show the current log path and a stable
error code in their error dialog.

Unhandled native exceptions also write up to five minidumps under the adjacent
`Crashes` directory. Dumps can contain private process memory and should be
transferred securely. Matching app-template and native-runtime PDBs are copied
to `Bin\Symbols\Win32` when building `CoronaBuilder.sln`; archive that directory
for every engine version used to ship a game.

Windows loader failures caused by a missing or invalid imported DLL happen
before the app entry point and cannot be recorded by this facility. Use Windows
Error Reporting, Reliability Monitor, or an external launcher for those cases.

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

Pass `-simulator-control-dir C:\private\session` to enable native local Lua
inspection, execution, and lifecycle control without modifying the project.
See [Simulator control](../../tools/simulator-control/README.md) for the command
reference.

The Simulator exposes the same programmatic device, relaunch, input, simulated
event, fullscreen, state, and quit API used on macOS. Windows does not draw
rounded device corners or a safe-area guide overlay; requesting rounded corners
or enabling that overlay returns an error. Custom safe-area insets still affect
the simulated runtime.

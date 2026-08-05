# macOS build Guide

To run and test Simulator open `ratatouille.xcodeproj` and select `rttplayer` target in Xcode. Press play.
Sometimes Xcode doesn't follow dependency order for tools, so you may have to build `Lua` and `Luac` targets first.

## Launching with a simulator device

The Simulator accepts a temporary device configuration on the command line. It is applied after loading the project's
`simulator/devices` directory and does not change the saved device preference.

Select a built-in or project device by its stable identifier:

```sh
"/Applications/Corona Simulator.app/Contents/MacOS/Corona Simulator" \
  -project "/path/to/project" \
  -simulator-device "project:tablet"
```

Or provide a custom size, with optional safe area insets:

```sh
"/Applications/Corona Simulator.app/Contents/MacOS/Corona Simulator" \
  -project "/path/to/project" \
  -simulator-width 1920 \
  -simulator-height 1080 \
  -simulator-safe-area-top 24 \
  -simulator-safe-area-left 0 \
  -simulator-safe-area-bottom 24 \
  -simulator-safe-area-right 0 \
  -simulator-rounded-corners false
```

`-simulator-rounded-corners true|false` can also be used by itself with the saved device. Custom dimensions must be
integers from 1 through 16384; omitted safe area insets default to zero.

## Agent mode

Agent mode gives Codex a clean Simulator window while leaving visual inspection
and interaction to Computer Use. A project path is required.

```sh
"/Applications/Corona Simulator.app/Contents/MacOS/Corona Simulator" \
  -agent-mode YES \
  -project "/path/to/project"
```

The Simulator remains attached to its launching terminal. Lua output and runtime
errors are written to standard output and error. Keep that terminal session open
while using Computer Use to inspect and interact with the rendered OpenGL
surface. Applications should provide their own log message when they need a
reliable, app-specific signal that initialization is complete.

Pass `-simulator-control-dir /private/session/path` to enable native local Lua
inspection, execution, and lifecycle control without modifying the project.
See [Simulator control](../../tools/simulator-control/README.md) for the command
reference.

Agent sessions do not update recent projects or persist their window position.
They start centered with rounded corners and safe-area guides disabled for an
unobstructed surface. Pass
`-simulator-rounded-corners YES` or other launch-time device configuration flags
when device chrome or a specific geometry matters.

## Screen recording

On macOS 15 or later, the Simulator's **File > Start Screen Recording...** command
records the device view directly to an H.264 `.mp4`. The capture and hardware
encoding are handled by ScreenCaptureKit outside Solar2D's render loop. The
selected capture area is fixed when recording starts, so keep the Simulator
window in place until recording ends.

Simulator Lua code can control the same recorder:

```lua
local path = system.pathForFile("capture.mp4", system.DocumentsDirectory)

local accepted, message = simulator.startScreenRecording({
	path = path,             -- Absolute .mp4 path (required).
	fps = 60,                -- Optional; defaults to 60 (1 through 240).
	resolutionScale = 1.0,   -- Optional; native resolution (greater than 0, at most 1).
	includeAudio = true,     -- Optional; defaults to true.
	showCursor = false,      -- Optional; defaults to false.
	overwrite = true         -- Optional; defaults to false.
}, function(event)
	print(event.phase, event.path, event.errorMessage)
	-- event.phase is "started", "ended", or "failed".
end)

if not accepted then
	print(message)
end

-- Finish and finalize the file later.
-- simulator.stopScreenRecording()
```

At the default `resolutionScale = 1.0`, capture uses the display's native pixel
scale, including Retina resolution. Use `0.5` for half the width and height
(about one quarter of the encoded pixels) when lower overhead or smaller files
matter more than maximum sharpness.

`simulator.getScreenRecordingState()` returns `"idle"`, `"starting"`,
`"recording"`, `"stopping"`, or `"unavailable"`. Recording completion is also
dispatched globally as a `screenRecording` Runtime event.

Simulator control clients can use `start-screen-recording`,
`screen-recording-status`, and `stop-screen-recording` without executing Lua.
See [Simulator control](../../tools/simulator-control/README.md) for arguments
and lifecycle details.

# Building your app with CoronaBuilder

The Simulator only runs projects; it does not provide build dialogs. Device and desktop packages are built separately with `CoronaBuilder`.
Some platforms require templates to be built first. CoronaBuilder owns and resolves those templates and related build resources from its own application bundle.

## Building iOS/tvOS apps

First one would need to build templates. To see how, check out the [README](../iphone/README.md) in `iphone` directory.
When templates are built and in place, open `CoronaBuilder.xcodeproj`, select the `CoronaBuilder` target, and build it (⌘B).

# Simulator control

The Solar2D Simulator executable can inspect and change the Lua state of another
running Simulator process. The client is built into the native executable, so
it has no Python, Lua, or other runtime dependency. Projects do not need to
require a library or add control-specific Lua code.

## Start a controllable Simulator

Give the Simulator an explicit private mailbox directory:

```sh
SIMULATOR="/Applications/Corona Simulator.app/Contents/MacOS/Corona Simulator"
CONTROL_DIR="$PWD/.solar2d-control"
mkdir -p "$CONTROL_DIR"
chmod 700 "$CONTROL_DIR"

"$SIMULATOR" \
  -agent-mode YES \
  -project "$PWD" \
  -simulator-control-dir "$CONTROL_DIR"
```

The bridge is disabled unless `-simulator-control-dir` is supplied. The
directory must exist before launch.

## Identify display objects

Every display object can have an optional Simulator automation identifier. It
does not require `widget` or any other library:

```lua
local playButton = display.newRoundedRect( 160, 240, 220, 72, 16 )
playButton.automationId = "playButton"
```

Discovery accepts a non-empty string of at most 128 bytes; other values are
ignored. Keep identifiers unique among the objects in the active display tree,
and assign `nil` to clear one. The identifier is app-authored and remains
stable when the display hierarchy or screen size changes.

The Simulator also assigns each discovered object a numeric `handle`. A handle
such as `@17` is convenient when following an unlabelled object returned by
`display-object-tree` or `hit-test-display-objects`. Handles do not resolve
while an object is outside the active tree, expire when its Lua proxy is
collected, and are not stable across a project relaunch; use `automationId` for
durable selectors.

## Commands

Run the same native executable in control-client mode from another terminal:

```sh
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control runtime-status
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control runtime-diagnostics
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control runtime-logs
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control runtime-logs --filter 'scene loaded' --follow
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control capture-screenshot
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control debug-snapshot
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control start-screen-recording capture.mp4 --fps 60 --overwrite
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control screen-recording-status
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control stop-screen-recording
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control display-object-tree
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control find-display-object playButton
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control hit-test-display-objects 640 360
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control wait-for-display-object playButton
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control tap-display-object playButton
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control swipe-display-object inventoryList 0 -300 12
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control tap-screen 640 360
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control send-touch-event moved 640 300 640 500
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control swipe-screen 640 500 640 200 12
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control send-mouse-event move 640 360
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control press-back-button
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control send-key-event s pressed --command
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control type-text 'Player name'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control send-scroll-event 640 360 0 -120
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control controller send-button-event buttonA
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control controller set-axis leftX -1
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control controller connect --id pad2 --profile playstation --player 2
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control controller send-button-event buttonA --id pad2
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control evaluate-lua 'player.score'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control inspect-lua-value player
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control execute-lua 'player.score = player.score + 100'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control execute-lua-file /absolute/path/to/experiment.lua
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control wait-for-condition 'player.ready'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control assert-condition 'player.score >= 100'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control wait-for-log 'level loaded'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control run-scenario /absolute/path/to/smoke.scenario
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control relaunch-project
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control quit-simulator
```

`evaluate-lua` evaluates one Lua expression and returns all of its values.
`execute-lua` executes statements. Either command reads standard input when its
code argument is omitted. `execute-lua-file` loads a file into the current Lua
runtime; it does not add a module or require project changes.

`runtime-diagnostics` returns the most recent Lua or control-command error,
including its type, message, stack trace, frame, and sequence number. Its
`latestRuntimeError` value is `null` if no error has occurred. Both it and
`runtime-status` include `runtimeErrorHalted`; status also reports an
`executionState` of `running`, `suspended`, or `error-halted`.

## Unhandled runtime errors

When the Lua `unhandledError` listener does not return `true`, a controllable
runtime enters the `error-halted` state after recording the error. The current
frame stops advancing, remaining scheduler tasks do not run, and subsequent
application events and automation input are not dispatched. An error that an
`unhandledError` listener explicitly handles by returning `true` is still
recorded but does not halt the runtime. Errors from `evaluate-lua`,
`execute-lua`, and other control commands are diagnostics only and do not halt
the application.

The control mailbox remains responsive while error-halted. The following safe
inspection, artifact, and lifecycle commands remain available:

- `runtime-status`, `runtime-diagnostics`, and `runtime-logs`
- `capture-screenshot` and `debug-snapshot`
- `screen-recording-status` and `stop-screen-recording`
- `display-object-tree`, `find-display-object`, and
  `hit-test-display-objects`
- `relaunch-project` and `quit-simulator`

Other commands, including waits, Lua inspection/evaluation, input, and
assertions, fail immediately with error code `runtime-error-halted` and include
the latest diagnostics in their result. `relaunch-project` replaces the Lua
runtime and clears the halt as part of the new control session.

`runtime-logs` returns recent Simulator log messages. Pass `--since SEQUENCE`
to return only newer messages, or `--filter TEXT` to return only messages that
contain the given case-sensitive text. `--follow` prints each non-empty result
batch as one line of JSON, flushes it immediately, and continues polling until
the client is interrupted. These options can be combined. Each entry includes
a Unix-epoch `timestampMs`.
`capture-screenshot` writes the current frame to a PNG in the control directory,
or to an optional output path.

`debug-snapshot` writes a PNG and a JSON manifest beside it using the `.png.json`
suffix. The manifest contains runtime status, the latest diagnostic, captured
logs, and the first page of the display tree. It is intended as a compact
debugging bundle rather than a complete trace.

On macOS 15 or later, `start-screen-recording` records the Simulator device view
to an H.264 `.mp4`. Its path may be relative to the control client's working
directory or absolute. Recording defaults to 60 FPS with audio enabled and the
cursor hidden. Pass `--fps FPS`, `--no-audio`, `--show-cursor`, or `--overwrite`
to change those settings. The destination directory must already exist.

Starting and stopping are asynchronous. `start-screen-recording` usually
returns state `"starting"`, and `stop-screen-recording` usually returns
`"stopping"`. Poll `screen-recording-status` until it returns `"recording"`
before performing actions that must appear in the video, then poll until
`"idle"` after stopping before reading the finalized file. It returns
`"unavailable"` when the platform does not support recording.

## Display discovery and semantic input

`display-object-tree` returns the active display hierarchy as a bounded, flat
list. Each node includes its `automationId`, handle, parent handle, child index,
depth, native object type, screen bounds and center, cumulative visibility and
alpha, hit-test state, touch-listener state, and child count. At most 100 nodes
are returned at once; pass the response's `nextCursor` back with `--cursor` to
read the next page.

`find-display-object playButton` resolves one object by `automationId`;
`find-display-object @17` resolves a previously returned handle. Duplicate
identifiers are reported as ambiguous. `tap-display-object` reads the object's
current screen bounds immediately before queuing a touch at its center, so an
agent does not have to remember coordinates across layout changes.
`swipe-display-object` begins at the current center and applies the given
screen-space delta.

`hit-test-display-objects X Y` reports front-to-back candidates at a screen
coordinate. It is explicitly marked `approximate` because it uses transformed
bounds and normal visibility/hit-test flags, not pixel masks or exact vector
geometry.

## Waiting, assertions, and scenarios

`wait-for-condition` evaluates a side-effect-free Lua expression once per
runtime frame until its first return value is truthy. `wait-for-display-object`
waits until a selected object exists, is visible, has non-empty bounds, and can
receive input. `wait-for-log` waits until captured output contains the given
text. `assert-condition` evaluates once and fails unless its first value is
truthy. Each uses the client timeout, which defaults to ten seconds:

```sh
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" \
  -simulator-control --timeout 30 wait-for-condition 'gameState == "playing"'
```

`run-scenario` runs a line-oriented command file. Blank lines and lines
beginning with `--` are ignored; every other line is one normal command followed
by its payload. A scenario is limited to 100 commands, stops at the first
failure, and returns the response from every completed step:

```text
-- smoke.scenario
wait-for-display-object playButton
tap-display-object playButton
wait-for-condition gameState == "playing"
wait-for-log PLAY_BUTTON_TAPPED
assert-condition player.score >= 0
debug-snapshot
```

Failed `wait-for-condition`, `wait-for-display-object`, `wait-for-log`, and
`assert-condition` commands automatically capture `automation-failure.png` and
`automation-failure.png.json` in the control directory. A failed scenario step
does the same and includes the artifact paths in its JSON result.

An executable sample project and scenario are in
[`example`](./example). They exercise delayed discovery, semantic tapping,
waiting, log matching, assertions, and snapshot capture.

## Raw input

`tap-screen` queues touch `began` and `ended` events at the given screen
coordinates; it does not dispatch a separate tap event. The coordinates use the
same simulated screen space returned by `capture-screenshot`.
`send-touch-event` sends an individual `began`, `moved`, `ended`, or `cancelled`
phase and can preserve an explicit gesture start point. `swipe-screen` emits a
began phase, interpolated moved phases, and an ended phase. The optional step
count is from 1 through 120 and defaults to 8.

`send-mouse-event` sends `down`, `up`, `drag`, `move`, `exit`, or `scroll`
phases. `press-back-button` dispatches the platform back action.
`send-key-event` queues a press by default, or one explicit `down` or `up`
phase, and supports Shift, Alt, Control, and Command modifiers. `type-text`
inserts UTF-8 text at the selection of the focused native text field, or
dispatches a `character` event when the simulated screen has focus. It reads
standard input when `TEXT` is omitted. `send-scroll-event` is the compact form
for a mouse scroll event at the given screen coordinate.

Input is dispatched after the control response is written, so handlers may
safely relaunch or close the runtime. Use a `wait-for-*` command or
`assert-condition` after input when the next action depends on its effect.

`controller` provides virtual gamepads through Solar2D's normal input-device
APIs. Commands without `--id` use a default Xbox controller. Use `--id` to
address independent devices, and explicitly `connect` a new ID with an `xbox`,
`playstation`, `nintendo`, or `generic` profile and optional player number from
1 through 4. Profiles set representative product names and controller types;
all provide the standard six-axis extended-gamepad layout.

`controller send-button-event` and `controller set-axis` automatically connect
their target as an Xbox controller if it does not exist yet. `disconnect`
emits a normal `inputDeviceStatus` event, and reconnecting an existing ID
preserves its profile. Connecting it with a different profile or player number
emits a reconfiguration event. Buttons use Solar2D key names such as `buttonA`,
`up`, and `leftShoulderButton1`. Supported axes are `leftX`, `leftY`, `rightX`,
and `rightY` from -1 through 1, plus `leftTrigger` and `rightTrigger` from 0
through 1. Axis values remain set until changed; send 0 to center a stick axis
or release a trigger.

`inspect-lua-value` is read-only. It accepts paths such as
`player.inventory[1]` and `settings["audio"]`, uses raw table access, and never
invokes metamethods. A table response includes its own entries and a handle such
as `12`; inspect that table later with `inspect-lua-value @12`. Large tables
return `nextCursor`, which can be passed as
`inspect-lua-value player --cursor 100`. There is deliberately no depth
argument: nested tables are represented as handles, keeping responses bounded
without making callers choose an arbitrary depth.

`relaunch-project` calls the Simulator's normal relaunch mechanism. It replaces
the Lua runtime exactly as a menu or file-triggered relaunch would; it is not a
separate module-reloading system. The client waits until the replacement Lua
runtime is ready before it exits successfully.

All output is JSON. Successful responses have `ok: true` and unsuccessful Lua
loads, runtime errors, invalid paths, and unsupported commands have `ok: false`.
The client exits with status 0, 1, or 2 for success, Simulator/Lua error, or
client/transport error respectively. Add `--timeout SECONDS` immediately after
`-simulator-control` to change the default ten-second deadline.

## Security and lifecycle

The mailbox is a local filesystem interface with no network listener. Treat
access to its directory as permission to execute arbitrary code in the running
project. Launch scripts should create it with user-only permissions and should
not place it in a shared directory.

The Simulator writes `session.json` once the Lua runtime can accept requests.
Requests are processed on the runtime thread, at most one per frame, so Lua
access does not race the engine. Relaunching replaces the session and invalidates
table and display-object handles. A suspended or blocked Lua runtime cannot
answer until it resumes; an error-halted runtime remains responsive to the safe
commands listed above.

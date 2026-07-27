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

## Commands

Run the same native executable in control-client mode from another terminal:

```sh
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control status
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control diagnostics
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control logs
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control screenshot
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control tap 640 360
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control key escape
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control eval 'player.score'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control inspect player
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control exec 'player.score = player.score + 100'
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control exec-file /absolute/path/to/experiment.lua
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control relaunch
"$SIMULATOR" -simulator-control-dir "$CONTROL_DIR" -simulator-control quit
```

`eval` evaluates one Lua expression and returns all of its values. `exec`
executes statements. Either command reads standard input when its code argument
is omitted. `exec-file` loads a file into the current Lua runtime; it does not
add a module or require project changes.

`diagnostics` returns the most recent Lua or control-command error, including
its type, message, stack trace, frame, and sequence number. Its
`latestRuntimeError` value is `null` if no error has occurred.

`logs` returns recent Simulator log messages. Pass `--since SEQUENCE` to return
only newer messages. `screenshot` writes the current frame to a PNG in the
control directory, or to an optional output path.

`tap` queues touch `began` and `ended` events at the given screen coordinates;
it does not dispatch a separate tap event. The coordinates use the same
simulated screen space returned by `screenshot`. `key` queues a key press by
default, or one explicit `down` or `up` phase. Input is dispatched after the
control response is written, so handlers may safely relaunch or close the
runtime.

`inspect` is read-only. It accepts paths such as `player.inventory[1]` and
`settings["audio"]`, uses raw table access, and never invokes metamethods. A
table response includes its own entries and a handle such as `12`; inspect that
table later with `inspect @12`. Large tables return `nextCursor`, which can be
passed as `inspect player --cursor 100`. There is deliberately no depth
argument: nested tables are represented as handles, keeping responses bounded
without making callers choose an arbitrary depth.

`relaunch` calls the Simulator's normal relaunch mechanism. It replaces the Lua
runtime exactly as a menu or file-triggered relaunch would; it is not a separate
module-reloading system. The client waits until the replacement Lua runtime is
ready before it exits successfully.

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
table handles. A suspended or blocked Lua runtime cannot answer until it resumes.

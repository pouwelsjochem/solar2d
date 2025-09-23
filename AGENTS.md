# Repository Guidelines

## Project Structure & Module Organization
Solar2D's native runtime lives in `librtt`, which holds the C++ engine core, rendering stack, and Lua bindings. Platform launchers, installers, and platform-specific tooling sit in `platform/<target>` (for example `platform/mac`, `platform/linux`, and `platform/android`), each with its own README. First-party plugins and templates are developed in `plugins`, reusable Lua/C modules are in `modules`, and simulator assets ship from `sdk`. Vendored dependencies remain under `external` and `subrepos`, while build and packaging helpers are collected in `tools`.

## Build, Test, and Development Commands
- `git clone --recursive https://github.com/coronalabs/corona.git`: fetch the repo together with submodules.
- `cd platform/linux && ./setup_dev.sh`: install Linux prerequisites (rerun after SDK updates).
- `cmake -S platform/linux -B platform/linux/build && cmake --build platform/linux/build --target install`: build the Linux simulator/templates and install them to `/usr/local/bin`.
- `platform/mac/build.sh [CUSTOM_ID]`: drive macOS Xcode builds and optionally tag artifacts.
- `cd platform/iphone && ./build.sh`: regenerate native iOS binaries (Lua tools, packaging utilities) before assembling device templates.
- `cd platform/android && ./gradlew setUpCoronaAppAndPlugins assembleRelease`: refresh plugin manifests and produce Android APK/AAB outputs.

## Coding Style & Naming Conventions
Match the surrounding style: C++ sources use tabs with Allman braces, `Rtt_`-prefixed types, and `m*` member fields. Include the standard header from `FILE_HEADER.md` on new files. Lua tooling also favors tabs and snake_case locals; follow existing spacing when editing Gradle or shell scripts. Run platform formatters (for example Xcode's clang-format targets inside the project) only when already used in the touched directory.

## Testing Guidelines
There is no single test runner, so validate on every platform your change touches. Build the relevant simulator/device binaries, then run smoke projects from `platform/test/assets` or a custom project in `platform/test/assets2`. For engine-side features, add or update Lua repro scenes and confirm plugin setup via `setUpCoronaAppAndPlugins`. Capture crash logs or regression evidence when filing issues.

## Commit & Pull Request Guidelines
Write commits in imperative mood (e.g. `Fix Android signing regression`) and keep them scoped to one logical change. Reference affected platforms or plugins in the body, and update documentation when behavior changes. Every pull request must include a short testing note, linked issue (if applicable), and confirmation that the CLA in `CLA.md` has been signed. Provide screenshots or console output when UI or build scripts change, and request reviews from maintainers owning the touched platform directories.

# SDL_GameControllerDB Integration

This directory contains `gamecontrollerdb.txt` from https://github.com/mdqinc/SDL_GameControllerDB

## Purpose

The database is used to reliably identify game controller types (Xbox, PlayStation, Nintendo, etc.) for displaying correct button glyphs in game UIs.

## Usage

The database is automatically loaded by Solar2D's input system. To initialize it, call:

```cpp
#include "Input/Rtt_GameControllerDB.h"

// Load from file
GameControllerDB* db = GameControllerDB::GetInstance();
db->LoadFromFile("path/to/gamecontrollerdb.txt");
```

## Platform Integration

Each platform generates SDL-compatible GUIDs from USB vendor/product IDs:

- **Windows**: Extracted from DirectInput GUID or hardcoded for XInput
- **Linux**: Read from `/sys/class/input/jsX/device/id/vendor` and `/product`
- **Android**: Use `InputDevice.getVendorId()` / `getProductId()` (API 19+)
- **macOS/iOS**: Extract from IOKit HID properties
- **Switch**: Hardcoded Nintendo vendor/product IDs

## Lua API

Controllers are automatically classified:

```lua
local function onKey(event)
    local controllerType = event.device.controllerType
    -- Returns: "xbox", "playstation", "nintendo", "steam", "generic", "unknown"

    if controllerType == "playstation" then
        showButton("circle.png")
    elseif controllerType == "xbox" then
        showButton("b.png")
    elseif controllerType == "nintendo" then
        showButton("a.png")
    end
end

Runtime:addEventListener("key", onKey)
```

## Updating the Database

To update to the latest version:

```bash
curl -o gamecontrollerdb.txt https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/master/gamecontrollerdb.txt
```

## Format

Each line in the database follows SDL's format:
```
GUID,Controller Name,button mappings...,platform:PlatformName
```

Example:
```
030000004c050000c405000000010000,PS4 Controller,a:b1,b:b2,...,platform:Windows,
```

## GUID Format

SDL uses a 32-character hex string for device identification:
- Bytes 0-1: Bus type (0x03 for USB)
- Bytes 4-5: Vendor ID (little-endian)
- Bytes 8-9: Product ID (little-endian)

Example: `03000000de280000ff11000001000000`
- Vendor: 0x28de (Valve)
- Product: 0x11ff (Steam Controller)

## Supported Platforms

The database includes mappings for:
- Windows
- Mac OS X
- Linux
- iOS
- tvOS
- Android
- Nintendo Switch

## License

The database is maintained by the community under the SDL license.

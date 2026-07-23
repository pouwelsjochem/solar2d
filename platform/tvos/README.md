# tvOS build guide

To run a Solar2D project directly from Xcode, create `platform/test/assets2` and copy the project's
Lua files and assets into it. Open `ratatouille.xcodeproj`, select the `template` target, choose an
Apple TV device or simulator, and press Run. Select `template-angle` to exercise the ANGLE renderer.

Do not commit `platform/test/assets2`.

## Building templates for CoronaBuilder

CoronaBuilder packages tvOS apps from archives embedded in
`platform/resources/iostemplate`. Build both the device and simulator archives from the repository
root:

```bash
platform/tvos/gh_build_templates.sh
TEMPLATE_TARGET=template-angle platform/tvos/gh_build_templates.sh
cp output/appletv*.tar.bz platform/resources/iostemplate/
```

Rebuild CoronaBuilder after copying the archives. A tvOS Simulator build uses
`"targetDevice": "tvos-simulator"` and does not require a provisioning profile. Device builds still
require `certificatePath`.

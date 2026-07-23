
CoronaBuilder embeds the iOS and tvOS template archives in this directory. Archive names identify
the Xcode SDK and optional renderer, for example `iphoneos_26.5.tar.bz` or
`appletvsimulator_26.5-angle.tar.bz`.

Build archives from the repository root with:

```bash
platform/iphone/gh_build_templates.sh
platform/tvos/gh_build_templates.sh
TEMPLATE_TARGET=template-angle platform/iphone/gh_build_templates.sh
TEMPLATE_TARGET=template-angle platform/tvos/gh_build_templates.sh
cp output/*.tar.bz platform/resources/iostemplate/
```

Rebuild CoronaBuilder after copying the archives so that they are included in its app bundle.

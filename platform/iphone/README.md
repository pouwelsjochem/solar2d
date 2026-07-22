# iOS build Guide

To run Corona app on device or Xcode simulator create folder `/platform/test/assets2`, copy your Corona lua project and assets there.
Then open `ratatouille.xcodeproj`, select `template` target and press Run.
If you wish to test MetalANGLE templates, use `template-angle` target.

Note, that `platform/test/assets2` directory must not be committed to the repo.


# Building iOS templates to use in Simulator

CoronaBuilder uses templates from `platform/resources/iostemplate` to build iOS and tvOS apps. They contain prebuilt templates whose contents are replaced and linked with plugins during packaging.

Here are commands which can be used to build templates. All commands assumed to be run from the repository root

```bash
# Important: make sure test platform is clean.
# Warning: This will delete all extra files in it
git clean -xfd platform/test
# build template. Output would be in the output/ directory
platform/iphone/gh_build_templates.sh
# for MetalANGLE templates run
TEMPLATE_TARGET=template-angle platform/iphone/gh_build_templates.sh

# copy templates where then can be found by simulator build
cp output/* platform/resources/iostemplate/
```

Alternatively, copy templates into `<CoronaBuilder.app>/Contents/Resources/iostemplate` in an existing CoronaBuilder build.

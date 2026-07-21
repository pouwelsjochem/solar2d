#!/usr/bin/env bash

set -ex

WORKSPACE=$(cd "$(dirname "$0")/../.." && pwd)
CONFIG=Release
STAGING_DIR="$WORKSPACE/build/solar2d-tools"
NATIVE_DIR="$STAGING_DIR/Native"
CORONA_DIR="$NATIVE_DIR/Corona"
MAC_BIN_DIR="$CORONA_DIR/mac/bin"

rm -rf "$STAGING_DIR"
mkdir -p "$MAC_BIN_DIR" "$CORONA_DIR/win/bin" "$CORONA_DIR/android/resource" "$CORONA_DIR/android/lib/gradle" "$CORONA_DIR/shared/resource"

xcodebuild -project "$WORKSPACE/platform/mac/ratatouille.xcodeproj" -scheme lua -configuration "$CONFIG"
xcodebuild -project "$WORKSPACE/platform/mac/ratatouille.xcodeproj" -scheme luac -configuration "$CONFIG"

if [[ ! -d "$WORKSPACE/platform/mac/build/Release/CoronaBuilder.app" ]]
then
	xcodebuild -project "$WORKSPACE/platform/mac/CoronaBuilder.xcodeproj" -target CoronaBuilder -configuration "$CONFIG" clean
	xcodebuild -project "$WORKSPACE/platform/mac/ratatouille.xcodeproj" -target CoronaCards -configuration "$CONFIG"
	xcodebuild -project "$WORKSPACE/platform/mac/CoronaBuilder.xcodeproj" -target CoronaBuilder -configuration "$CONFIG"
fi

cp -Rv "$WORKSPACE/platform/mac/build/Release/CoronaBuilder.app" "$MAC_BIN_DIR"
"$MAC_BIN_DIR/CoronaBuilder.app/Contents/MacOS/CoronaBuilder" version > "$CORONA_DIR/BUILD"
chmod -w "$CORONA_DIR/BUILD"

cp -v "$WORKSPACE/bin/mac/lfs.so" "$MAC_BIN_DIR"
cp -v "$WORKSPACE/platform/mac/build/$CONFIG/lstrip" "$MAC_BIN_DIR"
cp -v "$WORKSPACE/platform/mac/build/$CONFIG/lua" "$MAC_BIN_DIR"
cp -v "$WORKSPACE/platform/mac/build/$CONFIG/luac" "$MAC_BIN_DIR"

cp -v "$WORKSPACE/platform/resources/CoronaPListSupport.lua" "$CORONA_DIR/shared/resource"
cp -v "$WORKSPACE/platform/resources/dkjson.lua" "$CORONA_DIR/shared/resource"
cp -v "$WORKSPACE/platform/resources/json.lua" "$CORONA_DIR/shared/resource"
cp -v "$WORKSPACE/sdk/dmg/Corona3rdPartyLicenses.txt" "$CORONA_DIR/shared/resource"

(
	cd "$WORKSPACE/platform/android"
	./gradlew clean installAppTemplateAndAARToSim -Psolar2DBuildToolsOutputDir="$CORONA_DIR"
)

mkdir -p "$WORKSPACE/output"
COPYFILE_DISABLE=1 tar -C "$STAGING_DIR" -czvf "$WORKSPACE/output/Native.tar.gz" Native

#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build dist
export CLANG_MODULE_CACHE_PATH="$PWD/build/ModuleCache"
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
source scripts/debug-objects.sh
ng_build_debug_objects iphoneos "arm64" -miphoneos-version-min=15.0 device
xcrun --sdk iphoneos clang \
  -arch arm64 \
  -isysroot "$SDK" \
  -miphoneos-version-min=15.0 \
  -fobjc-arc \
  -fmodules \
  -fvisibility=hidden \
  -Os \
  -Wall \
  -Wextra \
  -Wno-unused-parameter \
  -Werror=implicit-function-declaration \
  -Werror=unguarded-availability -Werror=unguarded-availability-new \
  -dynamiclib \
  src/NaiwaGuo.m \
  "${NG_DEBUG_OBJECTS[@]}" \
  -framework AudioToolbox -framework AVFAudio -framework AVFoundation -framework CoreMedia -framework MediaToolbox -framework Accelerate -lc++ \
  -framework CoreText \
  -framework UniformTypeIdentifiers \
  -framework UIKit \
  -framework PhotosUI \
  -framework Foundation \
  -framework QuartzCore \
  -lz \
  -install_name @rpath/NaiwaGuo.dylib \
  -o dist/NaiwaGuo.dylib
cp dist/NaiwaGuo.dylib build/NaiwaGuo-unstripped.dylib
xcrun strip -S -x dist/NaiwaGuo.dylib
codesign --force --sign - dist/NaiwaGuo.dylib
codesign --verify --verbose dist/NaiwaGuo.dylib
cp NaiwaGuo.plist dist/NaiwaGuo.plist
python3 scripts/package.py
VERSION=$(awk '/^Version:/ {print $2}' control)
shasum -a 256 dist/NaiwaGuo.dylib dist/NaiwaGuo.plist "dist/com.naiwaguo.native_${VERSION}_iphoneos-arm.deb" "dist/com.naiwaguo.native_${VERSION}_iphoneos-arm64.deb" > dist/SHA256SUMS.txt

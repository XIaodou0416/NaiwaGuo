#!/bin/bash
# Sourced by both builds. Compile Objective-C++ separately with C++17.
ng_build_debug_objects() {
  local sdk_name="$1" arch="$2" deployment="$3" variant="$4" file object
  local sdk_path
  sdk_path=$(xcrun --sdk "$sdk_name" --show-sdk-path)
  NG_DEBUG_OBJECTS=()
  mkdir -p "build/debug-$variant"
  for file in src/Debug/*.m src/Debug/*.mm src/Debug/*.c; do
    object="build/debug-$variant/$(basename "$file").o"
    local flags=(-arch "$arch" -isysroot "$sdk_path" "$deployment" -fvisibility=hidden -Os -Wall -Wextra -Wno-unused-parameter -Werror=unguarded-availability -Werror=unguarded-availability-new -Isrc/Debug)
    case "$file" in
      *.m) flags+=(-fobjc-arc -fmodules);;
      *.mm) flags+=(-fobjc-arc -fmodules -std=c++17);;
      *.c) flags+=(-std=c11);;
    esac
    xcrun --sdk "$sdk_name" clang "${flags[@]}" -c "$file" -o "$object"
    NG_DEBUG_OBJECTS+=("$object")
  done
}

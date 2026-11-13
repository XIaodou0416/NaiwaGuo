#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
APP="build/NGLocalPreferencesTests.app"
mkdir -p "$APP/Contents/MacOS"
python3 - "$APP/Contents/Info.plist" <<'PY'
import plistlib, sys, uuid
with open(sys.argv[1], 'wb') as stream:
    plistlib.dump({
        'CFBundleIdentifier': 'com.naiwaguo.preferences-test.' + uuid.uuid4().hex,
        'CFBundleExecutable': 'NGLocalPreferencesTests',
        'CFBundlePackageType': 'APPL',
    }, stream)
PY
xcrun --sdk macosx clang -fobjc-arc -Wall -Wextra -Werror \
  tests/NGLocalPreferencesTests.m -framework Foundation \
  -o "$APP/Contents/MacOS/NGLocalPreferencesTests"
"$APP/Contents/MacOS/NGLocalPreferencesTests"

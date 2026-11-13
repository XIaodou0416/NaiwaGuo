#!/bin/bash
set -euo pipefail

# Fail before compiling when only Command Line Tools are selected. The project
# needs Apple's iOS SDKs and simulator runtimes, which are shipped with the
# full Xcode application.
ng_require_xcode() {
  if ! command -v xcodebuild >/dev/null 2>&1 || ! xcodebuild -version >/dev/null 2>&1; then
    cat >&2 <<'EOF'
NaiwaGuo requires the full Xcode application with iOS SDKs.
Install Xcode, launch it once to finish setup, then select it with:
  sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
EOF
    return 2
  fi
}

ng_sdk_path() {
  local sdk="$1"
  local path
  if ! path=$(xcrun --sdk "$sdk" --show-sdk-path 2>/dev/null) || [[ ! -d "$path" ]]; then
    echo "NaiwaGuo could not locate the $sdk SDK. Install a matching Xcode SDK." >&2
    return 2
  fi
  printf '%s\n' "$path"
}

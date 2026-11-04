#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=docs/previews
mkdir -p "$OUT"
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in home-light home-dark dock-settings dock-sizes fonts icons community community-dark support-light support-dark; do
  xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
  test ! -f "$CONTAINER/Documents/preview.png" || unlink "$CONTAINER/Documents/preview.png"
  case "$mode" in
    home-light) flags=(--preview-settings);;
    home-dark) flags=(--preview-settings --dark);;
    dock-settings) flags=(--preview-settings --dock-settings);;
    dock-sizes) flags=(--preview-settings --dock-settings --scroll-bottom);;
    fonts) flags=(--preview-fonts);;
    icons) flags=(--preview-icons);;
    community) flags=(--preview-community);;
    community-dark) flags=(--preview-community --dark);;
    support-light) flags=(--preview-support);;
    support-dark) flags=(--preview-support --dark);;
  esac
  xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "${flags[@]}" >/dev/null
  for attempt in $(seq 1 100); do test ! -f "$CONTAINER/Documents/preview.png" || break; sleep 0.1; done
  test -f "$CONTAINER/Documents/preview.png"
  sips -Z 1300 "$CONTAINER/Documents/preview.png" --out "$OUT/$mode.png" >/dev/null
done
for pair in "regular dock-regular" "clear dock-clear" "left dock-left" "right dock-right"; do
  read -r mode name <<< "$pair"
  sips -Z 1300 "dist/simulator-validation/$mode-smoke-003.png" --out "$OUT/$name.png" >/dev/null
done
sips -Z 1300 dist/simulator-validation/debug-menu-003.png --out "$OUT/debug-menu.png" >/dev/null

#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
if [[ -n "$(printenv NG_SIM_DEVICE)" ]]; then DEVICE="$NG_SIM_DEVICE"; fi
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in fonts icons visibility fullscreen clear qq cancel empty; do
  xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
  for name in search-navigation.json search-navigation.png; do
    test ! -f "$CONTAINER/Documents/$name" || unlink "$CONTAINER/Documents/$name"
  done
  xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-search --search-case "$mode" >/dev/null
  for attempt in $(seq 1 120); do
    if [[ -f "$CONTAINER/Documents/search-navigation.png" ]]; then break; fi
    sleep 0.1
  done
  test -f "$CONTAINER/Documents/search-navigation.png"
  cp "$CONTAINER/Documents/search-navigation.json" "$OUT/search-$mode-002.json"
  cp "$CONTAINER/Documents/search-navigation.png" "$OUT/search-$mode-002.png"
  python3 - "$OUT/search-$mode-002.json" <<'PY'
import json,sys
r=json.load(open(sys.argv[1])); print(json.dumps(r,ensure_ascii=False)); assert r['passed'],r
PY
done

#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in font regular clear dark left right center avatar avatar60 avatar100 search legacy; do
 xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
 if [[ "$mode" == font ]]; then NAME=font-stress-003; else NAME=dock-native-003; fi
 for ext in json png; do test ! -f "$CONTAINER/Documents/$NAME.$ext" || unlink "$CONTAINER/Documents/$NAME.$ext"; done
 case "$mode" in
 font) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --stress-font003 >/dev/null;;
 regular) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 >/dev/null;;
 clear) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear >/dev/null;;
 dark) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --dark >/dev/null;;
 left) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-left >/dev/null;;
 right) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-right >/dev/null;;
 center) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-center >/dev/null;;
 avatar) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-center --account-avatar >/dev/null;;
 legacy) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --legacy-dock006 >/dev/null;;
 avatar60) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-center --account-avatar --avatar-scale-60 >/dev/null;;
 avatar100) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --clear --collapse-center --account-avatar --avatar-scale-100 >/dev/null;;
 search) xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --preview-dock003 --search-native >/dev/null;;
 esac
 for attempt in $(seq 1 200); do test ! -f "$CONTAINER/Documents/$NAME.json" || break; sleep 0.1; done
 test -f "$CONTAINER/Documents/$NAME.json"
 cp "$CONTAINER/Documents/$NAME.json" "$OUT/$mode-smoke-003.json"
 cp "$CONTAINER/Documents/$NAME.png" "$OUT/$mode-smoke-003.png"
 if [[ "$mode" != font ]]; then cp "$CONTAINER/Documents/dock-expanded-003.png" "$OUT/$mode-expanded-003.png"; fi
 python3 - "$OUT/$mode-smoke-003.json" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]));print(json.dumps(r,ensure_ascii=False));assert r['passed'],r
PY
done

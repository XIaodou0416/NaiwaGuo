#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in settings-preview009 native-entry009 sponsor-never-seed009 sponsor-never-check009 unrestricted-restart009; do
 xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
 test ! -f "$CONTAINER/Documents/$mode.json" || unlink "$CONTAINER/Documents/$mode.json"
 xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "--$mode" >/dev/null
 for attempt in $(seq 1 150); do test ! -f "$CONTAINER/Documents/$mode.json" || break; sleep .1; done
 cp "$CONTAINER/Documents/$mode.json" "$OUT/"
 python3 - "$OUT/$mode.json" <<'PY'
import json,sys
r=json.load(open(sys.argv[1]));print(json.dumps(r,ensure_ascii=False));assert r['passed'],r
PY
done
for name in settings-unrestricted009 sponsor-prompt009 native-entry009; do cp "$CONTAINER/Documents/$name.png" "$OUT/"; done

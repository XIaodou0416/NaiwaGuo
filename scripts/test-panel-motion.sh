#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
for ext in json png; do test ! -f "$CONTAINER/Documents/panel-motion-005.$ext" || unlink "$CONTAINER/Documents/panel-motion-005.$ext"; done
xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --panel-motion005 >/dev/null
for attempt in $(seq 1 100); do test ! -f "$CONTAINER/Documents/panel-motion-005.json" || break; sleep 0.1; done
cp "$CONTAINER/Documents/"panel-motion-005.{json,png} dist/simulator-validation/
python3 - <<'PY'
import json
r=json.load(open('dist/simulator-validation/panel-motion-005.json'));print(json.dumps(r));assert r['passed'],r
PY

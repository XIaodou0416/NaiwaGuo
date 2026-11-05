#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in blocked-seed011 blocked-restart011 gesture011; do
 xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
 test ! -f "$CONTAINER/Documents/$mode.json" || unlink "$CONTAINER/Documents/$mode.json"
 xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "--$mode" >/dev/null
 for attempt in $(seq 1 100); do test ! -f "$CONTAINER/Documents/$mode.json" || break; sleep .1; done
 sleep .3
 cp "$CONTAINER/Documents/$mode.json" dist/simulator-validation/
 if [ "$mode" = blocked-seed011 ]; then xcrun simctl io "$DEVICE" screenshot dist/simulator-validation/blocked-panel011.png >/dev/null 2>&1; fi
done
cp "$CONTAINER/Documents/unblocked011.json" dist/simulator-validation/
python3 - <<'PY'
import json
for name in ['blocked-seed011','blocked-restart011','unblocked011','gesture011']:
 result=json.load(open('dist/simulator-validation/'+name+'.json'))
 assert result['passed'],result
 print(name+': passed')
PY

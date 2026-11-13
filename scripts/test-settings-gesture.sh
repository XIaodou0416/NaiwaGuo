#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in gesture011; do
 xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
 test ! -f "$CONTAINER/Documents/$mode.json" || unlink "$CONTAINER/Documents/$mode.json"
 xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "--$mode" >/dev/null
 for attempt in $(seq 1 100); do test ! -f "$CONTAINER/Documents/$mode.json" || break; sleep .1; done
 sleep .3
 cp "$CONTAINER/Documents/$mode.json" dist/simulator-validation/
done
python3 - <<'PY'
import json
for name in ['gesture011']:
 result=json.load(open('dist/simulator-validation/'+name+'.json'))
 assert result['passed'],result
 print(name+': passed')
PY

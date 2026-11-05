#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
test ! -f "$CONTAINER/Documents/usage-live010.json" || unlink "$CONTAINER/Documents/usage-live010.json"
xcrun simctl launch "$DEVICE" com.naiwaguo.testhost --usage-live010 >/dev/null
for attempt in $(seq 1 300); do test ! -f "$CONTAINER/Documents/usage-live010.json" || break; sleep .1; done
cp "$CONTAINER/Documents/usage-live010.json" dist/simulator-validation/
python3 - <<'PY'
import json
result=json.load(open('dist/simulator-validation/usage-live010.json'))
print(json.dumps(result,ensure_ascii=False));assert result['passed'],result
PY

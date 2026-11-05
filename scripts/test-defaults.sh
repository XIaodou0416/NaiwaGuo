#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in fresh retained; do
  xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
  FILE="$mode-defaults-005.json"
  test ! -f "$CONTAINER/Documents/$FILE" || unlink "$CONTAINER/Documents/$FILE"
  xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "--$mode-defaults005" >/dev/null
  for attempt in $(seq 1 100); do test ! -f "$CONTAINER/Documents/$FILE" || break; sleep 0.1; done
  cp "$CONTAINER/Documents/$FILE" "$OUT/$FILE"
  python3 - "$OUT/$FILE" <<'PY'
import json,sys
r=json.load(open(sys.argv[1])); print(json.dumps(r,ensure_ascii=False)); assert r['passed'],r
PY
done

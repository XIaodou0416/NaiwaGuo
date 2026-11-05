#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in access-preview007 sponsor-never-seed007 sponsor-never-check007 access-restart007 author-self008 author-self-restart008; do
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
for name in author-follow-prompt007 sponsor-prompt007 author-locked-settings007 author-unlocked-settings007 author-self-settings008; do cp "$CONTAINER/Documents/$name.png" "$OUT/"; done

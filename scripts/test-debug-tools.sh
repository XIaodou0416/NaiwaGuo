#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE=$(cat build/simulator-device.txt)
OUT=dist/simulator-validation
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for mode in debug-page-003 debug-audio-003 debug-ui-003 debug-late-003; do
  xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
  for ext in json zip; do test ! -f "$CONTAINER/Documents/$mode.$ext" || unlink "$CONTAINER/Documents/$mode.$ext"; done
  xcrun simctl launch "$DEVICE" com.naiwaguo.testhost "--$mode" >/dev/null
  for attempt in $(seq 1 400); do test ! -f "$CONTAINER/Documents/$mode.json" || break; sleep 0.1; done
  test -f "$CONTAINER/Documents/$mode.json"
  cp "$CONTAINER/Documents/$mode.json" "$OUT/"
  if [[ -f "$CONTAINER/Documents/$mode.zip" ]]; then cp "$CONTAINER/Documents/$mode.zip" "$OUT/"; fi
  python3 - "$OUT/$mode.json" <<'PY'
import json,sys,zipfile,struct
from pathlib import Path
p=Path(sys.argv[1]); r=json.loads(p.read_text()); print(json.dumps(r)); assert r['passed'],r
if p.with_suffix('.zip').exists():
    with zipfile.ZipFile(p.with_suffix('.zip')) as z:
        assert z.testzip() is None
        paths={n:n for n in z.namelist()}
        required={'manifest.json','ui/screenshot.png','probe/tabbar.txt','README.txt'}
        if 'page-' in p.name:
            required|={'page/windows.json','page/view-tree.txt','page/view-tree.json','page/layers.json','page/selected-view.json','page/view-controllers.txt'}
            assert any(n.startswith('page/classes/') and n.endswith('.h') for n in paths)
        else:
            required|={'audio/session.json','audio/backends.json','audio/timeline.jsonl','audio/signal.jsonl','audio/media-snapshots.json','audio/glass-target.json','audio/diagnostics.json','probe/audio-summary.txt'}
            waves=[n for n in paths if n.endswith('.wav')]
            assert waves
            for n in waves:
                data=z.read(paths[n]); assert data[:4]==b'RIFF' and data[8:12]==b'WAVE'
                assert len(data)>48000*4*2
        assert required<=paths.keys(),required-paths.keys()
PY
done
cp "$CONTAINER/Documents/debug-menu-003.png" "$OUT/debug-menu-003.png"

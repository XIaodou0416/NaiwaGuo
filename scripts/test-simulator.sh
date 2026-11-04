#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
DEVICE="${NG_SIM_DEVICE:-}"
IPA="${NG_TEST_IPA:-}"
APP="build/NaiwaGuoTestHost.app"
RESULT_DIR="dist/simulator-validation"
SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
ARCH=$(uname -m)
mkdir -p "$APP" "$RESULT_DIR"
if [[ -z "$DEVICE" ]]; then
  DEVICE=$(xcrun simctl list devices available -j | python3 -c 'import json,sys; ds=[d for group in json.load(sys.stdin)["devices"].values() for d in group if "iPhone" in d["name"]]; ds.sort(key=lambda d:d["state"]!="Booted"); print(ds[0]["udid"] if ds else "")')
fi
if [[ -z "$DEVICE" ]]; then echo "Create an iPhone simulator in Xcode, or set NG_SIM_DEVICE." >&2; exit 2; fi
printf '%s\n' "$DEVICE" > build/simulator-device.txt
if [[ -n "$IPA" ]]; then
  test -f "$IPA"
  unzip -jo "$IPA" 'Payload/Eggplant.app/AppIcon60x60@2x.png' -d "$APP" >/dev/null
else
  cp tests/assets/TestHostIcon.png "$APP/AppIcon60x60@2x.png"
fi
python3 - <<'CLEAN'
from pathlib import Path
for p in Path('dist/simulator-validation').iterdir():
    if p.suffix in ('.json', '.png', '.zip'): p.unlink()
CLEAN
export CLANG_MODULE_CACHE_PATH="$PWD/build/ModuleCache"
source scripts/debug-objects.sh
ng_build_debug_objects iphonesimulator "$ARCH" -mios-simulator-version-min=15.0 simulator
xcrun --sdk iphonesimulator clang \
  -arch "$ARCH" \
  -isysroot "$SDK" \
  -mios-simulator-version-min=15.0 \
  -fobjc-arc \
  -fmodules \
  -Wall \
  -Wextra \
  -Wno-unused-parameter \
  -Werror=implicit-function-declaration \
  tests/NaiwaGuoTestHost.m \
  "${NG_DEBUG_OBJECTS[@]}" \
  -framework AudioToolbox -framework AVFAudio -framework AVFoundation -framework CoreMedia -framework MediaToolbox -framework Accelerate -lc++ \
  -framework CoreText \
  -framework UniformTypeIdentifiers \
  -framework UIKit \
  -framework AVFoundation \
  -framework PhotosUI \
  -framework Foundation \
  -framework QuartzCore \
  -lz \
  -o "$APP/Eggplant"
python3 - <<'PY'
import plistlib
path = 'build/NaiwaGuoTestHost.app/Info.plist'
data = {
    'CFBundleExecutable': 'Eggplant',
    'CFBundleIdentifier': 'com.naiwaguo.testhost',
    'CFBundleName': 'NaiwaGuo Test Host',
    'CFBundleDisplayName': '红果短剧测试宿主',
    'CFBundlePackageType': 'APPL',
    'CFBundleIcons': {'CFBundlePrimaryIcon': {'CFBundleIconFiles': ['AppIcon60x60']}},
    'CFBundleVersion': '1',
    'CFBundleShortVersionString': '7.4.3',
    'MinimumOSVersion': '15.0',
    'UIDesignRequiresCompatibility': False,
    'LSRequiresIPhoneOS': True,
    'UIDeviceFamily': [1],
    'UILaunchScreen': {},
    'UIApplicationSceneManifest': {
        'UIApplicationSupportsMultipleScenes': False,
        'UISceneConfigurations': {
            'UIWindowSceneSessionRoleApplication': [
                {
                    'UISceneConfigurationName': 'Default',
                    'UISceneDelegateClassName': 'NGTestSceneDelegate',
                }
            ]
        },
    },
}
with open(path, 'wb') as stream:
    plistlib.dump(data, stream)
PY
codesign --force --sign - "$APP"
xcrun simctl boot "$DEVICE" >/dev/null 2>&1 || true
xcrun simctl bootstatus "$DEVICE" -b
xcrun simctl terminate "$DEVICE" com.naiwaguo.testhost >/dev/null 2>&1 || true
xcrun simctl install "$DEVICE" "$APP"
CONTAINER=$(xcrun simctl get_app_container "$DEVICE" com.naiwaguo.testhost data)
for name in result.json NaiwaGuo-capture-test.zip playback.png; do
  if [[ -f "$CONTAINER/Documents/$name" ]]; then unlink "$CONTAINER/Documents/$name"; fi
done
xcrun simctl launch "$DEVICE" com.naiwaguo.testhost
for _ in $(seq 1 60); do
  if [[ -f "$CONTAINER/Documents/result.json" ]]; then break; fi
  sleep 0.25
done
test -f "$CONTAINER/Documents/result.json"
xcrun simctl io "$DEVICE" screenshot "$RESULT_DIR/settings.png"
cp "$CONTAINER/Documents/result.json" "$RESULT_DIR/result.json"
cp "$CONTAINER/Documents/NaiwaGuo-capture-test.zip" "$RESULT_DIR/NaiwaGuo-capture-test.zip"
cp "$CONTAINER/Documents/playback.png" "$RESULT_DIR/playback.png"
cp "$CONTAINER/Documents/dock.png" "$RESULT_DIR/dock.png"
cp "$CONTAINER/Documents/header.png" "$RESULT_DIR/header.png"
for name in fullscreen-before.png fullscreen-after.png fullscreen-portrait.png mall-underlay-075.png mine-underlay-075.png; do
  cp "$CONTAINER/Documents/$name" "$RESULT_DIR/$name"
done
python3 - <<'PY'
import json
import pathlib
import struct
import zipfile
root = pathlib.Path('dist/simulator-validation')
result = json.loads((root / 'result.json').read_text())
assert all(result['nativeSettings'].values()), result['nativeSettings']
assert all(result['dock'].values()), result['dock']
for feature in ['episodeToolbar', 'dockColors', 'statusBarRouting', 'morePanelGlass', 'dramaTitle', 'shareIconReuse', 'thumbnailRepaint', 'dockAspectAndPreview', 'openingAnimation', 'settings001', 'corners002', 'shareRepaint002', 'typography002', 'search002', 'typography003', 'corners003', 'iconStates003', 'dock003', 'fullscreenPlayback', 'collection075', 'review075', 'underlay075', 'settings075', 'backdrop075']:
    assert all(result[feature].values()), result[feature]
assert all(result['playbackPresets'].values()), result['playbackPresets']
assert all(result['commentGlass'].values()), result['commentGlass']
assert all(result['shareGlass'].values()), result['shareGlass']
assert all(result['headerRestore'].values()), result['headerRestore']
assert result['fullscreenPreview']['fullscreenSettingPresent'], result['fullscreenPreview']
assert result['fullscreenPreview']['geometryUnchanged'], result['fullscreenPreview']
assert result['fullscreenPreview']['statusPlaybackDetected'], result['fullscreenPreview']
assert result['fullscreenPreview']['previewPresented'] and result['fullscreenPreview']['previewDismissed'], result['fullscreenPreview']
assert result['fullscreenPreview']['previewDragHandle'] and result['fullscreenPreview']['previewMovesAndPersists'], result['fullscreenPreview']
assert result['fullscreenPreview']['previewSliderCount'] == 15
assert all(result['playbackRail'].values()), result['playbackRail']
assert all(result['legacyXStyleRemoved'].values()), result['legacyXStyleRemoved']
assert all(result['photoIcons'].values()), result['photoIcons']
assert all(result['commentNativeGeometry'].values()), result['commentNativeGeometry']
assert all(result['playbackText'].values()), result['playbackText']
assert all(result['playbackHeaderGlass'].values()), result['playbackHeaderGlass']
assert result['overlayCreated'] and result['overlayVisible']  # DYKiller-style wrench remains global, including Settings.
assert result['settingsVisible']
assert result['archiveCopied']
assert result['viewCount'] >= 4
assert result['windowCount'] >= 1
assert result['eventCount'] > 0
assert result['performanceSamples'] >= 1
assert result['settingsGroups'] == 5
assert result['playbackGlassCount'] == 5
assert result['labelsOutsideGlass']
assert result['compatibilityDiskUnchanged']
assert result['runtimeCompatibilityFalse']
assert result['otherBundleUnchanged']
assert result['otherHostKeysUnchanged']
assert result['toggleRestores']
assert result['originalHitPreserved']
assert result['originalFramePreserved']
with zipfile.ZipFile(root / 'NaiwaGuo-capture-test.zip') as archive:
    assert archive.testzip() is None
    expected = {'NaiwaGuo-diagnostics.json', 'debug-settings.json', 'events.json', 'view-hierarchy.txt', 'page.png', 'README.txt'}
    assert expected <= set(archive.namelist())
    png = archive.read('page.png')
    assert png.startswith(b'\x89PNG\r\n\x1a\n')
    width, height = struct.unpack('>II', png[16:24])
    assert width > 300 and height > 600
    report = json.loads(archive.read('NaiwaGuo-diagnostics.json'))
    assert report['plugin'] == 'NaiwaGuo'
    assert report['viewsCount'] > 5
    assert isinstance(report['hitTests'], list)
print(json.dumps({'status': 'passed', 'checks': result, 'png': f'{width}x{height}'}, ensure_ascii=False, indent=2))
PY

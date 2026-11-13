"""Portable release checks; these do not replace iOS runtime tests."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]


class PublicSourceTests(unittest.TestCase):
    def test_no_service_implementation_or_network_transport(self):
        self.assertFalse((ROOT / 'backend').exists())
        self.assertFalse((ROOT / 'src/NGUsage.inc').exists())
        self.assertFalse((ROOT / 'scripts/test-usage-live.sh').exists())
        source = '\n'.join(p.read_text() for p in (ROOT / 'src').rglob('*') if p.is_file())
        for symbol in ('NGUsage', 'NGBlocked', 'NGPluginIsBlocked', 'NGHongguoUserID',
                       'NSURLSession', 'NSURLConnection', 'dataTaskWithRequest'):
            with self.subTest(symbol=symbol):
                self.assertNotIn(symbol, source)
        # External links are explicitly opened community links, not telemetry.
        urls = re.findall(r'https?://[^"\s]+', source)
        self.assertEqual(set(urls), {'https://qm.qq.com/q/w9rb2DnAoE',
                                    'https://t.me/beans0416', 'https://t.me/beans1011'})

    def test_version_and_public_documentation(self):
        version = re.search(r'^Version: (.+)$', (ROOT / 'control').read_text(), re.M).group(1)
        self.assertEqual(version, '0.1.1')
        self.assertIn(f'NGVersion = @"{version}"', (ROOT / 'src/NaiwaGuo.m').read_text())
        self.assertIn(f'NGD_VERSION @"{version}"', (ROOT / 'src/Debug/NGDKeys.h').read_text())
        self.assertEqual((ROOT / 'CONTRIBUTING.md').read_text(), '')
        for name in ('README.md', 'README-中文.md'):
            text = (ROOT / name).read_text()
            self.assertNotIn('## 0.1.1 更新', text)
            self.assertNotIn('仓库只用于私人维护', text)
            self.assertNotIn('backend/README.md', text)
            self.assertIn('0.1.1', text)

    def test_relative_source_includes_exist(self):
        for directory in ('src', 'tests'):
            for path in (ROOT / directory).rglob('*'):
                if path.is_file() and path.suffix in ('.m', '.mm', '.h', '.c', '.inc'):
                    for name in re.findall(r'^\s*#(?:include|import)\s+"([^"]+)"', path.read_text(), re.M):
                        with self.subTest(path=path.relative_to(ROOT), include=name):
                            self.assertTrue((path.parent / name).is_file())

    def test_source_zip_backup_without_git_metadata(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        # This nested export must not inherit its parent repository's metadata.
        for commit in (None, 'a' * 40):
            with self.subTest(commit=commit), tempfile.TemporaryDirectory(dir=ROOT / 'build') as directory:
                export = Path(directory)
                for name in ('scripts', 'src', 'build', 'dist'):
                    (export / name).mkdir()
                (export / 'control').write_text('Version: 0.1.1\n')
                (export / 'src/local.m').write_text('/* local source */\n')
                (export / 'build/generated.dylib').write_bytes(b'not source')
                (export / '.env').write_text('FAKE_SECRET=exclude-me\n')
                shutil.copy2(ROOT / 'scripts/backup-source.py', export / 'scripts/backup-source.py')
                if commit:
                    (export / 'SOURCE-MANIFEST.json').write_text(json.dumps({'gitCommit': commit}))
                command = [sys.executable, str(export / 'scripts/backup-source.py')]
                result = json.loads(subprocess.check_output(command, cwd=export, text=True))
                archive = export / result['privateSourceBackup']
                original = archive.read_bytes()
                with zipfile.ZipFile(archive) as zipped:
                    self.assertIsNone(zipped.testzip())
                    manifest = json.loads(zipped.read('BACKUP-MANIFEST.json'))
                    self.assertEqual(manifest['gitCommit'], commit)
                    self.assertIsNone(manifest['workingTreeDirty'])
                    self.assertNotIn('.env', manifest['files'])
                    self.assertNotIn('build/generated.dylib', manifest['files'])
                    for name, digest in manifest['files'].items():
                        self.assertEqual(hashlib.sha256(zipped.read('NaiwaGuo-0.1.1-source/' + name)).hexdigest(), digest)
                subprocess.check_output(command, cwd=export)
                self.assertEqual(archive.read_bytes(), original)


if __name__ == '__main__':
    unittest.main()

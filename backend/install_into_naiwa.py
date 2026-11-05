"""Stage the additive integration; run existing server tests before --apply."""
import argparse
import datetime
import hashlib
from pathlib import Path
import shutil
import sqlite3


def integrate(root):
    server = (root / 'server.py').read_text()
    if 'from guo_backend import install as install_guo' not in server:
        marker = 'init();KEY='
        if server.count(marker) != 1:
            raise ValueError('Naiwa startup changed; review integration first')
        server = server.replace(marker, 'from guo_backend import install as install_guo\ninstall_guo(Handler,DATA,limited)\n' + marker)
    original = "'/admin/app.js','/admin/style.css'"
    if "'/admin/guo-admin.js'" not in server:
        if server.count(original) != 1:
            raise ValueError('Naiwa asset routing changed; review integration first')
        server = server.replace(original, original + ",'/admin/guo-admin.js'")
    index = (root / 'web/index.html').read_text()
    if '/admin/guo-admin.js' not in index:
        if index.count('</body>') != 1:
            raise ValueError('Naiwa admin markup changed')
        index = index.replace('</body>', '<script src="/admin/guo-admin.js"></script></body>')
    return server, index


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--expected-server-sha256')
    parser.add_argument('--data', type=Path)
    parser.add_argument('--backup-root', type=Path)
    args = parser.parse_args()
    root = args.root
    if args.expected_server_sha256 and hashlib.sha256((root / 'server.py').read_bytes()).hexdigest() != args.expected_server_sha256:
        raise ValueError('Server changed since inspection; nothing modified')
    server, index = integrate(root)
    compile(server, str(root / 'server.py'), 'exec')
    if not args.apply:
        print('Integration plan valid; no files changed')
        return
    if args.backup_root:
        backup = args.backup_root / ('guo-0.1.0-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
        backup.mkdir(parents=True, mode=0o700)
        shutil.copy2(root / 'server.py', backup / 'server.py')
        shutil.copy2(root / 'web/index.html', backup / 'index.html')
        for name, path in [('guo_backend.py', root / 'guo_backend.py'), ('guo-admin.js', root / 'web/guo-admin.js')]:
            if path.exists(): shutil.copy2(path, backup / name)
        if args.data:
            for name in ('naiwa.sqlite3', 'guo.sqlite3'):
                if (args.data / name).exists():
                    with sqlite3.connect('file:' + str(args.data / name) + '?mode=ro', uri=True) as source, sqlite3.connect(backup / name) as destination:
                        source.backup(destination)
        print('Rollback backup:', backup)
    source = Path(__file__).resolve().parent
    shutil.copy2(source / 'guo_backend.py', root / 'guo_backend.py')
    shutil.copy2(source / 'guo-admin.js', root / 'web/guo-admin.js')
    for path, content in [(root / 'server.py', server), (root / 'web/index.html', index)]:
        temporary = path.with_suffix(path.suffix + '.guo-pending')
        temporary.write_text(content)
        shutil.copymode(path, temporary)
        temporary.replace(path)
    print('Integration files installed; service restart is separate')


if __name__ == '__main__': main()

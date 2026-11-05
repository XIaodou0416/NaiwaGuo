"""Optional 奶蛙 usage statistics for the existing Naiwa admin service."""
import datetime
import hashlib
import hmac
import json
import re
import sqlite3
import time
from pathlib import Path
from urllib.parse import parse_qs, urlsplit


def install(handler, data_directory, limited):
    database = Path(data_directory) / 'guo.sqlite3'

    def connect():
        connection = sqlite3.connect(database, timeout=10)
        connection.row_factory = sqlite3.Row
        connection.execute('PRAGMA foreign_keys=ON')
        return connection

    with connect() as connection:
        connection.executescript('''PRAGMA journal_mode=WAL;
CREATE TABLE IF NOT EXISTS guo_installations(
 installation TEXT PRIMARY KEY, secret_hash TEXT NOT NULL, hongguo_id TEXT NOT NULL,
 plugin_version TEXT NOT NULL, host_version TEXT NOT NULL, system_version TEXT NOT NULL,
 first_seen INTEGER NOT NULL, last_seen INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS guo_identities(
 installation TEXT REFERENCES guo_installations(installation) ON DELETE CASCADE,
 hongguo_id TEXT NOT NULL, first_seen INTEGER NOT NULL, last_seen INTEGER NOT NULL,
 PRIMARY KEY(installation,hongguo_id));
CREATE TABLE IF NOT EXISTS guo_activity(
 day TEXT NOT NULL, installation TEXT REFERENCES guo_installations(installation) ON DELETE CASCADE,
 hongguo_id TEXT NOT NULL, PRIMARY KEY(day,installation,hongguo_id));
CREATE INDEX IF NOT EXISTS guo_seen ON guo_installations(last_seen);
CREATE INDEX IF NOT EXISTS guo_day ON guo_activity(day);
''')

    original_get, original_post = handler.get, handler.post

    def get(self):
        path = urlsplit(self.path).path
        if path not in ('/api/admin/guo/stats', '/api/admin/guo/users'):
            return original_get(self)
        if not self.session():
            return self.fail(401, '请先登录')
        now = int(time.time())
        today = datetime.datetime.fromtimestamp(now, datetime.timezone(datetime.timedelta(hours=8))).date()
        with connect() as connection:
            if path.endswith('/users'):
                query = parse_qs(urlsplit(self.path).query)
                search = query.get('q', [''])[0].strip()[:64]
                offset = max(0, min(100000, int(query.get('offset', ['0'])[0])))
                where = ' WHERE hongguo_id LIKE ?'
                args = ('%' + search + '%',)
                total = connection.execute('SELECT count(*) FROM guo_installations' + where, args).fetchone()[0]
                rows = connection.execute('SELECT installation,hongguo_id,plugin_version,host_version,system_version,first_seen,last_seen FROM guo_installations' + where + ' ORDER BY last_seen DESC LIMIT 100 OFFSET ?', args + (offset,))
                return self.reply({'items': [dict(row) for row in rows], 'total': total, 'offset': offset})

            def scalar(sql, args=()):
                return connection.execute(sql, args).fetchone()[0]

            result = {'at': now, 'installations': scalar('SELECT count(*) FROM guo_installations'), 'users': scalar('SELECT count(DISTINCT hongguo_id) FROM guo_identities'), 'activeToday': scalar('SELECT count(DISTINCT hongguo_id) FROM guo_activity WHERE day=? AND hongguo_id<>\'\'', (str(today),)), 'active7d': scalar('SELECT count(DISTINCT hongguo_id) FROM guo_activity WHERE day>=? AND hongguo_id<>\'\'', (str(today - datetime.timedelta(days=6)),)), 'onlineInstallations': scalar('SELECT count(*) FROM guo_installations WHERE last_seen>=?', (now - 600,))}
            result['days'] = []
            for age in range(13, -1, -1):
                day = str(today - datetime.timedelta(days=age))
                result['days'].append({'day': day, 'users': scalar('SELECT count(DISTINCT hongguo_id) FROM guo_activity WHERE day=? AND hongguo_id<>\'\'', (day,)), 'installations': scalar('SELECT count(DISTINCT installation) FROM guo_activity WHERE day=?', (day,))})
            result['versions'] = {column: [dict(row) for row in connection.execute(f'SELECT {column} AS version,count(*) AS count FROM guo_installations WHERE last_seen>=? GROUP BY {column} ORDER BY count DESC LIMIT 100', (now - 30 * 86400,))] for column in ('plugin_version', 'host_version', 'system_version')}
            return self.reply(result)

    def post(self):
        path = urlsplit(self.path).path
        if path not in ('/api/guo/v1/usage', '/api/guo/v1/forget'):
            return original_post(self)
        if limited('guo:' + self.client_ip(), 60, 60):
            return self.fail(429, '请稍后重试')
        if not 0 < int(self.headers.get('Content-Length', '0')) <= 4096:
            return self.fail(413, '请求过大或为空')
        body = self.body()
        installation, secret = body.get('installation'), body.get('secret')
        if not isinstance(installation, str) or not re.fullmatch(r'[A-Fa-f0-9-]{36}', installation) or not isinstance(secret, str) or not re.fullmatch(r'[A-Za-z0-9-]{64,100}', secret):
            return self.fail(400, '安装凭据无效')
        digest = hashlib.sha256(secret.encode()).hexdigest()
        now = int(time.time())
        with connect() as connection:
            connection.execute('BEGIN IMMEDIATE')
            current = connection.execute('SELECT secret_hash FROM guo_installations WHERE installation=?', (installation,)).fetchone()
            if current and not hmac.compare_digest(current['secret_hash'], digest):
                return self.fail(403, '安装凭据不匹配')
            if path.endswith('/forget'):
                connection.execute('DELETE FROM guo_installations WHERE installation=?', (installation,))
                connection.commit()
                return self.reply({'ok': True})
            uid = body.get('hongguo_id', '')
            versions = [body.get(key) for key in ('plugin_version', 'host_version', 'system_version')]
            if body.get('schema_version') != 1 or not isinstance(uid, str) or (uid and not re.fullmatch(r'[0-9]{1,20}', uid)) or any(not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9._+() -]{1,40}', value) for value in versions):
                return self.fail(400, '统计字段无效')
            connection.execute('INSERT INTO guo_installations VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(installation) DO UPDATE SET hongguo_id=excluded.hongguo_id,plugin_version=excluded.plugin_version,host_version=excluded.host_version,system_version=excluded.system_version,last_seen=excluded.last_seen', (installation, digest, uid, *versions, now, now))
            if uid:
                connection.execute('INSERT INTO guo_identities VALUES(?,?,?,?) ON CONFLICT(installation,hongguo_id) DO UPDATE SET last_seen=excluded.last_seen', (installation, uid, now, now))
            today = datetime.datetime.fromtimestamp(now, datetime.timezone(datetime.timedelta(hours=8))).date()
            connection.execute('INSERT OR IGNORE INTO guo_activity VALUES(?,?,?)', (str(today), installation, uid))
            connection.execute('DELETE FROM guo_activity WHERE day<?', (str(today - datetime.timedelta(days=90)),))
            connection.execute('DELETE FROM guo_installations WHERE last_seen<?', (now - 180 * 86400,))
        return self.reply({'ok': True})

    handler.get, handler.post = get, post
    return database

import json
import tempfile
import unittest
from pathlib import Path

from guo_backend import install


class UsageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()

        class Handler:
            def get(self): return 'existing-get'
            def post(self): return 'existing-post'
            def reply(self, value, status=200): self.result = (status, value)
            def fail(self, status, message): self.result = (status, message)
            def session(self): return {'csrf': 'fixture'} if self.authenticated else None
            def client_ip(self): return '127.0.0.1'
            def body(self): return self.payload

        self.handler = Handler
        self.db = install(Handler, Path(self.temp.name), lambda *args: False)
        self.body = {'installation': '00000000-0000-4000-8000-000000000001', 'secret': 'a' * 72, 'hongguo_id': '123456', 'plugin_version': '0.1.0', 'host_version': '7.4.3', 'system_version': '15.8', 'schema_version': 1}

    def tearDown(self): self.temp.cleanup()

    def request(self, path, body=None, authenticated=True, csrf=True, origin=True):
        target = self.handler(); target.path = path; target.authenticated = authenticated
        target.payload = self.body.copy() if body is None else body
        target.headers = {'Content-Length': str(len(json.dumps(target.payload)))}
        if csrf: target.headers['X-CSRF-Token'] = 'fixture'
        if origin: target.headers['Origin'] = 'https://naiwa.xxdou.icu'
        value = target.post() if path.endswith('/ban') or not path.startswith('/api/admin') else target.get()
        return getattr(target, 'result', value)

    def test_dedup_versions_and_admin_auth(self):
        self.assertEqual(self.request('/api/guo/v1/usage')[0], 200)
        self.request('/api/guo/v1/usage')
        second = {**self.body, 'installation': '00000000-0000-4000-8000-000000000002', 'system_version': '27.0'}
        self.request('/api/guo/v1/usage', second)
        stats = self.request('/api/admin/guo/stats')[1]
        self.assertEqual((stats['users'], stats['installations'], stats['activeToday']), (1, 2, 1))
        self.assertEqual(len(stats['versions']['system_version']), 2)
        self.assertEqual(self.request('/api/admin/guo/users', authenticated=False)[0], 401)
        self.assertEqual(self.request('/api/admin/guo/stats', authenticated=False)[0], 401)
        rows = self.request('/api/admin/guo/users')[1]['items']
        self.assertNotIn('secret_hash', rows[0])

    def test_no_login_and_search(self):
        self.request('/api/guo/v1/usage', {**self.body, 'hongguo_id': ''})
        stats = self.request('/api/admin/guo/stats')[1]
        self.assertEqual((stats['users'], stats['installations']), (0, 1))
        self.assertEqual(self.request('/api/admin/guo/users?q=999')[1]['total'], 0)

    def test_reject_invalid_fields_and_schema(self):
        for field, value in [('hongguo_id', "1' OR 1=1"), ('system_version', '<script>'), ('secret', 'short'), ('installation', 'not-uuid'), ('schema_version', 0)]:
            self.assertEqual(self.request('/api/guo/v1/usage', {**self.body, field: value})[0], 400)
        self.assertEqual(self.request('/api/admin/guo/stats')[1]['installations'], 0)

    def test_forget_auth_and_cascade(self):
        self.request('/api/guo/v1/usage')
        self.assertEqual(self.request('/api/guo/v1/usage', {**self.body, 'secret': 'b' * 72})[0], 403)
        self.assertEqual(self.request('/api/guo/v1/forget', {**self.body, 'secret': 'b' * 72})[0], 403)
        self.assertEqual(self.request('/api/guo/v1/forget')[0], 200)
        stats = self.request('/api/admin/guo/stats')[1]
        self.assertEqual((stats['users'], stats['installations'], stats['activeToday']), (0, 0, 0))
        self.assertEqual(self.request('/api/guo/v1/forget')[0], 200)

    def test_existing_routes_unchanged(self):
        self.assertEqual(self.request('/api/v1/check'), 'existing-post')
        self.assertEqual(self.request('/api/admin/stats'), 'existing-get')

    def test_ban_requires_session_csrf_and_origin(self):
        body = {'hongguo_id': '123456', 'blocked': True}
        self.assertEqual(self.request('/api/admin/guo/ban', body, authenticated=False)[0], 401)
        self.assertEqual(self.request('/api/admin/guo/ban', body, csrf=False)[0], 403)
        self.assertEqual(self.request('/api/admin/guo/ban', body, origin=False)[0], 403)
        for invalid in [{'hongguo_id': '', 'blocked': True}, {'hongguo_id': '123', 'blocked': 1}, {'hongguo_id': "1'", 'blocked': True}]:
            self.assertEqual(self.request('/api/admin/guo/ban', invalid)[0], 400)
        self.assertEqual(self.request('/api/admin/guo/bans')[1]['items'], [])

    def test_ban_unban_multiple_installations_and_forget(self):
        body = {'hongguo_id': '123456', 'blocked': True, 'note': '<test>'}
        self.assertEqual(self.request('/api/admin/guo/ban', body)[0], 200)
        self.assertTrue(self.request('/api/guo/v1/usage')[1]['blocked'])
        second = {**self.body, 'installation': '00000000-0000-4000-8000-000000000002'}
        self.assertTrue(self.request('/api/guo/v1/usage', second)[1]['blocked'])
        self.assertTrue(self.request('/api/admin/guo/users')[1]['items'][0]['blocked'])
        self.request('/api/guo/v1/forget')
        self.assertTrue(self.request('/api/guo/v1/usage')[1]['blocked'])
        self.assertFalse(self.request('/api/guo/v1/usage', {**self.body, 'hongguo_id': '999'})[1]['blocked'])
        self.assertFalse(self.request('/api/guo/v1/usage', {**self.body, 'hongguo_id': ''})[1]['blocked'])
        self.assertEqual(self.request('/api/admin/guo/stats')[1]['blockedUsers'], 1)
        self.request('/api/admin/guo/ban', {**body, 'blocked': False})
        self.assertFalse(self.request('/api/guo/v1/usage', second)[1]['blocked'])
        self.assertEqual(self.request('/api/admin/guo/bans')[1]['items'], [])


if __name__ == '__main__': unittest.main()

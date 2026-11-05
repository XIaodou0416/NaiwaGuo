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
            def session(self): return self.authenticated
            def client_ip(self): return '127.0.0.1'
            def body(self): return self.payload

        self.handler = Handler
        self.db = install(Handler, Path(self.temp.name), lambda *args: False)
        self.body = {'installation': '00000000-0000-4000-8000-000000000001', 'secret': 'a' * 72, 'hongguo_id': '123456', 'plugin_version': '0.1.0', 'host_version': '7.4.3', 'system_version': '15.8', 'schema_version': 1}

    def tearDown(self): self.temp.cleanup()

    def request(self, path, body=None, authenticated=True):
        target = self.handler(); target.path = path; target.authenticated = authenticated
        target.payload = self.body.copy() if body is None else body
        target.headers = {'Content-Length': str(len(json.dumps(target.payload)))}
        value = target.get() if path.startswith('/api/admin') else target.post()
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


if __name__ == '__main__': unittest.main()

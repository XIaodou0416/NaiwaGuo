import importlib.util
import json
import sqlite3
import unittest

support = None
if importlib.util.find_spec('test_server'):
    import test_server as support


@unittest.skipIf(support is None, 'Requires the existing Naiwa test_server.py fixture')
class GuoHTTP(unittest.TestCase):
    @classmethod
    def setUpClass(cls): support.API.setUpClass.__func__(cls)

    def setUp(self):
        support.API.setUp(self)
        self.login()
        with sqlite3.connect(support.s.DATA / 'guo.sqlite3') as connection:
            for table in ('guo_activity', 'guo_identities', 'guo_installations'):
                connection.execute('DELETE FROM ' + table)

    req = support.API.req if support else None
    login = support.API.login if support else None

    def test_usage_delete_and_authenticated_admin(self):
        body = {'installation': '00000000-0000-4000-8000-000000000010', 'secret': 'z' * 72, 'hongguo_id': '123456', 'plugin_version': '0.1.0', 'host_version': '7.4.3', 'system_version': '15.8', 'schema_version': 1}
        self.assertEqual(self.req('/api/guo/v1/usage', body, auth=False)[0], 200)
        self.assertEqual(self.req('/api/guo/v1/usage', body, auth=False)[0], 200)
        self.assertEqual(self.req('/api/admin/guo/stats', auth=False)[0], 401)
        stats = self.req('/api/admin/guo/stats')[1]
        self.assertEqual((stats['users'], stats['installations']), (1, 1))
        self.assertEqual(self.req('/api/admin/guo/users')[1]['items'][0]['host_version'], '7.4.3')
        self.assertEqual(self.req('/api/guo/v1/forget', {**body, 'secret': 'x' * 72}, auth=False)[0], 403)
        self.assertEqual(self.req('/api/guo/v1/forget', body, auth=False)[0], 200)
        self.assertEqual(self.req('/api/admin/guo/stats')[1]['installations'], 0)

    def test_oversize_and_missing_schema(self):
        self.assertEqual(self.req('/api/guo/v1/usage', {'text': 'x' * 5000}, auth=False)[0], 413)
        self.assertEqual(self.req('/api/guo/v1/usage', {}, auth=False)[0], 400)


if __name__ == '__main__': unittest.main()

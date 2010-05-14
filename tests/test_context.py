#!/usr/bin/env python

import unittest
import smbc
import settings

class TestContext(unittest.TestCase):
    def setUp(self):
        self.ctx = smbc.Context()

    def tearDown(self):
        del self.ctx

    def testWorkgroup(self):
        list = self.ctx.opendir('smb://').getdents()
        assert(len(list) > 0)
        for entry in list:
            assert(entry.smbc_type == 1)

    def testServer(self):
        uri = 'smb://' + settings.WORKGROUP
        list = self.ctx.opendir(uri).getdents()
        assert(len(list) > 0)
        for entry in list:
            assert(entry.smbc_type == 2)

    def testShare(self):
        uri = 'smb://' + settings.SERVER
        list = self.ctx.opendir(uri).getdents()
        assert(len(list) > 0)
        for entry in list:
            assert(entry.smbc_type == 3 or entry.smbc_type == 6)

if __name__ == '__main__':
    unittest.main()

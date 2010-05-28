#!/usr/bin/env python

import unittest
import smbc
import settings

class TestContext(unittest.TestCase):
    def setUp(self):
        self.ctx = smbc.Context()

    def tearDown(self):
        del self.ctx

#    def test000_Workgroup(self):
#        list = self.ctx.opendir('smb://').getdents()
#        assert(len(list) > 0)
#        for entry in list:
#            assert(entry.smbc_type == 1)

    def test001_Server(self):
        uri = 'smb://' + settings.WORKGROUP
        list = self.ctx.opendir(uri).getdents()
        assert(len(list) > 0)
        for entry in list:
            assert(entry.smbc_type == 2)

    def test002_Share(self):
        uri = 'smb://' + settings.SERVER
        list = self.ctx.opendir(uri).getdents()
        assert(len(list) > 0)
        for entry in list:
            assert(3 <= entry.smbc_type and entry.smbc_type <= 6)

#    def test003_Dir(self):
#        uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
#        list = self.ctx.opendir(uri).getdents()
#        print list
#        assert(len(list) > 0)
#        for entry in list:
#            print entry
#            assert(3 <= entry.smbc_type and entry.smbc_type <= 6)

if __name__ == '__main__':
    unittest.main()

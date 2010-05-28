#!/usr/bin/env python

import unittest
import smbc
import settings

def auth_fn(server, share, workgroup, username, password):
    return (workgroup, settings.USERNAME, settings.PASSWORD)

class TestDir(unittest.TestCase):
    def setUp(self):
        self.basedir = 'smb://' + settings.SERVER + '/' + settings.SHARE + '/'
        self.ctx = smbc.Context()
        self.ctx.optionNoAutoAnonymousLogin = True
        self.ctx.functionAuthData = auth_fn

    def tearDown(self):
        del self.ctx

    def test000_Mkdir(self):
        uri = self.basedir + 'pysmbctest'
        ret = self.ctx.mkdir(uri, 0)
        assert(ret == 0)

    def test001_MkdirFail(self):
        uri = self.basedir + 'pysmbctest'
        ret = self.ctx.mkdir(uri)
        assert(ret < 0)

    def test002_ListDir(self):
        uri = self.basedir + 'pysmbctest'
        list = self.ctx.opendir(uri).getdents()
        assert(len(list) == 2)

    def test003_Stat(self):
        uri = self.basedir + 'pysmbctest'
        st = self.ctx.stat(uri)
        import stat
        mode = st[stat.ST_MODE]
        assert(stat.S_ISDIR(mode))
        assert(stat.S_ISREG(mode) == False)

    def test004_Rename(self):
        src = self.basedir + 'pysmbctest/dir1'
        dst = self.basedir + 'pysmbctest/dir2'
        ret = self.ctx.mkdir(src)
        assert(ret == 0)
        ret = self.ctx.rename(src, dst)
        assert(ret == 0)

    def test005_Rmdir(self):
        uri = self.basedir + 'pysmbctest/dir2'
        ret = self.ctx.rmdir(uri)
        assert(ret == 0)

    def test999_Rmdir(self):
        uri = self.basedir + 'pysmbctest'
        ret = self.ctx.rmdir(uri)
        assert(ret == 0)

if __name__ == '__main__':
    unittest.main()

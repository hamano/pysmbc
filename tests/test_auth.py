#!/usr/bin/env python

import unittest
import smbc
import settings

class TestAuth(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testNoAutoAnonymousLogin(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        uri = 'smb://' + settings.SERVER
        try:
            dir = ctx.opendir(uri)
        except smbc.PermissionError:
            pass
        except:
            self.fail()
        else:
            self.fail()

    def callback(self, server, share, workgroup, username, password):
        return (workgroup, settings.USERNAME, settings.PASSWORD)

    def testAuth(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        cb = lambda se, sh, w, u, p: (w, settings.USERNAME, settings.PASSWORD)
        ctx.functionAuthData = cb
        uri = 'smb://' + settings.SERVER
        try:
            dir = ctx.opendir(uri)
        except:
            self.fail()

    def testAuthFail(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        cb = lambda se, sh, w, u, p: (w, settings.USERNAME, "")
        ctx.functionAuthData = cb
        uri = 'smb://' + settings.SERVER
        try:
            dir = ctx.opendir(uri)
        except smbc.PermissionError:
            pass
        except:
            self.fail()
        else:
            self.fail()

if __name__ == '__main__':
    unittest.main()

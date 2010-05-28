#!/usr/bin/env python

import unittest
import smbc
import settings
import sys

class TestAuth(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test000_AuthSuccess(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        cb = lambda se, sh, w, u, p: (w, settings.USERNAME, settings.PASSWORD)
        ctx.functionAuthData = cb
        uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
        try:
            dir = ctx.opendir(uri)
        except:
            self.fail()

    def test004_AuthFailNoauth(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
        try:
            dir = ctx.opendir(uri)
        except smbc.PermissionError:
            pass
        except:
            self.fail()
        else:
            self.fail()

    def test005_AuthFailNopass(self):
        ctx = smbc.Context()
        ctx.optionNoAutoAnonymousLogin = True
        cb = lambda se, sh, w, u, p: (w, settings.USERNAME, "")
        ctx.functionAuthData = cb
        uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
        try:
            dir = ctx.opendir(uri)
        except smbc.PermissionError:
            pass
        except:
            self.fail()
        else:
            self.fail()

    def test006_AuthFailNoname(self):
        ctx = smbc.Context(debug=1)
        ctx.optionNoAutoAnonymousLogin = True
        cb = lambda se, sh, w, u, p: (w, "", "")
        ctx.functionAuthData = cb
        uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
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

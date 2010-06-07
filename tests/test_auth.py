#!/usr/bin/env python

import smbc
import settings
import sys

def setUp():
    pass

def tearDown():
    pass

def test_AuthSuccess():
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, settings.USERNAME, settings.PASSWORD)
    ctx.functionAuthData = cb
    uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
    try:
        dir = ctx.opendir(uri)
    except:
        assert False

def test_AuthFailNoauth():
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
    try:
        dir = ctx.opendir(uri)
    except smbc.PermissionError:
        pass
    except:
        assert False
    else:
        assert False

def test_AuthFailNopass():
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
        assert False
    else:
        assert False

def test_AuthFailNoname():
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, "", "")
    ctx.functionAuthData = cb
    uri = 'smb://' + settings.SERVER + '/' + settings.SHARE
    try:
        dir = ctx.opendir(uri)
    except smbc.PermissionError:
        pass
    except:
        assert False
    else:
        assert False

if __name__ == '__main__':
    import nose
    result = nose.run()


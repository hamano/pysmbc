#!/usr/bin/env python

import unittest
import smbc
import settings
import stat

basedir = 'smb://' + settings.SERVER + '/' + settings.SHARE + '/'
testdir = basedir  + '/' + settings.TESTDIR

def auth_fn(server, share, workgroup, username, password):
    return (workgroup, settings.USERNAME, settings.PASSWORD)

def setUp():
    global ctx
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    ctx.functionAuthData = auth_fn

def tearDown():
    global ctx
    del ctx

def test_Mkdir():
    ret = ctx.mkdir(testdir, 0)
    assert(ret == 0)

def test_MkdirFail():
    print testdir
    try:
        ret = ctx.mkdir(testdir)
    except smbc.ExistsError:
        pass
    except:
        assert False
    else:
        assert False
    assert True

def test_ListDir():
    list = ctx.opendir(testdir).getdents()
    assert(len(list) == 2)

def test_Stat():
    st = ctx.stat(testdir)
    mode = st[stat.ST_MODE]
    assert(stat.S_ISDIR(mode))
    assert(stat.S_ISREG(mode) == False)

def test_Rename():
    src = testdir + '/dir1'
    dst = testdir + '/dir2'
    ret = ctx.mkdir(src)
    assert(ret == 0)
    ret = ctx.rename(src, dst)
    assert(ret == 0)

def test_StatFail():
    uri = testdir + '/dir1'
    try:
        ctx.stat(uri)
    except smbc.NoEntryError:
        pass
    except:
        assert False

def test_Rmdir():
    uri = testdir + '/dir2'
    ret = ctx.rmdir(uri)
    assert(ret == 0)

def test_Cleanup():
    ret = ctx.rmdir(testdir)
    assert(ret == 0)

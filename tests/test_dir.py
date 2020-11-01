#!/usr/bin/env python

import smbc
import stat
import pytest

@pytest.fixture()
def fixture(config):
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, config['username'], config['password'])
    ctx.functionAuthData = cb
    yield {
        'ctx': ctx
    }

def test_mkdir(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    ret = ctx.mkdir(testdir, 0)
    assert ret == 0

def test_mkdir_error_exists(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    try:
        ret = ctx.mkdir(testdir)
    except smbc.ExistsError:
        pass
    except:
        assert False
    else:
        assert False
    assert True

def test_list_dir(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    entries = ctx.opendir(testdir).getdents()
    assert len(entries) == 2

def test_stat(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    st = ctx.stat(testdir)
    mode = st[stat.ST_MODE]
    assert stat.S_ISDIR(mode) 
    assert stat.S_ISREG(mode) == False

def test_rename(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    src = testdir + 'dir1/'
    dst = testdir + 'dir2/'
    ret = ctx.mkdir(src)
    assert ret == 0
    ret = ctx.rename(src, dst)
    assert ret == 0

def test_stat_error_notfound(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    uri = testdir + 'dir1/'
    try:
        ctx.stat(uri)
    except smbc.NoEntryError:
        pass
    except:
        assert False

def test_cleanup(config, fixture):
    ctx = fixture['ctx']
    testdir = config['uri'] + 'test/'
    uri = testdir + '/dir2'
    ret = ctx.rmdir(uri)
    assert ret == 0
    ret = ctx.rmdir(testdir)
    assert ret == 0

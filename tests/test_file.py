#!/usr/bin/env python

import smbc
import settings
import hashlib
import os

basedir = 'smb://' + settings.SERVER + '/' + settings.SHARE + '/'
testdir = basedir  + '/' + settings.TESTDIR

def auth_fn(server, share, workgroup, username, password):
    return (workgroup, settings.USERNAME, settings.PASSWORD)

def creat_randfile(path, size):
    rand = open('/dev/urandom', 'rb')
    file = open(path, 'wb')
    buf = rand.read(size)
    file.write(buf)
    file.close()
    rand.close()

def md(path):
    m = hashlib.md5()
    file = open(path, 'rb')
    m.update(file.read())
    file.close()
    return m.hexdigest()

def upload_simple(spath, ctx, duri):
    sfile = open(spath, 'rb')
    dfile = ctx.open(duri, os.O_CREAT | os.O_TRUNC | os.O_WRONLY)
    ret = dfile.write(sfile.read())
    if ret < 0:
        raise IOError("smbc write error")
    sfile.close()
    dfile.close()
    return True

def download_simple(ctx, suri, dpath):
    sfile = ctx.open(suri, os.O_RDONLY)
    dfile = open(dpath, 'wb')
    dfile.write(sfile.read())
    dfile.flush()
    sfile.close()
    dfile.close()
    return True

def upload_buffer(spath, ctx, duri):
    sfile = open(spath, 'rb')
    dfile = ctx.open(duri, os.O_CREAT | os.O_TRUNC | os.O_WRONLY)
    while True:
        buf = sfile.read(8192)
        if not buf:
            break
        ret = dfile.write(buf)
        if ret < 0:
            raise IOError("smbc write error")
    sfile.close()
    dfile.close()
    return True

def download_buffer(ctx, suri, dpath):
    sfile = ctx.open(suri, os.O_RDONLY)
    dfile = open(dpath, 'wb')
    while True:
        buf = sfile.read(8192)
        if not buf:
            break
        dfile.write(buf)
    dfile.flush()
    sfile.close()
    dfile.close()
    return True

def upload_iter(spath, ctx, duri):
    sfile = open(spath, 'rb')
    dfile = ctx.open(duri, os.O_CREAT | os.O_TRUNC | os.O_WRONLY)
    for buf in sfile:
        ret = dfile.write(buf)
        if ret < 0:
            raise IOError("smbc write error")
    sfile.close()
    dfile.close()
    return True

def download_iter(ctx, suri, dpath):
    sfile = ctx.open(suri, os.O_RDONLY)
    dfile = open(dpath, 'wb')
    for buf in sfile:
        dfile.write(buf)
    dfile.flush()
    sfile.close()
    dfile.close()
    return True

def setUp():
    global ctx
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    ctx.functionAuthData = auth_fn

def tearDown():
    global ctx
    del ctx

def test_copy_simple_1k():
    creat_randfile('test1.dat', 1000)
    testfile = basedir + '/' + 'test1.dat'
    upload_simple('test1.dat', ctx, testfile)
    download_simple(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_simple_100k():
    creat_randfile('test1.dat', 10000)
    testfile = basedir + '/' + 'test1.dat'
    upload_simple('test1.dat', ctx, testfile)
    download_simple(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_simple_1M():
    creat_randfile('test1.dat', 1000000)
    testfile = basedir + '/' + 'test1.dat'
    upload_simple('test1.dat', ctx, testfile)
    download_simple(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_buffer_1k():
    creat_randfile('test1.dat', 1000)
    testfile = basedir + '/' + 'test1.dat'
    upload_buffer('test1.dat', ctx, testfile)
    download_buffer(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_buffer_100k():
    creat_randfile('test1.dat', 10000)
    testfile = basedir + '/' + 'test1.dat'
    upload_buffer('test1.dat', ctx, testfile)
    download_buffer(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_buffer_1M():
    creat_randfile('test1.dat', 1000000)
    testfile = basedir + '/' + 'test1.dat'
    upload_buffer('test1.dat', ctx, testfile)
    download_buffer(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_iter_1k():
    creat_randfile('test1.dat', 1000)
    testfile = basedir + '/' + 'test1.dat'
    upload_iter('test1.dat', ctx, testfile)
    download_iter(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_iter_100k():
    creat_randfile('test1.dat', 10000)
    testfile = basedir + '/' + 'test1.dat'
    upload_iter('test1.dat', ctx, testfile)
    download_iter(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

def test_copy_iter_1M():
    creat_randfile('test1.dat', 1000000)
    testfile = basedir + '/' + 'test1.dat'
    upload_iter('test1.dat', ctx, testfile)
    download_iter(ctx, testfile, 'test2.dat')
    assert(md('test1.dat') == md('test2.dat'))

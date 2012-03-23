#!/usr/bin/env python
import os
import smbc
import settings

baseurl =  'smb://' + settings.SERVER + "/" + settings.SHARE +"/"

def setUp():
    global ctx
    ctx = smbc.Context()
    cb = lambda se, sh, w, u, p: (w, settings.USERNAME, settings.PASSWORD)
    ctx.functionAuthData = cb


def tearDown():
    global ctx
    del ctx

def touch_file(name):
    """
    create a file containing "sample test file" in the test baseurl 
    """
    tmpfile_name = baseurl + name
    dfile = ctx.open(tmpfile_name, os.O_CREAT | os.O_TRUNC | os.O_WRONLY)
    dfile.write("sample test file")
    dfile.close
    return tmpfile_name
        
        
def test_xattr_constants():
    assert smbc.XATTR_ACL 
    assert smbc.XATTR_OWNER 
    assert smbc.XATTR_GROUP

    
def test_xattr_get():
    """
    system.nt_sec_desc.<attribute name>
 *                     system.nt_sec_desc.*
 *                     system.nt_sec_desc.*+
 *
 *                  where <attribute name> is one of:
 *
 *                     revision
 *                     owner
 *                     owner+
 *                     group
 *                     group+
 *                     acl:<name or sid>
 *                     acl+:<name or sid
    """
    print "test_xattr"
    furl = touch_file("tmpfile.out")
    plus_xattrs = ["%s%s" % (i,j) for i in ["owner", "group", "*"] for j in ["","+"]]
    plus_xattrs.append("revision")
    valid_xatts = ["system.nt_sec_desc." +i for i in  plus_xattrs]
    for xattr in valid_xatts:
        print "\ttesting %s with %s" % (furl, xattr)
        assert(ctx.getxattr(furl, xattr))
    ctx.open(furl)

def test_xattr_put():
    print "test_xattr_put"
    furl = touch_file("tmpfile_set.out")
    attrs = ctx.getxattr(furl, smbc.XATTR_ALL)
    print "attrs(%s): %s" % (smbc.XATTR_ALL,  attrs)
    ctx.setxattr(furl, smbc.XATTR_ALL, attrs, smbc.XATTR_FLAG_REPLACE)
    
def test_Workgroup():
    list = ctx.opendir('smb://').getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(entry.smbc_type == 1)

def test_Server():
    uri = 'smb://' + settings.SERVER
    list = ctx.opendir(uri).getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(entry.smbc_type == 2)

def test_Share():
    uri = 'smb://' + settings.SERVER
    list = ctx.opendir(uri).getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(3 <= entry.smbc_type and entry.smbc_type <= 6)

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
    assert smbc.ACL_ALL
    
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
    attrs = ctx.getxattr(furl, smbc.ACL_ALL)
    print "attrs(%s): %s" % (smbc.ACL_ALL,  attrs)
    ctx.setxattr(furl, smbc.ACL_ALL, attrs, smbc.XATTR_FLAG_REPLACE)
    
def test_acl_parser():
    acl_s = "REVISION:1,OWNER:S-1-5-21-833659924-920326847-3160110649-3002,GROUP:S-1-22-2-1002,ACL:S-1-5-21-833659924-920326847-3160110649-3002:0/0/0x001e01ff,ACL:S-1-22-2-1002:0/0/0x00120089,ACL:S-1-1-0:0/0/0x00120089"
    acl = dict()
    acl["ACL"] = []
    for i in acl_s.split(","):
        k,v = i.split(":")
        if k != "ACL":
            acl[k] = v
        else:
            acl[k].append(v)
    
    print "aci: %s" % acl
    assert False
    
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

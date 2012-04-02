#!/usr/bin/env python
import os
import smbc
import settings
import nose
from nose.plugins.skip import SkipTest


baseurl =  'smb://' + settings.SERVER + "/" + settings.SHARE +"/"


# a nice map from/to smbc constants
smbcType = {
    'WORKGROUP' : smbc.WORKGROUP,
    'SERVER' : smbc.SERVER,
    'FILE_SHARE' : smbc.FILE_SHARE,
    'PRINTER_SHARE' : smbc.PRINTER_SHARE,
    'IPC_SHARE' : smbc.IPC_SHARE,	
	
    smbc.WORKGROUP :  'WORKGROUP',
    smbc.SERVER : 'SERVER',
    smbc.FILE_SHARE : 'FILE_SHARE',
    smbc.PRINTER_SHARE : 'PRINTER_SHARE',
    smbc.IPC_SHARE : 'IPC_SHARE'
}


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

    # create all combinations of attribute strings
    plus_xattrs = ["%s%s" % (i,j) for i in ["owner", "group", "*"] for j in ["","+"]]
    plus_xattrs.append("revision")
    valid_xatts = ["system.nt_sec_desc." +i for i in  plus_xattrs]

    # check their existence
    for xattr in valid_xatts:
        print "\ttesting %s with %s" % (furl, xattr)
        assert(ctx.getxattr(furl, xattr))
    ctx.open(furl)

def test_xattr_put():
    raise SkipTest("xattr_put to be implemented")
    print "test_xattr_put"
    furl = touch_file("tmpfile_set.out")
    attrs = ctx.getxattr(furl, smbc.XATTR_ALL_SID)
    print "attrs(%s): %s" % (smbc.XATTR_ALL_SID,  attrs)
    ctx.setxattr(furl, smbc.XATTR_ALL_SID, attrs, smbc.XATTR_FLAG_REPLACE)
    
def test_Workgroup():
    list = ctx.opendir('smb://').getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(entry.smbc_type == smbc.WORKGROUP), "Entry %s of type %s, expected %d" % (entry.name, smbcType[entry.smbc_type], smbc.WORKGROUP)

def test_Server():
    uri = 'smb://' + settings.WORKGROUP 
    list = ctx.opendir(uri).getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(entry.smbc_type == smbc.SERVER), "Entry %s of type %s, expected %d" % (entry.name, smbcType[entry.smbc_type], smbc.SERVER)

def test_Share():
    uri = 'smb://' + settings.SERVER
    list = ctx.opendir(uri).getdents()

    allowed_shares = [smbc.FILE_SHARE, smbc.PRINTER_SHARE, smbc.IPC_SHARE, smbc.COMMS_SHARE]
    assert(len(list) > 0)
    for entry in list:
        assert (entry.smbc_type in allowed_shares), "Entry was %s (%d), expected values: %s" % (smbcType[entry.smbc_type], entry.smbc_type, allowed_shares) 

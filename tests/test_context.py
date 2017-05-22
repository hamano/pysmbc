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

# another map for system errors TODO can you find them in another module?
EINVAL = 22

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
    print("test_xattr")
    furl = touch_file("tmpfile.out")

    # create all combinations of attribute strings
    plus_xattrs = ["%s%s" % (i,j) for i in ["owner", "group", "*"] for j in ["","+"]]
    plus_xattrs.append("revision")
    valid_xatts = ["system.nt_sec_desc." +i for i in  plus_xattrs]

    # check their existence
    for xattr in valid_xatts:
        print("\ttesting %s with %s" % (furl, xattr))
        assert(ctx.getxattr(furl, xattr))
    ctx.open(furl)

def test_xattr_get_error():
    """ Verify that a RuntimeError is raised when passing bad arguments to getxattr()

         Bad arguments include malformed xattrs and unexistent file
    """
    print("test_xattr")
    furl = touch_file("tmpfile.out")

    # create all combinations of attribute strings
    plus_xattrs = ["%s%s" % (i,j) for i in ["owner", "pluto", "*"] for j in ["x","-"]]
    plus_xattrs.append("revisionX")
    invalid_xatts = ["system.nt_sec_desc." +i for i in  plus_xattrs]

    try:
        ctx.getxattr("UNEXISTENT", smbc.XATTR_OWNER)
        assert False, "getxattr should fail with an unexistent file"
    except ValueError as e: 
        (errno,strerror) = e.args 
        assert errno == EINVAL # TODO is it possible to trap an unexistent entity error from smbclient?
        pass

    # check their existence
    for xattr in invalid_xatts:
        print("\ttesting %s with %s" % (furl, xattr))
        try:
            ctx.getxattr(furl, xattr)
            assert False, "getxattr should fail with %s" % xattr
        except ValueError as e:
            (errno,strerror) = e.args 
            assert errno == EINVAL # invalid arguments

    ctx.open(furl)

def test_xattr_set():
    #raise SkipTest("xattr_set to be implemented")
    print("test_xattr_put")
    furl = touch_file("tmpfile_set.out")
    attr_name = smbc.XATTR_ALL
    attrs = ctx.getxattr(furl, attr_name)
    print("attrs(%s): %s" % (attr_name,  attrs))
    ctx.setxattr(furl, attr_name, attrs, smbc.XATTR_FLAG_REPLACE)
    attrs1 = ctx.getxattr(furl, attr_name)
    print("attrs1(%s): %s" % (attr_name,  attrs1))
    assert attrs1 == attrs

@SkipTest
def test_xattr_set_2():
    furl = touch_file("tmpfile_set.out")
    attrs_new = u'REVISION:1,OWNER:RPOLLI\\babel" \
        + ",GROUP:Unix Group\\babel" \
        + ",ACL:RPOLLI\\babel:0/0/0x001e01ff" \
        + ",ACL:Unix Group\\babel:0/0/0x00120089" \
        + ",ACL:Unix Group\\games:0/0/0x001e01ff" \
        + ",ACL:\\Everyone:0/0/0x00120089'
    attr_name = smbc.XATTR_ALL_SID
    attrs_0 = ctx.getxattr(furl, attr_name)
    print("original attrs(%s)" % attrs_0)

    assert attrs_0 != attrs_new, "Old and new attributes are the same:\n%s\n%s\n" % (attrs_0, attrs_new)

    ctx.setxattr(furl, attr_name, attrs_new, smbc.XATTR_FLAG_REPLACE)
    attrs_1 = ctx.getxattr(furl, attr_name)

    print("attrs_1(%s): %s" % (attr_name,  attrs_1))
    assert attrs_1 == attrs_new

def test_xattr_set_error():
    #raise SkipTest("xattr_set to be implemented")
    print("test_xattr_set_error")
    furl = touch_file("tmpfile_set.out")
    attr_name = smbc.XATTR_ALL_SID
    attrs_ok = ctx.getxattr(furl, attr_name)
    attrs = "BAD_VALUE" # causes segfault
    for xa in ["BAD_VALUE", u'REVISION:1,OWNER:RPOLLI\\babel,GROUP:', 0, None]: 
        try:
            ctx.setxattr(furl, attr_name, xa, smbc.XATTR_FLAG_REPLACE)
        except ValueError as e:
            (errno,strerror) = e.args 
            assert errno == EINVAL # invalid arguments
            print("setxattr(%s) raises  %s" % (xa, e))
            pass
        except TypeError as e:
            print("setxattr(%s) raises  %s" % (xa, e))
            pass 
    
    
    
def test_Workgroup():
    l_entries = ctx.opendir('smb://').getdents()
    assert(len(l_entries) > 0)
    for entry in l_entries:
        assert(entry.smbc_type == smbc.WORKGROUP), "Entry %s of type %s, expected %s" % (entry.name, smbcType[entry.smbc_type], smbcType[smbc.WORKGROUP])

def test_Server():
    uri = 'smb://' + settings.WORKGROUP 
    l_entries = ctx.opendir(uri).getdents()
    assert(len(l_entries) > 0)
    for entry in l_entries:
        assert(entry.smbc_type == smbc.SERVER), "Entry %s of type %s, expected %s" % (entry.name, smbcType[entry.smbc_type], smbcType[smbc.SERVER])

def test_Share():
    uri = 'smb://' + settings.SERVER
    l_entries = ctx.opendir(uri).getdents()

    allowed_shares = [smbc.FILE_SHARE, smbc.PRINTER_SHARE, smbc.IPC_SHARE, smbc.COMMS_SHARE]
    assert(len(l_entries) > 0)
    for entry in l_entries:
        assert (entry.smbc_type in allowed_shares), "Entry was %s (%d), expected values: %s" % (smbcType[entry.smbc_type], entry.smbc_type, allowed_shares) 

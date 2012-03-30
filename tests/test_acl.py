import os
import smbc
import settings
import nose

""" Test acl parser


"""

"""SmbAcl parser To be implemented"""
def test_acl_parser():
    raise nose.plugins.skip.SkipTest("SmbAcl to be implemented")

    acl_s = "REVISION:1,OWNER:S-1-5-21-833659924-920326847-3160110649-3002,GROUP:S-1-22-2-1002,ACL:S-1-5-21-833659924-920326847-3160110649-3002:0/0/0x001e01ff,ACL:S-1-22-2-1002:0/0/0x00120089,ACL:S-1-1-0:0/0/0x00120089"
    acl = SmbAcl(acl_s)
    print "" , acl["revision"], acl["acl"]
    assert "revision" in acl, "error defining acl %s" % acl
    assert "acl" in acl, "error defining acl %s" % acl
    assert "owner" in acl, "error defining acl %s" % acl
    assert "group" in acl, "error defining acl %s" % acl

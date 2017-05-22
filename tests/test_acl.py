import os
import smbc
import settings
import nose
from nose.plugins.skip import SkipTest


""" Test acl parser


"""

class SmbAcl():
    revision = None
    owner = None
    group = None
    acl = []

    def __init__(self, xattr_s=None):
        """ parse an acl into a SmbAcl object """
        if xattr_s:
            xattr_l = [ s[s.index(":")+1:] for s in xattr_s.split(",")]
            (self.revision, self.owner, self.group) = xattr_l[0:3]
            self.acl = xattr_l[3:] 


    def __str__(self):
        ret = "REVISION:%s,OWNER:%s,GROUP:%s" % (self.revision,self.owner,self.group)
        for a in self.acl:
            ret = ret + ",ACL:%s" % a
        return ret

    @staticmethod
    def get_target(acl_s):
        return acl_s[0:acl_s.index(":")]

    @staticmethod
    def get_perm(acl_s):
        return acl_s[acl_s.index(":")+1:]


"""SmbAcl parser To be implemented"""
def test_acl_parser():
    raise SkipTest("SmbAcl to be implemented")

    xattr_l = ["REVISION:1,OWNER:S-1-5-21-833659924-920326847-3160110649-3002,GROUP:S-1-22-2-1002,ACL:S-1-5-21-833659924-920326847-3160110649-3002:0/0/0x001e01ff,ACL:S-1-22-2-1002:0/0/0x00120089,ACL:S-1-1-0:0/0/0x00120089",
   #attrs(system.nt_sec_desc.*+):
   'REVISION:1,OWNER:RPOLLI\\babel,GROUP:Unix Group\\babel,ACL:RPOLLI\\babel:0/0/0x001e01ff,ACL:Unix Group\\babel:0/0/0x00120089,ACL:\Everyone:0/0/0x00120089']
    
    for xattr_s in xattr_l: 
        xattr = SmbAcl(xattr_s)
        print("xattr: %s" % xattr)
        
        assert xattr.revision
        assert xattr.owner
        assert xattr.group
        assert xattr.acl and len(xattr.acl)>1

def test_acl_get_target():
    acl_l = ["S-1-5-21-833659924-920326847-3160110649-3002:0/0/0x001e01ff", 
	"Unix Group\\babel:0/0/0x00120089"]
    for acl_s in acl_l:
        perm = SmbAcl.get_perm(acl_s)
        target = SmbAcl.get_target(acl_s)
      
        assert target 
        assert perm
     

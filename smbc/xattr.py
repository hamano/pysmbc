#!/usr/bin/python


SMB_POSIX_ACL_USER_OBJ=0x01
SMB_POSIX_ACL_USER=0x02
SMB_POSIX_ACL_GROUP_OBJ=0x04
SMB_POSIX_ACL_GROUP=0x08
SMB_POSIX_ACL_MASK=0x10
SMB_POSIX_ACL_OTHER=0x20


SMB_POSIX_ACL_READ=0x04
SMB_POSIX_ACL_WRITE=0x02
SMB_POSIX_ACL_EXECUTE=0x01

SMB_POSIX_ACL_HEADER_SIZE=6
SMB_POSIX_ACL_ENTRY_SIZE=10


rwx=0x001e01ff
x=0x001200a0
w=0x00120116
r=0x00120089

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

#
#import smbc
#
#c = smbc.Context(debug=1)
#cb = lambda se, sh, w, u, p: ("WORKGROUP","babel","b3nedetto")
#c.functionAuthData = cb
#
#print c.opendir("smb://localhost/share/Neffa").getdents()
#print c.getxattr("smb://localhost/share/Neffa", "system.nt_sec_desc.*")
#print c.getxattr("smb://localhost/share/Neffa", "s")
#

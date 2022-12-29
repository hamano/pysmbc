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

ACL_KEYWORDS = {"revision" : "", "owner" : "", "group" : "", "acl" : ""}

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

def parse_sec_desc(raw, parsed):
    """ Parses the raw security descriptor string into the expected
        components and stores them in parsed """

    parts = raw.split(",")
    for part in parts:
        key, value = part.split(":", 1)
        key = key.lower().strip()
        if key in ACL_KEYWORDS:
            if key != "acl":
                parsed[key] = value
            else:
                sid, perms = value.split(":", 1)
                if sid not in parsed["acl"]:
                    parsed["acl"][sid] = []
                parsed["acl"][sid].append(perms)

def convert_acl_hex_to_int(data):
    """ Converts all ACL masks from hexadecimal to integer since
        smbc_setxattr() expects integer format """

    attrs_new = ""
    if data.get("revision"):
        attrs_new += "REVISION:" + data["revision"] + ","
    if data.get("owner"):
        attrs_new += "OWNER:" + data["owner"] + ","
    if data.get("group"):
        attrs_new += "GROUP:" + data["group"] + ","
    if data.get("acl"):
        for key in data["acl"].keys():
            for i in range(len(data["acl"][key])):
                attrs_new += "ACL:" + key + ":" + data["acl"][key][i].split("0x")[0] + str(int(data["acl"][key][i].split("/")[2], 0)) + ","

    return attrs_new[:-1]

def compare_xattr(xattr_converted, xattr_current):
    """ Compare the ACLs since the order might get altered by
        smbc_getxattr() """

    xattr_converted_data = {"revision" : "", "owner" : "", "group" : "", "acl" : {}}
    xattr_current_data = {"revision" : "", "owner" : "", "group" : "", "acl" : {}}

    parse_sec_desc(xattr_converted, xattr_converted_data)
    parse_sec_desc(xattr_current, xattr_current_data)

    for acl in xattr_current_data.get("acl"):
        if xattr_converted_data.get("acl").get(acl):
            for entry in xattr_current_data.get("acl").get(acl):
                int_entry = entry.split("0x")[0] + str(int(entry.split("/")[2], 0))
                if int_entry not in xattr_converted_data.get("acl").get(acl):
                    return False
        else:
            return False
    return True

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

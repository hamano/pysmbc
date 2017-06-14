#!/usr/bin/env python

## Copyright (C) 2002, 2005, 2006, 2007, 2008, 2010, 2011, 2012  Red Hat, Inc
## Copyright (C) 2010  Open Source Solution Technology Corporation
## Authors:
##  Tim Waugh <twaugh@redhat.com>
##  Tsukasa Hamano <hamano@osstech.co.jp>

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

"""This is a set of Python bindings for the libsmbclient library
from the samba project.

>>> # Directory listing example:
>>> import smbc
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> entries = ctx.opendir ("smb://SERVER").getdents ()
>>> for entry in entries:
...     print entry
<smbc.Dirent object "music" (File share) at 0x7fbd7c42b3a0>
<smbc.Dirent object "IPC$" (IPC share) at 0x7fbd7c42b148>
<smbc.Dirent object "Charlie" (Printer share) at 0x7fbd7c42b3c8>
>>> d = ctx.open ("smb://SERVER/music")

>>> # Write file example:
>>> import smbc
>>> import os
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> file = ctx.open ("smb://SERVER/music/file.txt", os.O_CREAT | os.O_WRONLY)
>>> file.write ("hello")

>>> # Read file example:
>>> import smbc
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> file = ctx.open ("smb://SERVER/music/file.txt")
>>> print file.read()
hello

"""

from distutils.core import setup, Extension
import subprocess

def pkgconfig_I (pkg):
    dirs = []
    c = subprocess.Popen (["pkg-config", "--cflags", pkg],
                          stdout=subprocess.PIPE)
    (stdout, stderr) = c.communicate ()
    for p in stdout.decode ('ascii').split ():
        if p.startswith ("-I"):
            dirs.append (p[2:])
    return dirs
    
setup (name="pysmbc",
       version="1.0.15.7",
       description="Python bindings for libsmbclient",
       long_description=__doc__,
       author=["Tim Waugh <twaugh@redhat.com>",
               "Tsukasa Hamano <hamano@osstech.co.jp>",
               "Roberto Polli <rpolli@babel.it>" ],
       url="http://cyberelk.net/tim/software/pysmbc/",
       download_url="http://cyberelk.net/tim/data/pysmbc/",
       classifiers=[
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "License :: OSI Approved :: GNU General Public License (GPL)",
        "Development Status :: 5 - Production/Stable",
        "Operating System :: Unix",
        "Programming Language :: C",
        ],
       license="GPLv2+",
       packages=["smbc"],
       ext_modules=[Extension("_smbc",
                              ["smbc/smbcmodule.c",
                               "smbc/context.c",
                               "smbc/dir.c",
                               "smbc/file.c",
                               "smbc/smbcdirent.c"],
                              libraries=["smbclient"],
                              include_dirs=pkgconfig_I("smbclient"))])

SMB bindings for Python
-----------------------

[![PyPI](https://img.shields.io/pypi/v/pysmbc.svg)](https://pypi.python.org/pypi/pysmbc/)
[![Build Status](https://travis-ci.org/hamano/pysmbc.svg?branch=master)](https://travis-ci.org/hamano/pysmbc)
[![GitHub license](https://img.shields.io/github/license/hamano/pysmbc.svg)]()

These Python bindings are intended to wrap the libsmbclient API.


Prerequisites
------

Currently libsmbclient 3.2.x or later is required.  Ubuntu Example:
~~~
# sudo apt install build-essential pkg-config smbclient libsmbclient libsmbclient-dev python-dev
~~~

Or `python3-dev` instead of `python-dev` depending on your needs.

Build
------
~~~
# make
~~~


Test
------

To run Python tests in tests/ you need python-nose
See nose documentation http://readthedocs.org/docs/nose

To run all the tests execute

~~~
# nosetests
~~~

To run just one test, use

~~~
# nosetests file.py
~~~

To selectively run test methods, printing output to console

~~~
# nosetests -vs  test_context.py:test_Workgroup
~~~

NOTE: to run your tests, you need

 * a running samba server
 * one shared folder with
	* rw permissions
 	* guest ok = no

Examples
------

Directory listing

~~~
>>> import smbc
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> entries = ctx.opendir ("smb://SERVER").getdents ()
>>> for entry in entries:
...     print entry
<smbc.Dirent object "music" (File share) at 0x7fbd7c42b3a0>
<smbc.Dirent object "IPC$" (IPC share) at 0x7fbd7c42b148>
<smbc.Dirent object "Charlie" (Printer share) at 0x7fbd7c42b3c8>
>>> d = ctx.open ("smb://SERVER/music")
~~~

Shared Printer Listing

~~~
>>> import smbc
>>> ctx = smbc.Context()
>>> ctx.optionNoAutoAnonymousLogin = True
>>> ctx.functionAuthData = lambda se, sh, w, u, p: (domain_name, domain_username, domain_password)
>>> uri = 'smb://' + smb_server
>>> shared_printers = []
>>> entries = ctx.opendir(uri).getdents()
>>> 	for entry in entries:
>>> 		if entry.smbc_type == 4:
>>> 			shared_printers.append(entry.name)
~~~

Write file

~~~
>>> import smbc
>>> import os
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> file = ctx.open ("smb://SERVER/music/file.txt", os.O_CREAT | os.O_WRONLY)
>>> file.write ("hello")
~~~

Read file

~~~
>>> import smbc
>>> ctx = smbc.Context (auth_fn=my_auth_callback_fn)
>>> file = ctx.open ("smb://SERVER/music/file.txt")
>>> print file.read()
hello
~~~

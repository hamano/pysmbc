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
# sudo apt install pkg-config libopencv-dev smbclient libsmbclient libsmbclient-dev
~~~

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


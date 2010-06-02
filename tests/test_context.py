#!/usr/bin/env python

import smbc
import settings

def setUp():
    global ctx
    ctx = smbc.Context()

def tearDown():
    global ctx
    del ctx

def test_Workgroup():
    list = ctx.opendir('smb://').getdents()
    assert(len(list) > 0)
    for entry in list:
        assert(entry.smbc_type == 1)

def test_Server():
    uri = 'smb://' + settings.WORKGROUP
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

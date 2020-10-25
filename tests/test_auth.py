import smbc

def test_auth_succes(config):
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, config['username'], config['password'])
    ctx.functionAuthData = cb
    d = ctx.opendir(config['uri'])
    assert d != None

def test_auth_failed_noauth(config):
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    try:
        ctx.opendir(config['uri'])
    except smbc.PermissionError:
        assert True
    else:
        assert False

def test_auth_failed_nopass(config):
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, config['username'], "")
    ctx.functionAuthData = cb
    try:
        ctx.opendir(config['uri'])
    except smbc.PermissionError:
        assert True
    else:
        assert False

def test_auth_failed_nouser(config):
    ctx = smbc.Context()
    ctx.optionNoAutoAnonymousLogin = True
    cb = lambda se, sh, w, u, p: (w, "", config['password'])
    ctx.functionAuthData = cb
    try:
        ctx.opendir(config['uri'])
    except smbc.PermissionError:
        assert True
    else:
        assert False

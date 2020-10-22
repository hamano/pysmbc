import pytest

def pytest_addoption(parser):
    parser.addoption('--server', action='store', default='localhost')
    parser.addoption('--share', action='store', default='share')
    parser.addoption('--username', action='store', default='user1')
    parser.addoption('--password', action='store', default='password1')

@pytest.fixture(autouse=True, scope='session')
def config(pytestconfig):
    server = pytestconfig.getoption('server')
    share = pytestconfig.getoption('share')
    username = pytestconfig.getoption('username')
    password = pytestconfig.getoption('password')
    uri = 'smb://{}/{}/'.format(server, share)
    yield {
        'server': server,
        'share': share,
        'username': username,
        'password': password,
        'uri': uri,
    }

import os, pycurl, getopt, sys, urllib
from functools import partial
from StringIO import StringIO
import base64

try:
        from io import BytesIO
except ImportError:
        from StringIO import StringIO as BytesIO

def base_url():
    return "http://localhost/irods-rest/1.0.0/"

def authenticate(_user_name, _password, _auth_type):
    buffer = StringIO()

    creds = _user_name + ':' + _password
    buff  = creds.encode('ascii')
    token = base64.b64encode(buff , None)

    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Authorization: Native '+token])
    c.setopt(c.CUSTOMREQUEST, 'POST')
    url = base_url()+'auth'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def access(_token, _logical_path):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    url = '{0}access?path={1}'.format(base_url(), _logical_path)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def get_configuration(_token):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    url = '{0}get_configuration'.format(base_url())

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def put_configuration(_token, _cfg):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'PUT')

    url = '{0}put_configuration?cfg={1}'.format(base_url(), _cfg)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def list(_token, _path, _stat, _permissions, _metadata, _offset, _limit):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    url = base_url()+'list?path={0}&stat={1}&permissions={2}&metadata={3}&offset={4}&limit={5}'.format(_path, _stat, _permissions, _metadata, _offset, _limit)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def put(_token, _physical_path, _logical_path):
    body = ""
    offset = 0
    file_size = 0
    read_size = 1024 * 1024 * 4
    with open(_physical_path, 'r') as f:
        for data in iter(partial(f.read, read_size), b''):
            c = pycurl.Curl()
            c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
            c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
            c.setopt(c.CUSTOMREQUEST, 'PUT')

            c.setopt(c.POSTFIELDSIZE, len(data))
            data_buffer = BytesIO(data.encode('utf-8'))
            c.setopt(c.READDATA, data_buffer)
            c.setopt(c.UPLOAD, 1)

            file_size = file_size + len(data)
            c.setopt(c.URL, '{0}stream?path={1}&offset={2}&limit={3}'.format(base_url(), _logical_path, offset, file_size))

            body_buffer = StringIO()
            c.setopt(c.WRITEDATA, body_buffer)

            c.perform()

            offset = offset + len(data)

            c.close()

            body = body_buffer.getvalue()
    return body

def get(_token, _physical_path, _logical_path):
    offset = 0
    read_size = 1024 * 1024 * 4
    with open(_physical_path, 'w') as f:
        while True:
            c = pycurl.Curl()
            c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
            c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
            c.setopt(c.CUSTOMREQUEST, 'GET')

            url = '{0}stream?path={1}&offset={2}&limit={3}'.format(base_url(), _logical_path, offset, read_size)
            c.setopt(c.URL, url)

            body_buffer = StringIO()
            c.setopt(c.WRITEDATA, body_buffer)

            c.perform()
            c.close()

            body = body_buffer.getvalue()

            if len(body) == 0:
                print("Length of body is zero")
                break

            f.write(body)

            offset = offset + len(body)

    return "Success"

def admin(_token, _action, _target, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    params = { 'action' : _action,
               'target' : _target,
               'arg2'   : _arg2,
               'arg3'   : _arg3,
               'arg4'   : _arg4,
               'arg5'   : _arg5,
               'arg6'   : _arg6,
               'arg7'   : _arg7
             }
    url = base_url()+'admin?'+urllib.urlencode(params)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def zone_report(_token):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')
    url = '{0}zone_report'.format(base_url())

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def query(_token, _string, _limit, _offset, _type):
    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    params = { 'query_string' : _string,
               'query_limit'  : _limit,
               'row_offset'   : _offset,
               'query_type'   : _type }
    url = base_url()+'query?'+urllib.urlencode(params)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body

def get_arguments():
    full_args = sys.argv
    arg_list  = full_args[1:]
    options_list = [ 'user_name=',    'password=',      'command=',
                     'logical_path=', 'physical_path=', 'metadata',
                     'permissions',   'stat',           'offset=',
                     'limit=',        'type=',          'query=',
                     'action=', 'target=', 'arg2=', 'arg3=', 'arg4=', 'arg5=', 'arg6=', 'arg7='
                   ]

    try:
        arguments, values = getopt.getopt(arg_list, [], options_list)
        return dict((arguments))
    except getopt.error as err:
        print (str(err))
        sys.exit(2)

def get_value(_args, _key):
    try:
        return _args['--'+_key]
    except:
        return ''

def get_flag(_args, _key):
    try:
        if None == _args['--'+_key]:
            return False
        else:
            return True
    except:
        return False

def main():
    args  = get_arguments()
    token = authenticate(get_value(args, 'user_name'), get_value(args, 'password'), 'native')
    cmd = args['--command']
    if('query' == cmd):
        qstr   = get_value(args, 'query')
        qtype  = get_value(args, 'type')
        limit  = get_value(args, 'limit')
        offset = get_value(args, 'offset')
        print query(token, qstr, limit, offset, qtype)
    elif('admin' == cmd):
        action = get_value(args, 'action')
        target = get_value(args, 'target')
        arg2   = get_value(args, 'arg2')
        arg3   = get_value(args, 'arg3')
        arg4   = get_value(args, 'arg4')
        arg5   = get_value(args, 'arg5')
        arg6   = get_value(args, 'arg6')
        arg7   = get_value(args, 'arg7')
        print admin(token, action, target, arg2, arg3, arg4, arg5, arg6, arg7)
    elif('get_configuration' == cmd):
        print get_configuration(token)
    elif('put_configuration' == cmd):
        cfg    = get_value(args, 'configuration')
        print put_configuration(token, cfg)
    elif('get' == cmd):
        print get(token, get_value(args,'physical_path'), get_value(args,'logical_path'))
    elif('put' == cmd):
        print put(token, get_value(args,'physical_path'), get_value(args,'logical_path'))
    elif('list' == cmd):
        path   = get_value(args, 'logical_path')
        limit  = get_value(args, 'limit')
        offset = get_value(args, 'offset')
        stat   = get_flag(args, 'stat')
        mdata  = get_flag(args, 'metadata')
        perms  = get_flag(args, 'permissions')
        print list(token, path, stat, perms, mdata, offset, limit)
    elif('access' == cmd):
        path = get_value(args, 'logical_path')
        print access(token, path)
    elif('zone_report' == cmd):
        print zone_report(token)
    else:
        print('Command ['+cmd+'] is not supported.')

if __name__ == '__main__':
        main()

import os, pycurl, getopt, sys, urllib
from functools import partial
from io import StringIO ## for Python 3
import base64
import tempfile

from . import settings

try:
    from io import BytesIO
except ImportError:
    from StringIO import StringIO as BytesIO

def base_url(reverse_proxy_host=None):
    if reverse_proxy_host == None:
        reverse_proxy_host = settings.HOSTNAME_1
    return f"http://{reverse_proxy_host}/irods-rest/0.9.2/"

def authenticate(_user_name, _password, _auth_type):
    buffer = BytesIO()

    creds = _user_name + ':' + _password
    buff  = creds.encode('ascii')
    token = base64.b64encode(buff , None)

    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER, ['Authorization: Native '+token.decode()])
    c.setopt(c.CUSTOMREQUEST, 'POST')
    url = base_url()+'auth'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)

    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def logical_path_rename(_token, _src, _dst):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')
    url = base_url()+'logicalpath/rename?src={0}&dst={1}'.format(_src, _dst)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)

    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def logical_path_replicate(_token, _logical_path, _admin=None, _all=None, _num_threads=None, _repl_num=None,
         _recursive=None, _dst_resource=None,
         _src_resource=None):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    url = base_url()+f'logicalpath/replicate?logical-path={_logical_path}'

    if _admin        : url += '&admin-mode=1'
    if _all          : url += '&all=1'
    if _num_threads  : url += f'&thread-count={_num_threads}'
    if _recursive    : url += '&recursive=1'
    if _repl_num     : url += f'&replica-number={_repl_num}'
    if _src_resource : url += f'&src-resource={_src_resource}'
    if _dst_resource : url += f'&dst-resource={_dst_resource}'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)

    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def logical_path_delete(_token, _logical_path, _no_trash = None,
                        _recursive = None, _unregister = None):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Authorization: ' + _token])
    c.setopt(c.CUSTOMREQUEST, 'DELETE')

    url = base_url()+'logicalpath?logical-path={0}'.format(_logical_path)
    if _no_trash: url += '&no-trash=1'
    if _unregister: url += '&unregister=1'
    if _recursive: url += '&recursive=1'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def logical_path_trim(_token, _logical_path, _age=None, _repl_num=None, _src_resc=None, _num_copies=None,
         _admin=None, _recursive=None):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    url = base_url()+f'logicalpath/trim?logical-path={_logical_path}'

    if _admin          : url += '&admin-mode=1'
    if _recursive      : url += '&recursive=1'
    if _age            : url += f'&minimum-age-in-minutes={_age}'
    if _repl_num       : url += f'&replica-number={_repl_num}'
    if _src_resc       : url += f'&src-resource={_src_resc}'
    if _num_copies     : url += f'&minimum-number-of-remaining-replicas={_num_copies}'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def metadata(_token, _cmds):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+  _token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    data_buf = BytesIO(_cmds.encode('utf-8'))
    c.setopt(c.POSTFIELDSIZE, len(_cmds))
    c.setopt(c.READDATA, data_buf)
    c.setopt(c.UPLOAD, 1)


    url = base_url()+"metadata"
    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.setopt(pycurl.HTTP_VERSION, pycurl.CURL_HTTP_VERSION_1_1)
    c.perform()
    c.close()
    body = buffer.getvalue()
    return body.decode('utf-8')

def access(_token, _logical_path, _ticket_type=None, _use_count=None,
           _write_file_count=None, _write_byte_count=None, _seconds_until_expiration=None,
           _users=None, _groups=None, _hosts=None):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    url = '{0}access?path={1}'.format(base_url(), _logical_path)

    if _ticket_type             : url += '&type={0}'.format(_ticket_type)
    if _use_count               : url += '&use_count={0}'.format(_use_count)
    if _write_file_count        : url += '&write_file_count={0}'.format(_write_file_count)
    if _write_byte_count        : url += '&write_byte_count={0}'.format(_write_byte_count)
    if _seconds_until_expiration: url += '&seconds_until_expiration={0}'.format(_seconds_until_expiration)
    if _users                   : url += '&users={0}'.format(_users)
    if _groups                  : url += '&groups={0}'.format(_groups)
    if _hosts                   : url += '&hosts={0}'.format(_hosts)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def get_configuration(_token):
    buffer = BytesIO()
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

    return body.decode('utf-8')

def put_configuration(_token, _cfg):
    buffer = BytesIO()
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

    return body.decode('utf-8')

def list(_token, _path, _stat, _permissions, _metadata, _offset, _limit, _recursive="1"):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    url = base_url()+f'list?path={_path}&stat={_stat}&permissions={_permissions}&metadata={_metadata}&offset={_offset}&limit={_limit}&recursive={_recursive}'

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def put(_token, _physical_path, _logical_path, _ticket_id=None):
    body = ""
    offset = 0
    file_size = 0
    read_size = 1024 * 1024 * 4
    with open(_physical_path, 'r') as f:
        for data in iter(partial(f.read, read_size), b''):
            c = pycurl.Curl()
            c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
            c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])

            if _ticket_id:
                c.setopt(pycurl.HTTPHEADER,['irods-ticket: '+_ticket_id])

            c.setopt(c.CUSTOMREQUEST, 'PUT')

            c.setopt(c.POSTFIELDSIZE, len(data))
            data_buffer = BytesIO(data.encode('utf-8'))
            c.setopt(c.READDATA, data_buffer)
            c.setopt(c.UPLOAD, 1)

            file_size = file_size + len(data)
            c.setopt(c.URL, '{0}stream?path={1}&offset={2}&count={3}'.format(base_url(), _logical_path, offset, file_size))

            body_buffer = BytesIO()
            c.setopt(c.WRITEDATA, body_buffer)

            c.perform()

            offset = offset + len(data)

            c.close()

            body = body_buffer.getvalue()

    return body.decode('utf-8')

def get(_token, _physical_path, _logical_path, _ticket_id=None):
    offset = 0
    read_size = 1024 * 1024 * 4
    with open(_physical_path, 'w') as f:
        while True:
            c = pycurl.Curl()
            c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
            c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])

            if _ticket_id:
                c.setopt(pycurl.HTTPHEADER,['irods-ticket: '+_ticket_id])

            c.setopt(c.CUSTOMREQUEST, 'GET')

            url = '{0}stream?path={1}&offset={2}&count={3}'.format(base_url(), _logical_path, offset, read_size)
            c.setopt(c.URL, url)

            body_buffer = StringIO()
            c.setopt(c.WRITEDATA, body_buffer)

            c.perform()
            c.close()

            body = body_buffer.getvalue()
            f.write(body)

            offset = offset + len(body)

    return "Success"

def admin(_token, _action, _target, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'POST')

    params = { 'action' : _action,
               'target' : _target,
               'arg2'   : _arg2,
               'arg3'   : _arg3,
               'arg4'   : _arg4,
               'arg5'   : _arg5,
               'arg6'   : _arg6,
               'arg7'   : _arg7
             }
    url = base_url()+'admin?'+urllib.parse.urlencode(params)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

def zone_report(_token):
    buffer = BytesIO()
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

    return body.decode('utf-8')

def query(_token, _string, _limit, _offset, _type, _case_sensitive='1', _distinct='1'):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(pycurl.HTTPHEADER,['Accept: application/json'])
    c.setopt(pycurl.HTTPHEADER,['Authorization: '+_token])
    c.setopt(c.CUSTOMREQUEST, 'GET')

    params = {'query_string'  : _string,
              'query_limit'   : _limit,
              'row_offset'    : _offset,
              'query_type'    : _type,
              'case_sensitive': _case_sensitive,
              'distinct'      : _distinct}
    url = base_url() + 'query?' + urllib.parse.urlencode(params)

    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()

    body = buffer.getvalue()

    return body.decode('utf-8')

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
        print (query(token, qstr, limit, offset, qtype))
    elif('admin' == cmd):
        action = get_value(args, 'action')
        target = get_value(args, 'target')
        arg2   = get_value(args, 'arg2')
        arg3   = get_value(args, 'arg3')
        arg4   = get_value(args, 'arg4')
        arg5   = get_value(args, 'arg5')
        arg6   = get_value(args, 'arg6')
        arg7   = get_value(args, 'arg7')
        print (admin(token, action, target, arg2, arg3, arg4, arg5, arg6, arg7))
    elif('get_configuration' == cmd):
        print (get_configuration(token))
    elif('put_configuration' == cmd):
        cfg    = get_value(args, 'configuration')
        print (put_configuration(token, cfg))
    elif('get' == cmd):
        print (get(token, get_value(args,'physical_path'), get_value(args,'logical_path')))
    elif('put' == cmd):
        print (put(token, get_value(args,'physical_path'), get_value(args,'logical_path')))
    elif('list' == cmd):
        path   = get_value(args, 'logical_path')
        limit  = get_value(args, 'limit')
        offset = get_value(args, 'offset')
        stat   = get_flag(args, 'stat')
        mdata  = get_flag(args, 'metadata')
        perms  = get_flag(args, 'permissions')
        print(list(token, path, stat, perms, mdata, offset, limit))
    elif('access' == cmd):
        path = get_value(args, 'logical_path')
        print(access(token, path))
    elif('zone_report' == cmd):
        print(zone_report(token))
    else:
        print('Command ['+cmd+'] is not supported.')

if __name__ == '__main__':
        main()

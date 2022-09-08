import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import shutil
from .. import lib
from . import session

import json
from . import irods_rest

def entity_has_metadata(session, _entity, _entity_type):
    entity_to_query_map = {
        'collection' : '"select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS where COLL_NAME = \'{}\'"',
        'data_object': f'"select META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS where COLL_NAME = \'{os.path.dirname(_entity)}\' and DATA_NAME = \'{os.path.basename(_entity)}\'"',
        'user'       : '"select META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS where USER_NAME = \'{}\'"',
        'resource'   : '"select META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS where RESC_NAME = \'{}\'"'
    }
    query = entity_to_query_map[_entity_type].format(_entity)
    query_output = session.run_icommand(['iquest', '%s:%s:%s', query])[0].split('\n')
    if "CAT_NO_ROWS_FOUND" in query_output[0]:
        return False
    return True # eventually should walk the actual avus to check for the target

class TestClientRest(session.make_sessions_mixin([], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(TestClientRest, self).setUp()
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(TestClientRest, self).tearDown()

    def test_invalid_authentication(self):
        token = irods_rest.authenticate('invalid', 'user', 'native')
        assert(token.find('827000') != -1)

    def test_authentication(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')
        assert(token.find('827000') == -1)

    def test_access_with_default_arguments(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_access_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                token = irods_rest.authenticate('rods', 'rods', 'native')

                json_string = irods_rest.access(token, logical_path)
                assert(json_string.find('error') == -1)

                json_object = json.loads(json_string)
                ticket_id = json_object['headers']['irods-ticket'][0]

                # Verify that the properties for the ticket are what we expect.
                _, out, _ = admin.assert_icommand(['iticket', 'ls', ticket_id], 'STDOUT', [
                    'ticket type: read',
                    'uses limit: 0',
                    'write file limit: 0',
                    'write byte limit: 0',
                    'expire time: none',
                    'No user restrictions',
                    'No group restrictions',
                    'No host restrictions'
                ])

                admin.assert_icommand(['iticket', 'delete', ticket_id])

            finally:
                os.remove(file_name)
                admin.assert_icommand(['irm', '-f', file_name])

    @unittest.skip('Fix me #129')
    def test_logical_path_rename(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')
        with session.make_session_for_existing_admin() as admin:
            try:
                coll = admin.home_collection + "/coll"

                # test on a data object
                new_coll = coll.replace('coll', 'new_coll')
                dobj = new_coll + "/data_object"
                new_dobj = dobj.replace("data_object", "new_data_object")
                admin.assert_icommand(['imkdir', coll])
                res = irods_rest.logical_path_rename(
                    token,
                    coll,
                    new_coll
                )
                self.assertEqual(res, "")
                # test on a collection
                # make sure that it works with a collection that's actually populated
                admin.assert_icommand(['itouch', dobj])
                res = irods_rest.logical_path_rename(
                    token,
                    dobj,
                    new_dobj
                )
                self.assertEqual(res, "")
            finally:
                admin.run_icommand(['irm', '-r', '-f', coll])
                admin.run_icommand(['irm', '-r', '-f', new_coll])


    @unittest.skip('Fix me #129')
    def test_logical_path_delete(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')
        with session.make_session_for_existing_admin() as admin:
            # test on data object
            coll_path = admin.home_collection + '/coll'
            dobj_path = coll_path + '/data_object'
            admin.assert_icommand(['imkdir', coll_path])
            admin.assert_icommand(['itouch', dobj_path])
            res = irods_rest.logical_path_delete(
                token,
                dobj_path
            )
            self.assertEqual(res, "")
            # test on collections
            res = irods_rest.logical_path_delete(
                token,
                coll_path
            )
            self.assertIn( "'recursive=1' required to delete a collection. Make sure you want to delete the whole sub-tree.", res)
            res = irods_rest.logical_path_delete(
                token,
                coll_path,
                _recursive=True
            )
            self.assertEqual(res, "")


    def test_access_with_explicit_arguments(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_access_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                token = irods_rest.authenticate('rods', 'rods', 'native')

                # The settings for the ticket.
                ticket_type = 'write'
                use_count = 50
                write_file_count = 100
                write_byte_count = 9999
                seconds_until_expiration = 45
                users = 'rods'
                groups = 'newgroup'
                hosts = 'irods.org'

                admin.assert_icommand(['iadmin', 'mkgroup', groups])

                json_string = irods_rest.access(token, logical_path, ticket_type, use_count,
                                                write_file_count, write_byte_count, seconds_until_expiration,
                                                users, groups, hosts)

                json_object = json.loads(json_string)
                ticket_id = json_object['headers']['irods-ticket'][0]

                # Verify that the properties for the ticket are what we expect.
                _, out, _ = admin.assert_icommand(['iticket', 'ls', ticket_id], 'STDOUT', [
                    'ticket type: ' + ticket_type,
                    'uses limit: ' + str(use_count),
                    'write file limit: ' + str(write_file_count),
                    'write byte limit: ' + str(write_byte_count),
                    'restricted-to user: ' + str(users),
                    'restricted-to group: ' + str(groups)
                ])

                # Expiration time and host restrictions must be checked separately because the
                # values for these fields are non-deterministic. The best we can do is show that
                # the values did change.
                self.assertNotIn('expire time: none', out)
                self.assertNotIn('No host restrictions', out)
                self.assertIn('restricted-to host: ', out)

                admin.assert_icommand(['iticket', 'delete', ticket_id])

            finally:
                os.remove(file_name)
                admin.run_icommand(['irm', '-f', file_name])
                admin.run_icommand(['iadmin', 'rmgroup', groups])

    def test_access_returns_error_on_invalid_value_for_seconds_until_expiration_parameter(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_access_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                token = irods_rest.authenticate('rods', 'rods', 'native')

                json_string = irods_rest.access(token, logical_path, _seconds_until_expiration=-1)
                self.assertGreater(json_string.find('error'), 0)
                json_object = json.loads(json_string)
                self.assertEqual(json_object['error_code'], -130000)

            finally:
                os.remove(file_name)
                admin.assert_icommand(['irm', '-f', file_name])

    @unittest.skip('Disabled until support in iRODS server is complete.')
    def test_get_configuration(self):
        token  = irods_rest.authenticate('rods', 'rods', 'native')
        result = irods_rest.get_configuration(token)
        assert(result.find('advanced_settings') != -1)

    @unittest.skip('Disabled until support in iRODS server is complete.')
    def test_put_configuration(self):
        file1 = "/etc/irods/test_rest_cfg_put_1.json"
        file2 = "/etc/irods/test_rest_cfg_put_2.json"

        # clean up
        if os.path.exists(file1):
              os.remove(file1)
        if os.path.exists(file2):
              os.remove(file2)

        cfg    = "%5B%7B%22file_name%22%3A%22test_rest_cfg_put_1.json%22%2C%20%22contents%22%3A%7B%22key0%22%3A%22value0%22%2C%22key1%22%20%3A%20%22value1%22%7D%7D%2C%7B%22file_name%22%3A%22test_rest_cfg_put_2.json%22%2C%22contents%22%3A%7B%22key2%22%20%3A%20%22value2%22%2C%22key3%22%20%3A%20%22value3%22%7D%7D%5D"
        token  = irods_rest.authenticate('rods', 'rods', 'native')

        # put config files
        irods_rest.put_configuration(token, cfg)

        # config files should exist
        assert(os.path.exists(file1))
        assert(os.path.exists(file2))

        # confirm contents for file1
        with open(file1) as f:
            data = json.load(f)
            assert(data['key0'] == 'value0')
            assert(data['key1'] == 'value1')

        # confirm contents for file2
        with open(file2) as f:
            data = json.load(f)
            assert(data['key2'] == 'value2')
            assert(data['key3'] == 'value3')

        # clean up /etc/irods
        os.remove(file1)
        os.remove(file2)

    def test_add_and_remove_metadata_on_data_object__issue_118(self):
        data_object = 'metadata_test_data_object'
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            try:
                path = os.path.join(admin.home_collection, data_object)
                admin.assert_icommand(['itouch', path])
                cmds = self.construct_add_metadata_op_for_target(path, 'data_object')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                desired_attr = json.loads(cmds)['operations'][0]['attribute']
                desired_val= json.loads(cmds)['operations'][0]['value']
                self.assertTrue(lib.metadata_attr_with_value_exists(admin, desired_attr, desired_val))

                self.assertTrue(entity_has_metadata(admin, path, 'data_object'))

                cmds = self.construct_remove_metadata_op_for_target(path, 'data_object')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                self.assertFalse(entity_has_metadata(admin, path, 'data_object'))
            finally:
                admin.run_icommand(['irm', '-f', path])

    def test_add_and_remove_metadata_on_collection__issue_118(self):
        collection = 'metadata_test_collection'
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            try:
                path = os.path.join(admin.home_collection, collection)
                admin.assert_icommand(['imkdir', path])
                cmds = self.construct_add_metadata_op_for_target(path, 'collection')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                desired_attr = json.loads(cmds)['operations'][0]['attribute']
                desired_val = json.loads(cmds)['operations'][0]['value']
                self.assertTrue(lib.metadata_attr_with_value_exists(admin, desired_attr, desired_val))

                cmds = self.construct_remove_metadata_op_for_target(path, 'collection')

                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                self.assertFalse(entity_has_metadata(admin, path, 'collection'))
            finally:
                admin.run_icommand(['irm', '-r', '-f', path])

    def test_add_and_remove_metadata_on_user__issue_118(self):
        user = 'metadata_test_user'
        token = irods_rest.authenticate('rods', 'rods', 'native')
        with session.make_session_for_existing_admin() as admin:
            try:
                admin.assert_icommand(['iadmin', 'mkuser', user, 'rodsuser'])
                cmds = self.construct_add_metadata_op_for_target(user, 'user')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                desired_attr = json.loads(cmds)['operations'][0]['attribute']
                desired_val= json.loads(cmds)['operations'][0]['value']
                self.assertTrue(lib.metadata_attr_with_value_exists(admin, desired_attr, desired_val))
                self.assertTrue(entity_has_metadata(admin, user, 'user'))

                cmds = self.construct_remove_metadata_op_for_target(user, 'user')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')
               
                self.assertFalse(entity_has_metadata(admin, user, 'user'))
            finally:
                admin.run_icommand(['iadmin', 'rmuser', user])

    def test_add_and_remove_metadata_on_resource__issue_118(self):
        resource = 'metadata_test_resource'
        token = irods_rest.authenticate('rods', 'rods', 'native')
        with session.make_session_for_existing_admin() as admin:
            try:
                lib.create_ufs_resource(resource, admin)
                cmds = self.construct_add_metadata_op_for_target(resource, 'resource')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                desired_attr = json.loads(cmds)['operations'][0]['attribute']
                desired_val= json.loads(cmds)['operations'][0]['value']
                self.assertTrue(lib.metadata_attr_with_value_exists(admin, desired_attr, desired_val))
                self.assertTrue(entity_has_metadata(admin, resource, 'resource'))

                cmds = self.construct_remove_metadata_op_for_target(resource, 'resource')
                res = irods_rest.metadata(
                    token,
                    cmds
                )
                self.assertEqual(res, '')

                self.assertFalse(entity_has_metadata(admin, resource, 'resource'))
            finally:
                admin.run_icommand(['iadmin', 'rmresc', resource])

    def construct_add_metadata_op_for_target(self, _target, _entity_type):
        return json.dumps(
            {
                "entity_name" : _target,
                "entity_type" : _entity_type,
                "operations"  : [
                    {
                        "operation" : "add",
                        "attribute" : "{}_attrib_{}".format(_entity_type, _target),
                        "value" : "{}_value_{}".format(_entity_type, _target),
                        "units" : "{}_units_{}".format(_entity_type, _target)
                    }
                ]
            }
        )

    
    def construct_remove_metadata_op_for_target(self, _target, _entity_type):
        return json.dumps(
            {
                "entity_name" : _target,
                "entity_type" : _entity_type,
                "operations"  : [
                    {
                        "operation" : "remove",
                        "attribute" : "{}_attrib_{}".format(_entity_type, _target),
                        "value" : "{}_value_{}".format(_entity_type, _target),
                        "units" : "{}_units_{}".format(_entity_type, _target)
                    }
                ]
            }
        )
        
    def test_list(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_count = 10

                dir_name = 'test_list_collection_directory'
                lib.make_large_local_tmp_dir(dir_name, file_count, 1024)

                admin.assert_icommand(['iput', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l', '-r'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                token  = irods_rest.authenticate('rods', 'rods', 'native')
                result = irods_rest.list(token, logical_path, 'false', 'false', 'false', 0, 100)

                cnt = 0
                lst = json.loads(result)
                for entry in lst['_embedded']:
                    lp = entry['logical_path']
                    print(lp)
                    assert(lp.find('junk') != -1)
                    cnt = cnt + 1

                assert(file_count == cnt)

            finally:
                shutil.rmtree(dir_name)
                admin.assert_icommand(['irm', '-f', '-r', dir_name])

    def test_list_honors_recursive__issue_114(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dirname = 'test_list_honors_recursive__issue_114'
                logical_path = os.path.join(admin.home_collection, dirname)
                physical_path = os.path.join(admin.local_session_dir, dirname)
                lib.make_deep_local_tmp_dir(physical_path)

                token = irods_rest.authenticate('rods', 'rods', 'native')

                admin.assert_icommand(['iput', '-r', physical_path, logical_path], 'STDOUT_SINGLELINE', 'Running')

                offset = 0
                limit = 0
                res_deep = irods_rest.list(token, logical_path, 0, 0, 0, offset, limit)
                res_shallow = irods_rest.list(token, logical_path, 0, 0, 0, offset, limit, _recursive=0)

                self.assertGreater(len(res_deep), len(res_shallow))

            finally:
                shutil.rmtree(physical_path)
                admin.assert_icommand(['irm', '-f', '-r', logical_path])


    def test_list_with_limit_and_offset(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_count = 10

                dir_name = 'test_list_collection_directory'
                lib.make_large_local_tmp_dir(dir_name, file_count, 1024)

                admin.assert_icommand(['iput', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l', '-r'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                token  = irods_rest.authenticate('rods', 'rods', 'native')

                offset = 0
                for i in range(1,10):
                    result = irods_rest.list(token, logical_path, 'false', 'false', 'false', offset, 1)
                    lst = json.loads(result)

                    assert(len(lst['_embedded']) == 1)

                    fn = 'junk000' + str(offset)
                    lp = lst['_embedded'][0]['logical_path']
                    print(fn+' vs '+lp)

                    assert(lp.find(fn) != -1)

                    offset = offset + 1

            finally:
                shutil.rmtree(dir_name)
                admin.run_icommand(['irm', '-f', '-r', dir_name])

    def test_list_with_accoutrements(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_access_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                admin.assert_icommand(['imeta', 'set', '-d', logical_path, 'attr', 'val', 'unit'])

                token  = irods_rest.authenticate('rods', 'rods', 'native')
                result = irods_rest.list(token, logical_path, 1, 1, 1, 0, 0)

                lst = json.loads(result)['_embedded'][0]

                assert(lst['type'] == 'data_object')

                md = lst['metadata'][0]
                assert(md['attribute'] == 'attr')
                assert(md['value'] == 'val')
                assert(md['units'] == 'unit')

                stat = lst['status_information']
                assert(stat['last_write_time'] != '')
                assert(stat['size'] == 1024)

                perm = lst['permission_information']
                assert(perm['rods']== 'own')

            finally:
                os.remove(file_name)
                admin.run_icommand(['irm', '-f', file_name])

    def test_query_handles_case_insensitivity__issue_124(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dobj_name = 'mAy_I_tAkE_yOuR_hAt_SIR'
                a_path = os.path.join(admin.home_collection, dobj_name)
                admin.assert_icommand(['itouch', a_path])

                token  = irods_rest.authenticate('rods', 'rods', 'native')

                query_lower = "SELECT DATA_PATH, COLL_NAME, DATA_NAME, DATA_SIZE WHERE DATA_NAME LIKE '%i%'"
                query_upper = "SELECT DATA_PATH, COLL_NAME, DATA_NAME, DATA_SIZE WHERE DATA_NAME LIKE '%I%'"

                result_lower = irods_rest.query(token, query_lower, 1, 0, 'general', _case_sensitive='0')
                result_upper = irods_rest.query(token, query_upper, 1, 0, 'general')
                
                embedded_lower = sorted(json.loads(result_lower)['_embedded'])
                embedded_upper = sorted(json.loads(result_upper)['_embedded'])
                
                self.assertEqual(embedded_lower, embedded_upper)

            finally:
                admin.run_icommand(['irm', '-f', '-r', a_path])

    def test_query_with_limit_and_offset(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_count = 10

                dir_name = 'test_query_collection_directory'
                lib.make_large_local_tmp_dir(dir_name, file_count, 1024)

                admin.assert_icommand(['iput', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l', '-r'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                token  = irods_rest.authenticate('rods', 'rods', 'native')

                query = "SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME = '"+logical_path+"'"

                offset = 0
                for i in range(1,10):
                    fn = 'junk000' + str(offset)

                    result = irods_rest.query(token, query, 1, offset, 'general')

                    print('*****************\n')
                    print(result)
                    print('*****************\n')

                    res = json.loads(result)
                    assert(len(res['_embedded']) == 1)

                    arr = res['_embedded'][0]

                    assert(arr[0] == logical_path)
                    assert(arr[1] == fn)

                    offset = offset + 1

            finally:
                shutil.rmtree(dir_name)
                admin.run_icommand(['irm', '-f', '-r', dir_name])

    def test_query_with_case_sensitive_search_option_set_to_zero__issue_71(self):
        with session.make_session_for_existing_admin() as admin:
            pwd, _ = lib.execute_command(['ipwd'])
            pwd = pwd.rstrip()
            collection = os.path.join(pwd, 'test_query_with_case_sensitive_search_option_set_to_zero__issue_71')

            try:
                admin.assert_icommand(['imkdir', collection])

                token = irods_rest.authenticate(admin.username, admin.password, 'native')

                # Search for newly created collection using the case-sensitive search option.
                gql = "SELECT COLL_NAME WHERE COLL_NAME = '{0}'".format(collection.upper())
                result = irods_rest.query(token, gql, 1, 0, 'general', _case_sensitive='0')

                print('*****************\n')
                print(result)
                print('*****************\n')

                res = json.loads(result)
                self.assertEqual(len(res['_embedded']), 1)

                arr = res['_embedded'][0]
                self.assertEqual(arr[0], collection)

            finally:
                admin.run_icommand(['irmdir', collection])

    def test_query_with_distinct_search_option_set_to_zero__issue_71(self):
        with session.make_session_for_existing_admin() as admin:
            other_resc = 'other_resc_71'
            data_object = 'foo.issue_71'

            try:
                lib.create_ufs_resource(other_resc, admin)

                admin.assert_icommand(['itouch', '-R', 'demoResc', data_object])
                admin.assert_icommand(['irepl', '-R', other_resc, data_object])
                admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', [' 0 demoResc', ' 1 ' + other_resc])

                token = irods_rest.authenticate(admin.username, admin.password, 'native')

                # Search for data object. The no-distinct option should result in
                # two rows being returned. One for each replica.
                gql = "SELECT DATA_NAME WHERE DATA_NAME = '{0}'".format(data_object)
                result = irods_rest.query(token, gql, 100, 0, 'general', _distinct='0')

                print('*****************\n')
                print(result)
                print('*****************\n')

                res = json.loads(result)
                self.assertEqual(len(res['_embedded']), 2)

                rows = res['_embedded']
                self.assertEqual(rows[0][0], data_object)
                self.assertEqual(rows[1][0], data_object)

            finally:
                admin.run_icommand(['irm', '-f', data_object])
                admin.run_icommand(['iadmin', 'rmresc', other_resc])

    def test_query_returns_error_on_invalid_options__issue_71(self):
        with session.make_session_for_existing_admin() as admin:
            token = irods_rest.authenticate(admin.username, admin.password, 'native')

            result = irods_rest.query(token, 'select COLL_NAME', 1, 0, 'general', _case_sensitive='nopes')
            self.assertIn('error', result)

            result = irods_rest.query(token, 'select COLL_NAME', 1, 0, 'general', _distinct='nopes')
            self.assertIn('error', result)

    @unittest.skip('Hangs: #130')
    def test_stream_put_and_get(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name  = 'stream_put_and_get_file'
                downloaded_file_name = file_name + '2'
                with open(file_name, 'w') as f:
                    f.write('This is some test data.  This is only a test.')

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                token = irods_rest.authenticate('rods', 'rods', 'native')

                irods_rest.put(token, file_name, logical_path)
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                irods_rest.get(token, downloaded_file_name, logical_path)

                sz  = os.path.getsize(file_name)
                sz2 = os.path.getsize(downloaded_file_name)

                self.assertEqual(sz, sz2)

            finally:
                os.remove(file_name)
                os.remove(downloaded_file_name)
                admin.run_icommand(['irm', '-f', file_name])

    def test_zone_report(self):
        with session.make_session_for_existing_admin() as admin:
            zr0, _ = lib.execute_command(['izonereport'])
            js0 = json.loads(zr0)

            token = irods_rest.authenticate('rods', 'rods', 'native')
            zr1 = irods_rest.zone_report(token)
            js1 = json.loads(zr1)

            assert(js0 == js1)

    def test_changing_passwords_is_supported__issue_99(self):
        # Show that the user can execute commands without error.
        self.user.assert_icommand(['ils', '-ld'], 'STDOUT', [self.user.session_collection])

        # Show that changing the user's password will require them to reauthenticate.
        token = irods_rest.authenticate('rods', 'rods', 'native')
        old_password = self.user.password
        new_password = 'newpass'
        irods_rest.admin(token, 'modify', 'user', self.user.username, 'password', new_password, None, None, None)
        self.user.assert_icommand(['ils', '-ld'], 'STDERR', ['CAT_INVALID_AUTHENTICATION'])

        # Show that reauthenticating with the new password restores the user's ability
        # to execute commands. This proves the REST API is able to change passwords without
        # requiring the user to obfuscate the password first.
        self.user.assert_icommand(['iinit'], 'STDOUT', 'Enter your current iRODS password:', input=new_password + '\n')
        self.user.assert_icommand(['ils', '-ld'], 'STDOUT', [self.user.session_collection])

        # Restore the user's password for other tests.
        irods_rest.admin(token, 'modify', 'user', self.user.username, 'password', old_password, None, None, None)
        self.user.assert_icommand(['iinit'], 'STDOUT', 'Enter your current iRODS password:', input=old_password + '\n')
        self.user.assert_icommand(['ils', '-ld'], 'STDOUT', [self.user.session_collection])

    def test_replicate_data_object(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            new_resc_name = 'newResc'
            logical_path = os.path.join(admin.home_collection, 'hello.cpp')

            try:
                lib.create_ufs_resource(new_resc_name, admin)

                # Create a data object on default resource
                admin.assert_icommand(["itouch", logical_path])
                self.assertTrue(lib.replica_exists_on_resource(admin, logical_path, admin.default_resource))
                self.assertFalse(lib.replica_exists_on_resource(admin, logical_path, new_resc_name))

                # Replicate the data object to the new resource and ensure a new replica appears on that resource
                res = irods_rest.logical_path_replicate(token, logical_path, _dst_resource=new_resc_name)
                self.assertEqual(res, "")
                self.assertTrue(lib.replica_exists_on_resource(admin, logical_path, new_resc_name))

            finally:
                admin.run_icommand(['iadmin', 'rmresc', new_resc_name])
                admin.run_icommand(['irm', '-f', logical_path])

    def test_replicate_collection(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            coll_name = 'irods_coll_1'
            coll = os.path.join(admin.home_collection, coll_name)

            data_obj_name = 'data_object_1'
            data_obj = os.path.join(coll, data_obj_name)

            resc_one = "newResc1"

            try:
                lib.create_ufs_resource(resc_one, admin)

                # Create a collection and create a data object inside of it on the default resource
                admin.assert_icommand(['imkdir', coll])
                admin.assert_icommand(['itouch', data_obj])
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, admin.default_resource))
                self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, resc_one))

                # Replicate the collection without the recursive flag
                repl_output = irods_rest.logical_path_replicate(token, coll, _dst_resource=resc_one, _recursive=False)
                self.assertIn("Make sure you want to replicate the whole sub-tree", repl_output)
                self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, resc_one))

                # Replicate the collection to the new resource and ensure a new replica appears on that resource
                repl_output = irods_rest.logical_path_replicate(token, coll, _dst_resource=resc_one, _recursive=True)
                self.assertEqual(repl_output, "")
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, resc_one))

            finally:
                admin.run_icommand(['iadmin', 'rmresc', resc_one])
                admin.run_icommand(['irm', '-r', '-f', coll])

    def test_trim_data_object(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            data_obj_name = "data_obj"
            data_obj = os.path.join(admin.home_collection, data_obj_name)

            resource_base_name = 'newResc'
            resources = [f'{resource_base_name}_{i}' for i in range(2)]

            try:
                # Create a data object on the default resource
                admin.assert_icommand(['itouch', data_obj])
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, admin.default_resource))

                # Replicate the data object to each new resource
                for resc in resources:
                    lib.create_ufs_resource(resc, admin)
                    self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, resc))
                    admin.assert_icommand(['irepl', data_obj, '-R', resc])
                    self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, resc))

                # Trim the data object targeting a particular resource and ensure that replica is trimmed
                trim_resource = f'{resource_base_name}_0'
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, trim_resource))
                res = irods_rest.logical_path_trim(token, data_obj, _src_resc=trim_resource)
                self.assertEqual(res, "")
                self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, trim_resource))
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, admin.default_resource))

            finally:
                admin.run_icommand(['irm', '-f', data_obj])
                for resc in resources:
                   admin.run_icommand(['iadmin', 'rmresc', resc])

    def test_trim_collection(self):
        token = irods_rest.authenticate('rods', 'rods', 'native')

        with session.make_session_for_existing_admin() as admin:
            coll_name = 'collection_one'
            coll = os.path.join(admin.home_collection, coll_name)
            data_obj = os.path.join(coll, 'junk0000')

            resource_base_name = 'newResc'
            resources = [f'{resource_base_name}_{i}' for i in range(2)]

            try:
                # Create a directory and put it as a collection to the default resource
                lib.make_large_local_tmp_dir(coll_name, 1, 10)
                admin.assert_icommand(['iput', '-r', coll_name, coll], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-r', coll], 'STDOUT', coll_name)
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, admin.default_resource))

                # Replicate the collection to each new resource
                for resc in resources:
                    lib.create_ufs_resource(resc, admin)
                    self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, resc))
                    admin.assert_icommand(['irepl', '-r', coll, '-R', resc])
                    self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, resc))

                # Trim the collection without specifying the recursive flag
                trim_resource = f'{resource_base_name}_0'
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, trim_resource))
                res = irods_rest.logical_path_trim(token, coll, _src_resc=trim_resource)
                self.assertIn('Make sure you want to trim the whole sub-tree', res)
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, trim_resource))

                # Trim the collection targeting a particular resource and ensure the replica is trimmed
                res = irods_rest.logical_path_trim(token, data_obj, _src_resc=trim_resource, _recursive=True)
                self.assertEqual(res, '')
                self.assertFalse(lib.replica_exists_on_resource(admin, data_obj, trim_resource))
                self.assertTrue(lib.replica_exists_on_resource(admin, data_obj, admin.default_resource))

            finally:
                shutil.rmtree(coll_name)
                admin.run_icommand(['irm', '-r', '-f', coll_name])
                for resc in resources:
                   admin.run_icommand(['iadmin', 'rmresc', resc])

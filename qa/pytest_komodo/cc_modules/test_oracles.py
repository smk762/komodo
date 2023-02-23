#!/usr/bin/env python3
# Copyright (c) 2021 SuperNET developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import pytest
from slickrpc.exc import RpcException as RPCError
from lib.pytest_util import validate_template, mine_and_waitconfirms, randomstring,\
                            randomhex, validate_raddr_pattern


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestOracleInstance:

    def test_oraclescreate_list_info(self, oracle_instance):
        create_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        list_schema = {
            'type': 'array',
            'items': {'type': 'string'}
        }

        info_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'txid': {'type': 'string'},
                'description': {'type': 'string'},
                'name': {'type': 'string'},
                'format': {'type': 'string'},
                'marker': {'type': 'string'},
                'registered': {
                    'type': 'array',
                    'items': {
                        'type': 'object',
                        'properties': {
                            'publisher': {'type': 'string'},
                            'baton': {'type': 'string'},
                            'batontxid': {'type': 'string'},
                            'lifetime': {'type': 'string'},
                            'funds': {'type': 'string'},
                            'datafee': {'type': 'string'}
                        }
                    }
                }
            },
            'required': ['result']
        }

        rpc1 = oracle_instance.rpc[0]
        rpc2 = oracle_instance.rpc[1]
        # test oracle creation
        oracle_instance.new_oracle(rpc2, schema=create_schema)

        res = rpc1.oracleslist()
        validate_template(res, list_schema)

        res = rpc1.oraclesinfo(oracle_instance.base_oracle.get('oracle_id'))
        validate_template(res, info_schema)

    def test_oraclesaddress(self, oracle_instance):

        oraclesaddress_schema = {
            'type': 'object',
            'properties': {
                "result": {'type': 'string'},
                "OraclesAddress": {'type': 'string'},
                "OraclesBalance": {'type': 'number'},
                "OraclesNormalAddress": {'type': 'string'},
                "OraclesNormalBalance": {'type': 'number'},
                "OraclesTokensAddress": {'type': 'string'},
                "PubkeyCCaddress(Oracles)": {'type': 'string'},
                "PubkeyCCbalance(Oracles)": {'type': 'number'},
                "myCCAddress(Oracles)": {'type': 'string'},
                "myCCbalance(Oracles)": {'type': 'number'},
                "myaddress": {'type': 'string'},
                "mybalance": {'type': 'number'}
            },
            'required': ['result']
        }

        rpc1 = oracle_instance.rpc[0]
        rpc2 = oracle_instance.rpc[1]
        pubkey = oracle_instance.pubkey[0]

        if not oracle_instance.base_oracle:
            oracle_instance.new_oracle(rpc2)

        res = rpc1.oraclesaddress(pubkey)
        validate_template(res, oraclesaddress_schema)

        res = rpc1.oraclesaddress(pubkey)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

        res = rpc1.oraclesaddress()
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

    def test_oracles_data_interactions(self, oracle_instance):
        general_hex_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        sample_schema = {
            'type': 'object',
            'properties': {
                'samples': {
                    'type': 'array',
                    'items': {
                        'type': 'object',
                        'properties': {
                            'txid': {'type': 'string'},
                            'data': {'type': 'string'}
                        }
                    }
                }
            }
        }

        rpc1 = oracle_instance.rpc[0]
        rpc2 = oracle_instance.rpc[1]
        amount = '10000000'
        sub_amount = '0.1'

        if not oracle_instance.base_oracle:
            oracle_instance.new_oracle(rpc2)

        oracle_id = oracle_instance.base_oracle.get('oracle_id')

        # Fund fresh oracle
        res = rpc1.oraclesfund(oracle_id)
        validate_template(res, general_hex_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

        # Register as publisher
        res = rpc1.oraclesregister(oracle_id, amount)
        validate_template(res, general_hex_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

        # Subscrive to new oracle
        oraclesinfo = rpc1.oraclesinfo(oracle_id)
        publisher = oraclesinfo.get('registered')[0].get('publisher')
        res = rpc1.oraclessubscribe(oracle_id, publisher, sub_amount)
        validate_template(res, general_hex_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, oracle_instance.rpc[0])

        # Publish new data
        res = rpc1.oraclesdata(oracle_id, '0a74657374737472696e67')  # teststring
        validate_template(res, general_hex_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

        # Check data
        oraclesinfo = rpc1.oraclesinfo(oracle_id)
        baton = oraclesinfo.get('registered')[0].get('batontxid')
        res = rpc1.oraclessample(oracle_id, baton)
        validate_template(res, sample_schema)
        assert res.get('txid') == baton

    def test_bad_calls(self, oracle_instance):
        if not oracle_instance.base_oracle:
            oracle_instance.new_oracle(oracle_instance.rpc[1])

        oracle = oracle_instance
        # trying to register with negative datafee
        res = oracle.rpc[0].oraclesregister(oracle.base_oracle.get('oracle_id'), "-100")
        assert res.get('error')

        # trying to register with zero datafee
        res = oracle.rpc[0].oraclesregister(oracle.base_oracle.get('oracle_id'), "0")
        assert res.get('error')

        # trying to register with datafee less than txfee
        res = oracle.rpc[0].oraclesregister(oracle.base_oracle.get('oracle_id'), "500")
        assert res.get('error')

        # trying to register valid (unfunded)
        res = oracle.rpc[0].oraclesregister(oracle.base_oracle.get('oracle_id'), "1000000")
        assert res.get('error')

        # trying to register with invalid datafee
        res = oracle.rpc[0].oraclesregister(oracle.base_oracle.get('oracle_id'), "asdasd")
        assert res.get('error')

        # looking up non-existent oracle should return error.
        res = oracle.rpc[0].oraclesinfo("none")
        assert res.get('error')

        # attempt to create oracle with not valid data type should return error
        res = oracle.rpc[0].oraclescreate("Test", "Test", "Test")
        assert res.get('error')

        # attempt to create oracle with name > 32 symbols should return error
        too_long_name = randomstring(33)
        res = oracle.rpc[0].oraclescreate(too_long_name, "Test", "s")
        assert res.get('error')

        # attempt to create oracle with description > 4096 symbols should return error
        too_long_description = randomstring(4100)
        res = oracle.rpc[0].oraclescreate("Test", too_long_description, "s")
        assert res.get('error')

    def test_oracles_data(self, oracle_instance):
        oracles_data = {
            's': '05416e746f6e',
            'S': '000161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161',
            'd': '0101',
            'D': '010001',
            'c': 'ff',
            'C': 'ff',
            't': 'ffff',
            'T': 'ffff',
            'i': 'ffffffff',
            'I': 'ffffffff',
            'l': '00000000ffffffff',
            'L': '00000000ffffffff',
            'h': 'ffffffff00000000ffffffff00000000ffffffff00000000ffffffff00000000',
        }

        oracles_response = {
            's_un': 'Anton',
            'S_un': 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
            'd_un': '01',
            'D_un': '01',
            'c_un': '-1',
            'C_un': '255',
            't_un': '-1',
            'T_un': '65535',
            'i_un': '-1',
            'I_un': '4294967295',
            'l_un': '-4294967296',
            'L_un': '18446744069414584320',
            'h_un': '00000000ffffffff00000000ffffffff00000000ffffffff00000000ffffffff'
        }

        rpc = oracle_instance.rpc[0]
        oracles = oracle_instance.new_oracle(rpc, o_type=list(oracles_data.keys()))

        txid = ""
        for oracle in oracles:
            res = rpc.oraclesfund(oracle.get('oracle_id'))
            txid = rpc.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc)

        for oracle in oracles:
            res = rpc.oraclesregister(oracle.get('oracle_id'), '10000000')
            txid = rpc.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc)

        for oracle in oracles:
            oraclesinfo = rpc.oraclesinfo(oracle.get('oracle_id'))
            publisher = oraclesinfo.get('registered')[0].get('publisher')
            res = rpc.oraclessubscribe(oracle.get('oracle_id'), publisher, '0.1')
            txid = rpc.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc)

        o_data = ""
        for oracle, o_type in zip(oracles, oracles_data.keys()):
            res = rpc.oraclesdata(oracle.get('oracle_id'), oracles_data.get(o_type))
            assert res.get('result') == 'success'
            o_data = rpc.sendrawtransaction(res.get("hex"))
        mine_and_waitconfirms(o_data, rpc)

        for oracle, o_type in zip(oracles, oracles_data.keys()):
            oraclesinfo = rpc.oraclesinfo(oracle.get('oracle_id'))
            baton = oraclesinfo.get('registered')[0].get('batontxid')

            res = rpc.oraclessample(oracle.get('oracle_id'), baton)
            assert (res.get('data')[0] == oracles_response.get(str(o_type) + '_un'))
